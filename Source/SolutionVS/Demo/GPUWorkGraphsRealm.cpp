
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

GPUWorkGraphsRealm::GPUWorkGraphsRealm()
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

	return RenderRealm::Initialize(realmParams);
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
				{ "ProceduralEntryNode", GrafShaderType::Node }
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
			bufferParams.BufferDesc.SizeInBytes = PartitionDataSizeMax;
			res = this->graphicsObjects->partitionDataBuffer->Initialize(grafDevice, bufferParams);

			// upload initial data
			//std::vector<ur_uint32> initialData(1024, 0);
			//this->GetGrafRenderer()->Upload((ur_byte*)initialData.data(), this->graphicsObjects->partitionDataBuffer.get(), bufferParams.BufferDesc.SizeInBytes);
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

Result GPUWorkGraphsRealm::Update(const UpdateContext& updateContext)
{
	return Result(Success);
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

Result GPUWorkGraphsRealm::ProceduralRender(const RenderContext& renderContext)
{
	return Result(Success);
}

Result GPUWorkGraphsRealm::ProceduralDisplayImgui()
{
	return Result(Success);
}

#endif