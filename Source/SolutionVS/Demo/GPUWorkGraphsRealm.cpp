
#include "stdafx.h"
#include "GPUWorkGraphsRealm.h"
#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Core/Math.h"
#include "Graf/GrafRenderer.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/DX12/GrafSystemDX12.h"
#include "GPUWorkGraphsCommon.h"
#include "ProceduralGenerationGraph.h"
#pragma comment(lib, "UnlimRealms.lib")

using namespace UnlimRealms;

#define UR_GRAF_SYSTEM GrafSystemDX12

static const ur_float4 DebugLabelPass = { 0.8f, 0.8f, 1.0f, 1.0f };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// gbuffer & lighting render target desc

enum RTImageType
{
	RTImage_Depth = 0,
	//RTImage_Geometry0, // xyz: baseColor; w: materialTypeID;
	//RTImage_Lighting, // xyz: hdr color; w: unused;
	RTImageCount,
	RTColorImageCount = RTImageCount - 1,
};

struct RTImageDesc
{
	GrafImageUsageFlag Usage;
	GrafFormat Format;
	GrafClearValue ClearColor;
	ur_bool HasHistory; // number of images stored, (Count > 1) means keep history of (Count-1) frames
};

static const ur_uint RTHistorySize = 0; // number of additional RT images to store history

static const RTImageDesc g_RTImageDesc[RTImageCount] = {
	{ GrafImageUsageFlag::DepthStencilRenderTarget, GrafFormat::D24_UNORM_S8_UINT, { 1.0f, 0.0f, 0.0f, 0.0f }, true },
	//{ GrafImageUsageFlag::ColorRenderTarget, GrafFormat::R8G8B8A8_UNORM, { 0.0f, 0.0f, 0.0f, 0.0f }, false },
	//{ GrafImageUsageFlag::ColorRenderTarget, GrafFormat::R16G16B16A16_SFLOAT, { 0.0f, 0.0f, 0.0f, 0.0f }, true }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GPUWorkGraphsRealm::GPUWorkGraphsRealm() :
	camera(*this),
	cameraControl(*this, &camera, CameraControl::Mode::AroundPoint)
{
}

GPUWorkGraphsRealm::~GPUWorkGraphsRealm()
{
}

Result GPUWorkGraphsRealm::Initialize()
{
#if (UR_GRAF_DX12_WORK_GRAPHS_SUPPORTED)

	RenderRealm::InitParams realmParams = RenderRealm::InitParams::Default;
	realmParams.Title = L"GPUWorkGraphsApp";
	realmParams.CanvasStyle = WinCanvas::Style::OverlappedWindowMaximized;
	realmParams.GrafSystemType = RenderRealm::GrafSystemType::DX12;
	realmParams.RendererParams = GrafRenderer::InitParams::Default;

	Result res = RenderRealm::Initialize(realmParams);
	if (Failed(res))
		return res;

	// procedural object desc
	this->proceduralObject = {};
	this->proceduralObject.position = ur_float3(0.0f, 0.0f, 0.0f);
	this->proceduralObject.extent = 1000.0f;

	// camera
	this->cameraControl.SetTargetPoint(this->proceduralObject.position);
	this->cameraControl.SetSpeed(5.0);
	this->camera.SetProjection(0.1f, 1.0e+4f, camera.GetFieldOFView(), camera.GetAspectRatio());
	this->camera.SetPosition(this->proceduralObject.position + ur_float3(0.0f, 0.0f, -this->proceduralObject.extent * 2.0f));
	this->camera.SetLookAt(cameraControl.GetTargetPoint(), cameraControl.GetWorldUp());

	return res;
#else
	return Result(NotImplemented); // used DX12 SDK does not support work graphs
#endif
}

Result GPUWorkGraphsRealm::InitializeGraphicObjects()
{
	GrafSystem* grafSystem = this->GetGrafRenderer()->GetGrafSystem();
	GrafDevice* grafDevice = this->GetGrafRenderer()->GetGrafDevice();
	this->graphicsObjects.reset(new GraphicsObjects());
	
	Result res = Success;
	do
	{
	#if (GPUWORKGRAPH_SAMPLE)
		res = grafSystem->CreateShaderLib(this->graphicsObjects->workGraphShaderLib);
		if (Succeeded(res))
		{
			GrafShaderLib::EntryPoint shaderLibEntries[] = {
				{ "firstNode", GrafShaderType::Node }
			};
			res = GrafUtils::CreateShaderLibFromFile(*grafDevice, "GPUWorkGraphs_cs_lib", shaderLibEntries, ur_array_size(shaderLibEntries), this->graphicsObjects->workGraphShaderLib);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateDescriptorTableLayout(this->graphicsObjects->workGraphDescTableLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc descTableLayoutRanges[] = {
				g_CBDescriptor,
				g_UAVDescriptor
			};
			GrafDescriptorTableLayoutDesc descTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute),
				descTableLayoutRanges, ur_array_size(descTableLayoutRanges)
			};
			res = this->graphicsObjects->workGraphDescTableLayout->Initialize(grafDevice, { descTableLayoutDesc });
		}
		if (Failed(res))
			break;

		this->graphicsObjects->workGraphDescTable.reset(new GrafManagedDescriptorTable(*this->GetGrafRenderer()));
		res = this->graphicsObjects->workGraphDescTable->Initialize({ this->graphicsObjects->workGraphDescTableLayout.get() });
		if (Failed(res))
			break;

		res = grafSystem->CreateWorkGraphPipeline(this->graphicsObjects->workGraphPipeline);
		if (Succeeded(res))
		{
			GrafDescriptorTableLayout* workGraphDescriptorLayouts[] = {
				this->graphicsObjects->workGraphDescTableLayout.get()
			};
			GrafWorkGraphPipeline::InitParams initParams = {};
			initParams.Name = "HelloWorkGraphs";
			initParams.ShaderLib = this->graphicsObjects->workGraphShaderLib.get();
			initParams.DescriptorTableLayouts = workGraphDescriptorLayouts;
			initParams.DescriptorTableLayoutCount = ur_array_size(workGraphDescriptorLayouts);
			res = this->graphicsObjects->workGraphPipeline->Initialize(grafDevice, initParams);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateBuffer(this->graphicsObjects->workGraphDataBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams bufferParams = {};
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferSrc);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
			bufferParams.BufferDesc.SizeInBytes = 1024 * sizeof(ur_uint32);
			res = this->graphicsObjects->workGraphDataBuffer->Initialize(grafDevice, bufferParams);
			
			// upload initial data
			std::vector<ur_uint32> initialData(1024, 0);
			this->GetGrafRenderer()->Upload((ur_byte*)initialData.data(), this->graphicsObjects->workGraphDataBuffer.get(), bufferParams.BufferDesc.SizeInBytes);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateBuffer(this->graphicsObjects->workGraphReadbackBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams bufferParams = {};
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::TransferDst);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::CpuVisible);
			bufferParams.BufferDesc.SizeInBytes = this->graphicsObjects->workGraphDataBuffer->GetDesc().SizeInBytes;
			res = this->graphicsObjects->workGraphReadbackBuffer->Initialize(grafDevice, bufferParams);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateFence(this->graphicsObjects->workGraphReadbackFence);
		if (Succeeded(res))
		{
			this->graphicsObjects->workGraphReadbackFence->Initialize(grafDevice);
			this->graphicsObjects->workGraphReadbackFence->SetState(GrafFenceState::Signaled);
		}
		if (Failed(res))
			break;
	#else

		res = grafSystem->CreateShaderLib(this->graphicsObjects->proceduralGraphShaderLib);
		if (Succeeded(res))
		{
			GrafShaderLib::EntryPoint shaderLibEntries[] = {
				{ "PartitionUpdateRootNode", GrafShaderType::Node }
			};
			res = GrafUtils::CreateShaderLibFromFile(*grafDevice, "ProceduralGenerationGraph_lib", shaderLibEntries, ur_array_size(shaderLibEntries), this->graphicsObjects->proceduralGraphShaderLib);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateDescriptorTableLayout(this->graphicsObjects->proceduralGraphDescTableLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc descTableLayoutRanges[] = {
				g_ProceduralConstsDescriptor,
				g_PartitionDataDescriptor
			};
			GrafDescriptorTableLayoutDesc descTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute),
				descTableLayoutRanges, ur_array_size(descTableLayoutRanges)
			};
			res = this->graphicsObjects->proceduralGraphDescTableLayout->Initialize(grafDevice, { descTableLayoutDesc });
		}
		if (Failed(res))
			break;

		this->graphicsObjects->proceduralGraphDescTable.reset(new GrafManagedDescriptorTable(*this->GetGrafRenderer()));
		res = this->graphicsObjects->proceduralGraphDescTable->Initialize({ this->graphicsObjects->proceduralGraphDescTableLayout.get() });
		if (Failed(res))
			break;

		res = grafSystem->CreateWorkGraphPipeline(this->graphicsObjects->proceduralGraphPipeline);
		if (Succeeded(res))
		{
			GrafDescriptorTableLayout* descriptorLayouts[] = {
				this->graphicsObjects->proceduralGraphDescTableLayout.get()
			};
			GrafWorkGraphPipeline::InitParams initParams = {};
			initParams.Name = "ProceduralWorkGraph";
			initParams.ShaderLib = this->graphicsObjects->proceduralGraphShaderLib.get();
			initParams.DescriptorTableLayouts = descriptorLayouts;
			initParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
			res = this->graphicsObjects->proceduralGraphPipeline->Initialize(grafDevice, initParams);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateBuffer(this->graphicsObjects->partitionDataBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams bufferParams = {};
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferSrc);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
			bufferParams.BufferDesc.SizeInBytes = PartitionDataBufferSize;
			res = this->graphicsObjects->partitionDataBuffer->Initialize(grafDevice, bufferParams);

			// initial setup of partition data buffer
			std::vector<ur_byte> partitionInitialData(PartitionDataNodesOfs);
			ur_byte* nodesDebugPtr = reinterpret_cast<ur_byte*>(partitionInitialData.data() + PartitionDataDebugOfs);
			memset(nodesDebugPtr, 0, sizeof(PartitionDataDebugSize));
			ur_uint32* nodesCounterPtr = reinterpret_cast<ur_uint32*>(partitionInitialData.data() + PartitionDataNodesCounterOfs);
			*nodesCounterPtr = 0;
			ur_uint32* freeNodeIdPtr = reinterpret_cast<ur_uint32*>(partitionInitialData.data() + PartitionDataFreeNodesOfs);
			for (ur_uint32 inode = 0; inode < PartitionNodeCountMax; ++inode)
			{
				*freeNodeIdPtr++ = inode;
			}
			this->GetGrafRenderer()->Upload((ur_byte*)partitionInitialData.data(), this->graphicsObjects->partitionDataBuffer.get(), partitionInitialData.size());
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateBuffer(this->graphicsObjects->readbackBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams bufferParams = {};
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::TransferDst);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::CpuVisible);
			bufferParams.BufferDesc.SizeInBytes = this->graphicsObjects->partitionDataBuffer->GetDesc().SizeInBytes;
			res = this->graphicsObjects->readbackBuffer->Initialize(grafDevice, bufferParams);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateFence(this->graphicsObjects->readbackFence);
		if (Succeeded(res))
		{
			this->graphicsObjects->readbackFence->Initialize(grafDevice);
			this->graphicsObjects->readbackFence->SetState(GrafFenceState::Signaled);
			this->graphicsObjects->readbackPending = false;
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateRenderPass(this->graphicsObjects->directRenderPass);
		if (Succeeded(res))
		{
			GrafRenderPassImageDesc grafRenderPassImages[] = {
			{ // color
				GrafFormat::B8G8R8A8_UNORM, // renderer's canvas format
				GrafImageState::ColorWrite, GrafImageState::ColorWrite,
				GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store,
				GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
			},
			{ // depth
				GrafFormat::D24_UNORM_S8_UINT,
				GrafImageState::DepthStencilWrite, GrafImageState::DepthStencilWrite,
				GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store,
				GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
			}
			};
			res = this->graphicsObjects->directRenderPass->Initialize(grafDevice, { grafRenderPassImages, ur_array_size(grafRenderPassImages) });
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateRenderPass(this->graphicsObjects->gbufferRenderPass);
		if (Succeeded(res))
		{
			GrafRenderPassImageDesc rasterRenderPassImageDesc[RTImageCount];
			for (ur_uint imageIdx = 0; imageIdx < RTImageCount; ++imageIdx)
			{
				auto& passImageDesc = rasterRenderPassImageDesc[imageIdx];
				const RTImageDesc& rtImageDesc = g_RTImageDesc[imageIdx];
				passImageDesc.Format = rtImageDesc.Format;
				passImageDesc.InitialState = (GrafImageUsageFlag::DepthStencilRenderTarget == rtImageDesc.Usage ? GrafImageState::DepthStencilWrite : GrafImageState::ColorWrite);
				passImageDesc.FinalState = passImageDesc.InitialState;
				passImageDesc.LoadOp = GrafRenderPassDataOp::Load;
				passImageDesc.StoreOp = GrafRenderPassDataOp::Store;
				passImageDesc.StencilLoadOp = GrafRenderPassDataOp::DontCare;
				passImageDesc.StencilStoreOp = GrafRenderPassDataOp::DontCare;
			}
			res = this->graphicsObjects->gbufferRenderPass->Initialize(grafDevice, { rasterRenderPassImageDesc, ur_array_size(rasterRenderPassImageDesc) });
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateShaderLib(this->graphicsObjects->proceduralRenderShaderLib);
		if (Succeeded(res))
		{
			GrafShaderLib::EntryPoint shaderLibEntries[] = {
				{ "PartitionStructureVS", GrafShaderType::Vertex },
				{ "PartitionStructurePS", GrafShaderType::Pixel }
			};
			res = GrafUtils::CreateShaderLibFromFile(*grafDevice, "ProceduralGenerationRender_lib", shaderLibEntries, ur_array_size(shaderLibEntries), this->graphicsObjects->proceduralRenderShaderLib);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateDescriptorTableLayout(this->graphicsObjects->proceduralRenderDescTableLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc descTableLayoutRanges[] = {
				g_SceneRenderConstsDescriptor,
				g_PartitionDataDescriptor
			};
			GrafDescriptorTableLayoutDesc descTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute),
				descTableLayoutRanges, ur_array_size(descTableLayoutRanges)
			};
			res = this->graphicsObjects->proceduralRenderDescTableLayout->Initialize(grafDevice, { descTableLayoutDesc });
		}
		if (Failed(res))
			break;

		this->graphicsObjects->proceduralRenderDescTable.reset(new GrafManagedDescriptorTable(*this->GetGrafRenderer()));
		res = this->graphicsObjects->proceduralRenderDescTable->Initialize({ this->graphicsObjects->proceduralRenderDescTableLayout.get() });
		if (Failed(res))
			break;

		res = grafSystem->CreatePipeline(this->graphicsObjects->proceduralRenderPipeline);
		{
			GrafShader* shaderStages[] = {
				this->graphicsObjects->proceduralRenderShaderLib->GetShader(0),
				this->graphicsObjects->proceduralRenderShaderLib->GetShader(1)
			};
			GrafDescriptorTableLayout* descriptorLayouts[] = {
				this->graphicsObjects->proceduralRenderDescTableLayout.get(),
			};
			GrafColorBlendOpDesc colorTargetBlendOpDesc[] = {
				GrafColorBlendOpDesc::Default
			};
			GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
			pipelineParams.RenderPass = this->graphicsObjects->directRenderPass.get();
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.DescriptorTableLayouts = descriptorLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
			pipelineParams.VertexInputDesc = ur_null;
			pipelineParams.VertexInputCount = 0;
			pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::LineList;
			pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
			pipelineParams.CullMode = GrafCullMode::None;
			pipelineParams.DepthTestEnable = true;
			pipelineParams.DepthWriteEnable = true;
			pipelineParams.DepthCompareOp = GrafCompareOp::LessOrEqual;
			pipelineParams.ColorBlendOpDesc = colorTargetBlendOpDesc;
			pipelineParams.ColorBlendOpDescCount = ur_array_size(colorTargetBlendOpDesc);
			res = this->graphicsObjects->proceduralRenderPipeline->Initialize(grafDevice, pipelineParams);
		}
		if (Failed(res))
			break;

	#endif
	} while (false);
	if (Failed(res))
	{
		this->DeinitializeGraphicObjects();
		return ResultError(Failure, "GPUWorkGraphsRealm: failed to initialize graphic objects");
	}

	return Result(Success);
}

Result GPUWorkGraphsRealm::DeinitializeGraphicObjects()
{
	this->graphicsObjects.reset(ur_null);

	return Result(Success);
}

Result GPUWorkGraphsRealm::InitializeCanvasObjects()
{
#if (GPUWORKGRAPH_SAMPLE)
	return Result(Success);
#else

	Result res = Success;
	GrafSystem* grafSystem = this->GetGrafRenderer()->GetGrafSystem();
	GrafDevice* grafDevice = this->GetGrafRenderer()->GetGrafDevice();
	ur_uint canvasWidth = this->GetCanvasWidth();
	ur_uint canvasHeight = this->GetCanvasHeight();
	this->graphicsObjects->canvas.reset(new CanvasObjects());

	// gbuffer RT(s)

	ur_uint rtFrameCount = 1 + RTHistorySize;
	ur_uint rtImageCommonFlags = ur_uint(GrafImageUsageFlag::TransferDst) | ur_uint(GrafImageUsageFlag::ShaderRead);
	ur_uint rtImageCountWithHistory = RTImageCount * rtFrameCount;
	this->graphicsObjects->canvas->rtImages.resize(rtImageCountWithHistory);
	for (ur_uint imageIdx = 0; imageIdx < rtImageCountWithHistory; ++imageIdx)
	{
		const RTImageDesc& rtImageDesc = g_RTImageDesc[imageIdx % RTImageCount];
		ur_bool isHistoryImage = (imageIdx < RTImageCount);
		if (isHistoryImage && !rtImageDesc.HasHistory)
			continue;

		std::unique_ptr<GrafImage>& rtImage = this->graphicsObjects->canvas->rtImages[imageIdx];
		res = grafSystem->CreateImage(rtImage);
		if (Failed(res))
			break;

		GrafImageDesc grafImageDesc = {
			GrafImageType::Tex2D,
			rtImageDesc.Format,
			ur_uint3(canvasWidth, canvasHeight, 1), 1,
			ur_uint(rtImageDesc.Usage) | rtImageCommonFlags,
			ur_uint(GrafDeviceMemoryFlag::GpuLocal)
		};
		res = rtImage->Initialize(grafDevice, { grafImageDesc });
		if (Failed(res))
			break;
	}
	if (Failed(res))
	{
		LogError("GPUWorkGraphsRealm::InitializeCanvasObjects: failed to initialize RT image(s)");
		this->graphicsObjects->canvas.reset();
	}

	this->graphicsObjects->canvas->renderTargets.resize(rtFrameCount);
	for (ur_uint frameIdx = 0; frameIdx < rtFrameCount; ++frameIdx)
	{
		std::unique_ptr<GrafRenderTarget>& renderTarget = this->graphicsObjects->canvas->renderTargets[frameIdx];
		res = grafSystem->CreateRenderTarget(renderTarget);
		if (Failed(res))
			break;

		GrafImage* rtImages[RTImageCount];
		for (ur_uint imageIdx = 0; imageIdx < RTImageCount; ++imageIdx)
		{
			rtImages[imageIdx] = this->graphicsObjects->canvas->rtImages[imageIdx + (g_RTImageDesc[imageIdx].HasHistory ? frameIdx * RTImageCount : 0)].get();
		}

		GrafRenderTarget::InitParams grafRTParams = {
			this->graphicsObjects->gbufferRenderPass.get(),
			rtImages,
			RTImageCount
		};
		res = renderTarget->Initialize(grafDevice, grafRTParams);
		if (Failed(res))
			break;
	}
	if (Failed(res))
	{
		LogError("GPUWorkGraphsRealm::InitializeCanvasObjects: failed to initialize render target(s)");
		this->graphicsObjects->canvas.reset();
	}

	// direct rendering RTs (color image from swap chain + depth from gbuffer)

	ur_uint canvasImageCount = this->GetGrafRenderer()->GetGrafCanvas()->GetSwapChainImageCount();
	ur_uint depthImageCount = 1 + (g_RTImageDesc[RTImage_Depth].HasHistory ? RTHistorySize : 0);
	ur_uint directRTCount = depthImageCount * canvasImageCount;
	this->graphicsObjects->canvas->directRenderTargets.resize(directRTCount);
	for (ur_uint depthIdx = 0; depthIdx < depthImageCount; ++depthIdx)
	{
		for (ur_uint canvasIdx = 0; canvasIdx < canvasImageCount; ++canvasIdx)
		{
			std::unique_ptr<GrafRenderTarget>& renderTarget = this->graphicsObjects->canvas->directRenderTargets[canvasIdx + depthIdx * canvasImageCount];
			res = grafSystem->CreateRenderTarget(renderTarget);
			if (Failed(res))
				break;

			GrafImage* rtImages[] = {
				this->GetGrafRenderer()->GetGrafCanvas()->GetSwapChainImage(canvasIdx),
				this->graphicsObjects->canvas->rtImages[RTImage_Depth + RTImageCount * depthIdx].get()
			};

			GrafRenderTarget::InitParams grafRTParams = {
				this->graphicsObjects->directRenderPass.get(),
				rtImages,
				RTImageCount
			};
			res = renderTarget->Initialize(grafDevice, grafRTParams);
			if (Failed(res))
				break;
		}
		if (Failed(res))
			break;
	}
	if (Failed(res))
	{
		LogError("GPUWorkGraphsRealm::InitializeCanvasObjects: failed to initialize direct render target(s)");
		this->graphicsObjects->canvas.reset();
	}

	return res;
#endif
}

Result GPUWorkGraphsRealm::SafeDeleteCanvasObjects(GrafCommandList* commandList)
{
#if (GPUWORKGRAPH_SAMPLE)
	return Result(Success);
#else

	CanvasObjects* canvasObjects = this->graphicsObjects->canvas.release();
	if (canvasObjects != ur_null)
	{
		this->GetGrafRenderer()->AddCommandListCallback(commandList, {}, [canvasObjects](GrafCallbackContext& ctx) -> Result
		{
			delete canvasObjects;
			return Result(Success);
		});
	}

	return Result(Success);
#endif
}

Result GPUWorkGraphsRealm::Update(const UpdateContext& updateContext)
{
#if (GPUWORKGRAPH_SAMPLE)
	return Result(Success);
#else
	return ProceduralUpdate(updateContext);
#endif
}

Result GPUWorkGraphsRealm::Render(const RenderContext& renderContext)
{
#if (GPUWORKGRAPH_SAMPLE)
	return SampleRender(renderContext);
#else
	return ProceduralRender(renderContext);
#endif
}

Result GPUWorkGraphsRealm::DisplayImgui()
{
#if (GPUWORKGRAPH_SAMPLE)
	return SampleDisplayImgui();
#else
	return ProceduralDisplayImgui();
#endif
}

#if (GPUWORKGRAPH_SAMPLE)

Result GPUWorkGraphsRealm::SampleRender(const RenderContext& renderContext)
{
	if (ur_null == this->graphicsObjects.get())
		return Result(NotInitialized);

	// readback work graph output
	if (this->GetGrafRenderer()->GetFrameIdx() > 0)
	{
		GrafFenceState readbackFenceState;
		this->graphicsObjects->workGraphReadbackFence->GetState(readbackFenceState);
		if (GrafFenceState::Signaled == readbackFenceState)
		{
			this->graphicsObjects->workGraphReadbackData.resize(this->graphicsObjects->workGraphDataBuffer->GetDesc().SizeInBytes);
			ur_byte* readbackDstPtr = this->graphicsObjects->workGraphReadbackData.data();
			this->graphicsObjects->workGraphReadbackBuffer->Read(readbackDstPtr);
		}
		else if (GrafFenceState::Reset == readbackFenceState)
			return Result(Success); // do not dispatch another work graph till readback is done
	}

	if (this->GetGrafRenderer()->GetFrameIdx() % 256 != 0)
		return Result(Success); // slow down update

	// prepare resources
	renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->workGraphDataBuffer.get(), GrafBufferState::Current, GrafBufferState::ComputeReadWrite);

	// update descriptor table
	GrafDescriptorTable* workGraphFrameTable = this->graphicsObjects->workGraphDescTable->GetFrameObject();
	workGraphFrameTable->SetRWBuffer(g_UAVDescriptor, this->graphicsObjects->workGraphDataBuffer.get());

	// bind pipeline
	renderContext.CommandList->BindWorkGraphPipeline(this->graphicsObjects->workGraphPipeline.get());
	renderContext.CommandList->BindWorkGraphDescriptorTable(workGraphFrameTable, this->graphicsObjects->workGraphPipeline.get());

	// generate graph inputs
	std::vector<entryRecord> graphInputData;
	UINT numRecords = 4;
	graphInputData.resize(numRecords);
	for (UINT recordIndex = 0; recordIndex < numRecords; recordIndex++)
	{
		graphInputData[recordIndex].gridSize = recordIndex + 1;
		graphInputData[recordIndex].recordIndex = recordIndex;
	}

	// dispatch work graph
	renderContext.CommandList->DispatchGraph(0, (ur_byte*)graphInputData.data(), (ur_uint)graphInputData.size(), sizeof(entryRecord));

	// schedule readback
	renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->workGraphDataBuffer.get(), GrafBufferState::Current, GrafBufferState::TransferSrc);
	renderContext.CommandList->Copy(this->graphicsObjects->workGraphDataBuffer.get(), this->graphicsObjects->workGraphReadbackBuffer.get());
	this->graphicsObjects->workGraphReadbackFence->SetState(GrafFenceState::Reset);

	return Result(Success);
}

Result GPUWorkGraphsRealm::SampleDisplayImgui()
{
	std::string titleString;
	WstringToString(this->GetCanvas()->GetTitle(), titleString);
	ImGui::Begin(titleString.c_str());
	if (ImGui::CollapsingHeader("SampleWorkGraph"))
	{
		std::stringstream outputStr;
		ur_uint32* dataPtr = (ur_uint32*)this->graphicsObjects->workGraphReadbackData.data();
		for (ur_uint i = 0; i < 16; ++i)
		{
			if (i % 8 == 0) outputStr << "\n";
			outputStr << *(ur_uint32*)dataPtr++;
			if (i + 1 < 16) outputStr << ", ";
		}
		ImGui::TextWrapped("Output: %s", outputStr.str().c_str());
	}
	ImGui::End();

	return Result(Success);
}

#else

Result GPUWorkGraphsRealm::ProceduralUpdate(const UpdateContext& renderContext)
{
	this->camera.SetAspectRatio((float)this->GetCanvasWidth() / this->GetCanvasHeight());
	ur_float distToObj = (camera.GetPosition() - this->proceduralObject.position).Length() - this->proceduralObject.extent;
	this->cameraControl.SetSpeed(std::max(5.0f, distToObj * 0.5f));
	this->cameraControl.Update();

	return Result(Success);
}

Result GPUWorkGraphsRealm::ProceduralRender(const RenderContext& renderContext)
{
	// work graph pass
	{
		GrafUtils::ScopedDebugLabel label(renderContext.CommandList, "WorkGraphPass", DebugLabelPass);

		// prepare resources
		renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->partitionDataBuffer.get(), GrafBufferState::Current, GrafBufferState::ComputeReadWrite);

		// update constants
		ProceduralConsts proceduralConsts;
		proceduralConsts.RootPosition = this->proceduralObject.position;
		proceduralConsts.RootExtent = this->proceduralObject.extent;
		proceduralConsts.RefinementPoint = this->camera.GetPosition();
		proceduralConsts.RefinementDistanceFactor = 1.0f;
		Allocation proceduralConstsAlloc = this->GetGrafRenderer()->GetDynamicConstantBufferAllocation(sizeof(ProceduralConsts));
		GrafBuffer* dynamicCB = this->GetGrafRenderer()->GetDynamicConstantBuffer();
		dynamicCB->Write((ur_byte*)&proceduralConsts, sizeof(ProceduralConsts), 0, proceduralConstsAlloc.Offset);

		// update descriptor table
		GrafDescriptorTable* proceduralGraphFrameTable = this->graphicsObjects->proceduralGraphDescTable->GetFrameObject();
		proceduralGraphFrameTable->SetConstantBuffer(g_ProceduralConstsDescriptor, dynamicCB, proceduralConstsAlloc.Offset, proceduralConstsAlloc.Size);
		proceduralGraphFrameTable->SetRWBuffer(g_PartitionDataDescriptor, this->graphicsObjects->partitionDataBuffer.get());

		// bind pipeline
		renderContext.CommandList->BindWorkGraphPipeline(this->graphicsObjects->proceduralGraphPipeline.get());
		renderContext.CommandList->BindWorkGraphDescriptorTable(proceduralGraphFrameTable, this->graphicsObjects->proceduralGraphPipeline.get());

		// dispatch partition work graph
		renderContext.CommandList->DispatchGraph(0, ur_null, 1, 0);

		// readback partition data
		if (!this->graphicsObjects->readbackPending)
		{
			renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->partitionDataBuffer.get(), GrafBufferState::Current, GrafBufferState::TransferSrc);
			renderContext.CommandList->Copy(this->graphicsObjects->partitionDataBuffer.get(), this->graphicsObjects->readbackBuffer.get());
			this->graphicsObjects->readbackFence->SetState(GrafFenceState::Reset);
			this->graphicsObjects->readbackPending = true;
		}
		else
		{
			GrafFenceState readbackFenceState;
			this->graphicsObjects->readbackFence->GetState(readbackFenceState);
			if (GrafFenceState::Signaled == readbackFenceState)
			{
				this->graphicsObjects->readbackData.resize(this->graphicsObjects->readbackBuffer->GetDesc().SizeInBytes);
				ur_byte* readbackDstPtr = this->graphicsObjects->readbackData.data();
				ur_byte* readbackNodesCounterPtr = readbackDstPtr + PartitionDataNodesCounterOfs;
				ur_byte* readbackNodesDataPtr = readbackDstPtr + PartitionDataNodesOfs;
				this->graphicsObjects->readbackBuffer->Read(readbackDstPtr);
				this->graphicsObjects->readbackPending = false;
			}
		}
	}

	// direct render pass
	{
		GrafUtils::ScopedDebugLabel label(renderContext.CommandList, "DirectRenderPass", DebugLabelPass);

		ur_uint canvasImageCount = this->GetGrafRenderer()->GetGrafCanvas()->GetSwapChainImageCount();
		ur_uint canvasImageId = this->GetGrafRenderer()->GetGrafCanvas()->GetCurrentImageId();
		ur_uint depthImageId = (g_RTImageDesc[RTImage_Depth].HasHistory ?  this->GetGrafRenderer()->GetFrameIdx() % (1 + RTHistorySize) : 0);
		GrafRenderTarget* directRT = this->graphicsObjects->canvas->directRenderTargets[canvasImageId + depthImageId * canvasImageCount].get();
		renderContext.CommandList->ImageMemoryBarrier(directRT->GetImage(0), GrafImageState::Current, GrafImageState::ColorWrite);
		renderContext.CommandList->ImageMemoryBarrier(directRT->GetImage(1), GrafImageState::Current, GrafImageState::DepthStencilWrite);
		renderContext.CommandList->BeginRenderPass(this->graphicsObjects->directRenderPass.get(), directRT);

		// render partition structure

		renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->partitionDataBuffer.get(), GrafBufferState::Current, GrafBufferState::ShaderRead);

		// update constants
		SceneRenderConsts sceneRenderConsts;
		sceneRenderConsts.ViewProj = this->camera.GetViewProj();
		Allocation sceneRenderConstsAlloc = this->GetGrafRenderer()->GetDynamicConstantBufferAllocation(sizeof(SceneRenderConsts));
		GrafBuffer* dynamicCB = this->GetGrafRenderer()->GetDynamicConstantBuffer();
		dynamicCB->Write((ur_byte*)&sceneRenderConsts, sizeof(SceneRenderConsts), 0, sceneRenderConstsAlloc.Offset);

		// update descriptor table
		GrafDescriptorTable* proceduralRenderFrameTable = this->graphicsObjects->proceduralRenderDescTable->GetFrameObject();
		proceduralRenderFrameTable->SetConstantBuffer(g_SceneRenderConstsDescriptor, dynamicCB, sceneRenderConstsAlloc.Offset, sceneRenderConstsAlloc.Size);
		proceduralRenderFrameTable->SetRWBuffer(g_PartitionDataDescriptor, this->graphicsObjects->partitionDataBuffer.get());

		renderContext.CommandList->BindPipeline(this->graphicsObjects->proceduralRenderPipeline.get());
		renderContext.CommandList->BindDescriptorTable(this->graphicsObjects->proceduralRenderDescTable->GetFrameObject(), this->graphicsObjects->proceduralRenderPipeline.get());
		// TODO: DrawIndirect
		// TEMP: constant instance count
		renderContext.CommandList->Draw(12, 1024, 0, 0);

		renderContext.CommandList->EndRenderPass();
	}

	return Result(Success);
}

Result GPUWorkGraphsRealm::ProceduralDisplayImgui()
{
	std::string titleString;
	WstringToString(this->GetCanvas()->GetTitle(), titleString);
	ImGui::Begin(titleString.c_str());
	// TODO
	ImGui::End();

	return Result(Success);
}

#endif