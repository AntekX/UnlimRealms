
#include "stdafx.h"
#include "RayTracingSandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Core/Math.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/GrafRenderer.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

int RayTracingSandboxApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"RayTracing Sandbox"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));
	ur_uint canvasWidth = realm.GetCanvas()->GetClientBound().Width();
	ur_uint canvasHeight = realm.GetCanvas()->GetClientBound().Height();
	ur_bool canvasValid = (realm.GetCanvas()->GetClientBound().Area() > 0);

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// create GRAF renderer
	GrafRenderer *grafRenderer = realm.AddComponent<GrafRenderer>(realm);
	Result res = Success;
	do
	{
		// create GRAF system
		std::unique_ptr<GrafSystem> grafSystem(new GrafSystemVulkan(realm));
		res = grafSystem->Initialize(realm.GetCanvas());
		if (Failed(res)) break;

		// initialize renderer
		GrafRenderer::InitParams grafRendererParams = GrafRenderer::InitParams::Default;
		grafRendererParams.DeviceId = grafSystem->GetRecommendedDeviceId();
		grafRendererParams.CanvasParams = GrafCanvas::InitParams::Default;
		grafRendererParams.CanvasParams.PresentMode = GrafPresentMode::Immediate;
		res = grafRenderer->Initialize(std::move(grafSystem), grafRendererParams);
		if (Failed(res)) break;

	} while (false);
	if (Failed(res))
	{
		realm.RemoveComponent<GrafRenderer>();
		grafRenderer = ur_null;
		realm.GetLog().WriteLine("RayTracingSandboxApp: failed to initialize GrafRenderer", Log::Error);
	}

	// initialize sample GRAF objects
	std::unique_ptr<GrafImage> grafRTImageColor;
	std::unique_ptr<GrafImage> grafRTImageDepth;
	std::unique_ptr<GrafRenderTarget> grafColorDepthTarget;
	std::unique_ptr<GrafRenderPass> grafColorDepthPass;
	std::vector<std::unique_ptr<GrafCommandList>> grafMainCmdList;
	auto& deinitializeGrafRenderTargetObjects = [&](GrafCommandList* deferredDestroyCmdList = ur_null) -> void
	{
		if (deferredDestroyCmdList != ur_null)
		{
			GrafRenderTarget *grafPrevRT = grafColorDepthTarget.release();
			GrafImage *grafPrevRTImageColor = grafRTImageColor.release();
			GrafImage *grafPrevRTImageDepth = grafRTImageDepth.release();
			grafRenderer->AddCommandListCallback(deferredDestroyCmdList, {},
				[grafPrevRT, grafPrevRTImageColor, grafPrevRTImageDepth](GrafCallbackContext& ctx) -> Result
			{
				delete grafPrevRT;
				delete grafPrevRTImageColor;
				delete grafPrevRTImageDepth;
				return Result(Success);
			});
		}
		grafColorDepthTarget.reset();
		grafRTImageColor.reset();
		grafRTImageDepth.reset();
	};
	auto& deinitializeGrafFrameObjects = [&]() -> void
	{
		grafMainCmdList.clear();
	};
	auto& deinitializeGrafObjects = [&]() -> void
	{
		// order matters!
		deinitializeGrafRenderTargetObjects();
		deinitializeGrafFrameObjects();
		grafColorDepthPass.reset();
	};
	Result grafRes = NotInitialized;
	auto& initializeGrafObjects = [&]() -> void
	{
		if (ur_null == grafRenderer) return;
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// color & depth render pass
		grafRes = grafSystem->CreateRenderPass(grafColorDepthPass);
		if (Failed(grafRes)) return;
		GrafRenderPassImageDesc grafColorDepthPassImages[] = {
			{ // color
				GrafFormat::B8G8R8A8_UNORM,
				GrafImageState::Undefined, GrafImageState::ColorWrite,
				GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
				GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
			},
			{ // depth
				GrafFormat::D24_UNORM_S8_UINT,
				GrafImageState::Undefined, GrafImageState::DepthStencilWrite,
				GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
				GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
			}
		};
		grafRes = grafColorDepthPass->Initialize(grafDevice, { grafColorDepthPassImages, ur_array_size(grafColorDepthPassImages) });
		if (Failed(grafRes)) return;
	};
	auto& initializeGrafFrameObjects = [&]() -> void
	{
		if (Failed(grafRes)) return;
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// match number of recorded (in flight) frames to the GrafRenderer
		ur_uint frameCount = grafRenderer->GetRecordedFrameCount();

		// command lists (per frame)
		grafMainCmdList.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			grafRes = grafSystem->CreateCommandList(grafMainCmdList[iframe]);
			if (Failed(grafRes)) break;
			grafRes = grafMainCmdList[iframe]->Initialize(grafDevice);
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;
	};
	auto& initializeGrafRenderTargetObjects = [&]() -> void
	{
		if (Failed(grafRes)) return;
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// color target image
		grafRes = grafSystem->CreateImage(grafRTImageColor);
		if (Failed(grafRes)) return;
		GrafImageDesc grafRTImageColorDesc = {
			GrafImageType::Tex2D,
			GrafFormat::B8G8R8A8_UNORM,
			ur_uint3(canvasWidth, canvasHeight, 1), 1,
			ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::ShaderReadWrite) | ur_uint(GrafImageUsageFlag::TransferSrc),
			ur_uint(GrafDeviceMemoryFlag::GpuLocal)
		};
		grafRes = grafRTImageColor->Initialize(grafDevice, { grafRTImageColorDesc });
		if (Failed(grafRes)) return;

		// depth target image
		grafRes = grafSystem->CreateImage(grafRTImageDepth);
		if (Failed(grafRes)) return;
		GrafImageDesc grafRTImageDepthDesc = {
			GrafImageType::Tex2D,
			GrafFormat::D24_UNORM_S8_UINT,
			ur_uint3(canvasWidth, canvasHeight, 1), 1,
			ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget),
			ur_uint(GrafDeviceMemoryFlag::GpuLocal)
		};
		grafRes = grafRTImageDepth->Initialize(grafDevice, { grafRTImageDepthDesc });
		if (Failed(grafRes)) return;

		// initialize render target
		grafRes = grafSystem->CreateRenderTarget(grafColorDepthTarget);
		if (Failed(grafRes)) return;
		GrafImage* grafColorDepthTargetImages[] = {
			grafRTImageColor.get(),
			grafRTImageDepth.get()
		};
		GrafRenderTarget::InitParams grafColorDepthTargetParams = {
			grafColorDepthPass.get(),
			grafColorDepthTargetImages,
			ur_array_size(grafColorDepthTargetImages)
		};
		grafRes = grafColorDepthTarget->Initialize(grafDevice, grafColorDepthTargetParams);
		if (Failed(grafRes)) return;
	};
	initializeGrafObjects();
	initializeGrafFrameObjects();
	initializeGrafRenderTargetObjects();
	if (Failed(grafRes))
	{
		deinitializeGrafObjects();
		realm.GetLog().WriteLine("RayTracingSandboxApp: failed to initialize one or more graphics objects", Log::Error);
	}

	// initialize ImguiRender
	ImguiRender* imguiRender = ur_null;
	if (grafRenderer != ur_null)
	{
		imguiRender = realm.AddComponent<ImguiRender>(realm);
		Result res = imguiRender->Init();
		if (Failed(res))
		{
			realm.RemoveComponent<ImguiRender>();
			imguiRender = ur_null;
			realm.GetLog().WriteLine("RayTracingSandboxApp: failed to initialize ImguiRender", Log::Error);
		}
	}

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	camera.SetPosition(ur_float3(0.0f, 0.0f, -10.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

	// animated clear color test
	static const ur_float clearColorDelay = 2.0f;
	static const ur_float4 clearColors[] = {
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
	};
	ur_uint crntColorId = 0;
	ur_uint nextColorId = (crntColorId + 1) % ur_array_size(clearColors);
	ur_float crntColorWeight = 0.0f;
	ur_float4 crntClearColor(0.0f);

	// ray tracing scene
	class RayTracingScene : public RealmEntity
	{
	public:

		GrafRenderer* grafRenderer;
		std::unique_ptr<GrafBuffer> vertexBuffer;
		std::unique_ptr<GrafBuffer> indexBuffer;
		std::unique_ptr<GrafBuffer> instanceBuffer;
		std::unique_ptr<GrafAccelerationStructure> accelerationStructureBL;
		std::unique_ptr<GrafAccelerationStructure> accelerationStructureTL;
		std::unique_ptr<GrafDescriptorTableLayout> bindingTableLayout;
		std::vector<std::unique_ptr<GrafDescriptorTable>> bindingTables;
		std::unique_ptr<GrafShader> shaderRayGen;
		std::unique_ptr<GrafShader> shaderClosestHit;
		std::unique_ptr<GrafShader> shaderMiss;
		std::unique_ptr<GrafRayTracingPipeline> pipelineState;
		std::unique_ptr<GrafBuffer> shaderHandlesBuffer;
		GrafStridedBufferRegionDesc rayGenShaderTable;
		GrafStridedBufferRegionDesc missShaderTable;
		GrafStridedBufferRegionDesc hitShaderTable;

		RayTracingScene(Realm& realm) : RealmEntity(realm), grafRenderer(ur_null) {}
		~RayTracingScene()
		{
			if (ur_null == grafRenderer)
				return;

			grafRenderer->SafeDelete(vertexBuffer.release());
			grafRenderer->SafeDelete(indexBuffer.release());
			grafRenderer->SafeDelete(instanceBuffer.release());
			grafRenderer->SafeDelete(accelerationStructureBL.release());
			grafRenderer->SafeDelete(accelerationStructureTL.release());
			grafRenderer->SafeDelete(bindingTableLayout.release());
			for (auto& bindingTable : bindingTables)
			{
				grafRenderer->SafeDelete(bindingTable.release());
			}
			grafRenderer->SafeDelete(shaderRayGen.release());
			grafRenderer->SafeDelete(shaderClosestHit.release());
			grafRenderer->SafeDelete(shaderMiss.release());
			grafRenderer->SafeDelete(pipelineState.release());
			grafRenderer->SafeDelete(shaderHandlesBuffer.release());
		}

		void Initialize()
		{
			this->grafRenderer = this->GetRealm().GetComponent<GrafRenderer>();
			if (ur_null == grafRenderer)
				return;

			GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = grafRenderer->GetGrafDevice();
			const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());

			// sample geomtry data

			struct VertexSample
			{
				ur_float3 pos;
			};
			const ur_float vs = 0.5f;
			VertexSample sampleVertices[] = {
				{ {-1.0f * vs,-1.0f * vs, 1.0f } },
				{ { 1.0f * vs,-1.0f * vs, 1.0f } },
				{ { 0.0f * vs, 1.0f * vs, 1.0f } }
			};

			typedef ur_uint32 IndexSample;
			IndexSample sampleIndices[] = {
				0, 1, 2
			};

			GrafBuffer::InitParams sampleVBParams;
			sampleVBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			sampleVBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			sampleVBParams.BufferDesc.SizeInBytes = sizeof(sampleVertices);
			grafSystem->CreateBuffer(this->vertexBuffer);
			this->vertexBuffer->Initialize(grafDevice, sampleVBParams);
			this->vertexBuffer->Write((ur_byte*)sampleVertices);

			GrafBuffer::InitParams sampleIBParams;
			sampleIBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			sampleIBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			sampleIBParams.BufferDesc.SizeInBytes = sizeof(sampleIndices);
			grafSystem->CreateBuffer(this->indexBuffer);
			this->indexBuffer->Initialize(grafDevice, sampleIBParams);
			this->indexBuffer->Write((ur_byte*)sampleIndices);

			// initiaize bottom level acceleration structure container

			GrafAccelerationStructureGeometryDesc accelStructGeomDescBL = {};
			accelStructGeomDescBL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
			accelStructGeomDescBL.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
			accelStructGeomDescBL.IndexType = GrafIndexType::UINT32;
			accelStructGeomDescBL.PrimitiveCountMax = 128;
			accelStructGeomDescBL.VertexCountMax = 8;
			accelStructGeomDescBL.TransformsEnabled = false;

			GrafAccelerationStructure::InitParams accelStructParamsBL = {};
			accelStructParamsBL.StructureType = GrafAccelerationStructureType::BottomLevel;
			accelStructParamsBL.BuildFlags = GrafAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlag::PreferFastTrace);
			accelStructParamsBL.Geometry = &accelStructGeomDescBL;
			accelStructParamsBL.GeometryCount = 1;

			grafSystem->CreateAccelerationStructure(this->accelerationStructureBL);
			this->accelerationStructureBL->Initialize(grafDevice, accelStructParamsBL);

			// build bottom level acceleration structure for sample geometry

			GrafAccelerationStructureTrianglesData sampleTrianglesData = {};
			sampleTrianglesData.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
			sampleTrianglesData.VertexStride = sizeof(VertexSample);
			sampleTrianglesData.VerticesDeviceAddress = this->vertexBuffer->GetDeviceAddress();
			sampleTrianglesData.IndexType = GrafIndexType::UINT32;
			sampleTrianglesData.IndicesDeviceAddress = this->indexBuffer->GetDeviceAddress();

			GrafAccelerationStructureGeometryData sampleGeometryDataBL = {};
			sampleGeometryDataBL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
			sampleGeometryDataBL.GeometryFlags = ur_uint(GrafAccelerationStructureGeometryFlag::Opaque);
			sampleGeometryDataBL.TrianglesData = &sampleTrianglesData;
			sampleGeometryDataBL.PrimitiveCount = ur_array_size(sampleVertices) / 3;

			GrafCommandList* cmdListBuildAccelStructBL = grafRenderer->GetTransientCommandList();
			cmdListBuildAccelStructBL->Begin();
			cmdListBuildAccelStructBL->BuildAccelerationStructure(this->accelerationStructureBL.get(), &sampleGeometryDataBL, 1);
			cmdListBuildAccelStructBL->End();
			grafDevice->Record(cmdListBuildAccelStructBL);
			grafDevice->Submit();
			grafDevice->WaitIdle();

			// initiaize top level acceleration structure container

			GrafAccelerationStructureGeometryDesc accelStructGeomDescTL = {};
			accelStructGeomDescTL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
			accelStructGeomDescTL.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
			accelStructGeomDescTL.IndexType = GrafIndexType::UINT32;
			accelStructGeomDescTL.PrimitiveCountMax = 128;
			accelStructGeomDescTL.VertexCountMax = 8;
			accelStructGeomDescTL.TransformsEnabled = false;

			GrafAccelerationStructure::InitParams accelStructParamsTL = {};
			accelStructParamsTL.StructureType = GrafAccelerationStructureType::TopLevel;
			accelStructParamsTL.BuildFlags = GrafAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlag::PreferFastTrace);
			accelStructParamsTL.Geometry = &accelStructGeomDescTL;
			accelStructParamsTL.GeometryCount = 1;

			grafSystem->CreateAccelerationStructure(this->accelerationStructureTL);
			this->accelerationStructureTL->Initialize(grafDevice, accelStructParamsTL);

			// build top level acceleration structure

			GrafAccelerationStructureInstance sampleInstances[] = {
				{
					{
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f
					},
					0, 0xff, 0, (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable)),
					this->accelerationStructureBL->GetDeviceAddress()
				}
			};

			GrafBuffer::InitParams sampleInstanceParams;
			sampleInstanceParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			sampleInstanceParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			sampleInstanceParams.BufferDesc.SizeInBytes = sizeof(sampleInstances);
			grafSystem->CreateBuffer(this->instanceBuffer);
			this->instanceBuffer->Initialize(grafDevice, sampleInstanceParams);
			this->instanceBuffer->Write((ur_byte*)sampleInstances);

			GrafAccelerationStructureInstancesData sampleInstancesData = {};
			sampleInstancesData.IsPointersArray = false;;
			sampleInstancesData.DeviceAddress = this->instanceBuffer->GetDeviceAddress();

			GrafAccelerationStructureGeometryData sampleGeometryDataTL = {};
			sampleGeometryDataTL.GeometryType = GrafAccelerationStructureGeometryType::Instances;
			sampleGeometryDataTL.GeometryFlags = ur_uint(GrafAccelerationStructureGeometryFlag::Opaque);
			sampleGeometryDataTL.InstancesData = &sampleInstancesData;
			sampleGeometryDataTL.PrimitiveCount = ur_array_size(sampleInstances);

			GrafCommandList* cmdListBuildAccelStructTL = grafRenderer->GetTransientCommandList();
			cmdListBuildAccelStructTL->Begin();
			cmdListBuildAccelStructTL->BuildAccelerationStructure(this->accelerationStructureTL.get(), &sampleGeometryDataTL, 1);
			cmdListBuildAccelStructTL->End();
			grafDevice->Record(cmdListBuildAccelStructTL);
			grafDevice->Submit();
			grafDevice->WaitIdle();

			// shaders

			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleRaygen", GrafShaderType::RayGen, this->shaderRayGen);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleMiss", GrafShaderType::Miss, this->shaderMiss);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleClosestHit", GrafShaderType::ClosestHit, this->shaderClosestHit);

			// global shader bindings

			GrafDescriptorRangeDesc bindingTableLayoutRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
				{ GrafDescriptorType::AccelerationStructure, 0, 1 },
				{ GrafDescriptorType::RWTexture, 0, 1 }
			};
			GrafDescriptorTableLayoutDesc bindingTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::AllRayTracing),
				bindingTableLayoutRanges, ur_array_size(bindingTableLayoutRanges)
			};
			grafSystem->CreateDescriptorTableLayout(this->bindingTableLayout);
			this->bindingTableLayout->Initialize(grafDevice, { bindingTableLayoutDesc });
			this->bindingTables.resize(grafRenderer->GetRecordedFrameCount());
			for (auto& bindingTable : this->bindingTables)
			{
				grafSystem->CreateDescriptorTable(bindingTable);
				bindingTable->Initialize(grafDevice, { this->bindingTableLayout.get() });
			}

			// pipeline

			GrafShader* shaderStages[] = {
				this->shaderRayGen.get(),
				this->shaderMiss.get(),
				this->shaderClosestHit.get()
			};

			GrafRayTracingShaderGroupDesc shaderGroups[3];
			shaderGroups[0] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[0].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[0].GeneralShaderIdx = 0; // shaderRayGen
			shaderGroups[1] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[1].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[1].GeneralShaderIdx = 1; // shaderMiss
			shaderGroups[2] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[2].Type = GrafRayTracingShaderGroupType::TrianglesHit;
			shaderGroups[2].ClosestHitShaderIdx = 2; // shaderClosestHit

			GrafDescriptorTableLayout* bindingLayouts[] = {
				this->bindingTableLayout.get()
			};
			GrafRayTracingPipeline::InitParams pipelineParams = GrafRayTracingPipeline::InitParams::Default;
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.ShaderGroups = shaderGroups;
			pipelineParams.ShaderGroupCount = ur_array_size(shaderGroups);
			pipelineParams.DescriptorTableLayouts = bindingLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(bindingLayouts);
			pipelineParams.MaxRecursionDepth = 8;
			grafSystem->CreateRayTracingPipeline(this->pipelineState);
			this->pipelineState->Initialize(grafDevice, pipelineParams);

			// shader group handles buffer

			ur_size shaderGroupHandleSize = grafDeviceDesc->RayTracing.ShaderGroupHandleSize;
			ur_size shaderBufferSize = pipelineParams.ShaderGroupCount * shaderGroupHandleSize;
			GrafBuffer::InitParams shaderBufferParams;
			shaderBufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::TransferSrc);
			shaderBufferParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			shaderBufferParams.BufferDesc.SizeInBytes = shaderBufferSize;
			grafSystem->CreateBuffer(this->shaderHandlesBuffer);
			this->shaderHandlesBuffer->Initialize(grafDevice, shaderBufferParams);
			GrafRayTracingPipeline* rayTracingPipeline = this->pipelineState.get();
			this->shaderHandlesBuffer->Write([rayTracingPipeline, pipelineParams, shaderBufferParams](ur_byte* mappedDataPtr) -> Result
			{
				rayTracingPipeline->GetShaderGroupHandles(0, pipelineParams.ShaderGroupCount, shaderBufferParams.BufferDesc.SizeInBytes, mappedDataPtr);
				return Result(Success);
			});
			this->rayGenShaderTable = { this->shaderHandlesBuffer.get(), 0 * shaderGroupHandleSize, shaderGroupHandleSize, shaderGroupHandleSize };
			this->missShaderTable = { this->shaderHandlesBuffer.get(), 1 * shaderGroupHandleSize, shaderGroupHandleSize, shaderGroupHandleSize };
			this->hitShaderTable = { this->shaderHandlesBuffer.get(), 2 * shaderGroupHandleSize, shaderGroupHandleSize, shaderGroupHandleSize };
		}

		void Render(GrafCommandList* grafCmdList, GrafImage* grafTargetImage, const ur_float4 &clearColor, const ur_float4x4 &viewProj, const ur_float3 &cameraPos)
		{
			if (ur_null == this->grafRenderer)
				return;

			struct SceneConstants
			{
				ur_float4x4 viewProjInv;
				ur_float4 cameraPos;
				ur_float4 viewportSize;
				ur_float4 clearColor;
			} cb;
			ur_uint3 targetSize = grafTargetImage->GetDesc().Size;
			cb.viewProjInv = ur_float4x4::Identity;// todo: calculate inverse matrix
			cb.cameraPos = cameraPos;
			cb.viewportSize.x = (ur_float)targetSize.x;
			cb.viewportSize.y = (ur_float)targetSize.y;
			cb.viewportSize.z = 1.0f / cb.viewportSize.x;
			cb.viewportSize.w = 1.0f / cb.viewportSize.y;
			cb.clearColor = clearColor;
			GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
			Allocation dynamicCBAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SceneConstants));
			dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

			GrafDescriptorTable* descriptorTable = this->bindingTables[this->grafRenderer->GetCurrentFrameId()].get();
			descriptorTable->SetConstantBuffer(0, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
			descriptorTable->SetRWImage(0, grafTargetImage);
			descriptorTable->SetAccelerationStructure(0, this->accelerationStructureTL.get());

			grafCmdList->ImageMemoryBarrier(grafTargetImage, GrafImageState::Current, GrafImageState::RayTracingReadWrite);
			grafCmdList->BindRayTracingPipeline(this->pipelineState.get());
			grafCmdList->BindRayTracingDescriptorTable(descriptorTable, this->pipelineState.get());
			grafCmdList->DispatchRays(targetSize.x, targetSize.y, 1, &this->rayGenShaderTable, &this->missShaderTable, &this->hitShaderTable, ur_null);
		}
	};
	std::unique_ptr<RayTracingScene> rayTracingScene;
	if (grafRenderer != ur_null)
	{
		GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = grafRenderer->GetGrafDevice();
		const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());
		if (grafDeviceDesc != ur_null && grafDeviceDesc->RayTracing.RayTraceSupported)
		{
			rayTracingScene.reset(new RayTracingScene(realm));
			rayTracingScene->Initialize();
		};
	}

	// Main message loop:
	ClockTime timer = Clock::now();
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT && !realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::Escape))
	{
		// process messages first
		ClockTime timeBefore = Clock::now();
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) && msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// forward msg to WinInput system
			WinInput* winInput = static_cast<WinInput*>(realm.GetInput());
			winInput->ProcessMsg(msg);
		}
		if (msg.message == WM_QUIT)
			break;
		auto inputDelay = Clock::now() - timeBefore;
		timer += inputDelay;  // skip input delay

		canvasValid = (realm.GetCanvas()->GetClientBound().Area() > 0);
		if (!canvasValid)
			Sleep(60); // lower update frequency while minimized

		// update sub systems
		realm.GetInput()->Update();
		if (imguiRender != ur_null)
		{
			imguiRender->NewFrame();
		}
		cameraControl.Update();
		camera.SetAspectRatio((float)realm.GetCanvas()->GetClientBound().Width() / realm.GetCanvas()->GetClientBound().Height());

		// update frame
		auto updateFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void
		{
			// reset update progress fence
			ctx.progress = 0;

			// move timer
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f;  // to seconds

			// clear color animation
			crntColorWeight += elapsedTime / clearColorDelay;
			if (crntColorWeight >= 1.0f)
			{
				ur_float intPart;
				crntColorWeight = std::modf(crntColorWeight, &intPart);
				crntColorId = (crntColorId + ur_uint(intPart)) % ur_array_size(clearColors);
				nextColorId = (crntColorId + 1) % ur_array_size(clearColors);
			}
			crntClearColor = ur_float4::Lerp(clearColors[crntColorId], clearColors[nextColorId], crntColorWeight);
			ctx.progress = 1;
		});

		// draw frame
		if (grafRenderer != ur_null && canvasValid)
		{
			// update render target size
			if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() ||
				canvasHeight != realm.GetCanvas()->GetClientBound().Height())
			{
				// make sure prev RT objects are no longer used before destroying
				GrafCommandList* grafCmdListPrev = grafMainCmdList[grafRenderer->GetPrevFrameId()].get();
				deinitializeGrafRenderTargetObjects(grafCmdListPrev);
				// recreate RT objects for new canvas dimensions
				canvasWidth = realm.GetCanvas()->GetClientBound().Width();
				canvasHeight = realm.GetCanvas()->GetClientBound().Height();
				initializeGrafRenderTargetObjects();
			}

			// begin frame rendering
			GrafDevice *grafDevice = grafRenderer->GetGrafDevice();
			GrafCanvas *grafCanvas = grafRenderer->GetGrafCanvas();
			ur_uint frameIdx = grafRenderer->GetCurrentFrameId();
			grafRenderer->BeginFrame();

			auto drawFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void
			{
				// updateFrameJob->Wait(); // wait till update job is fully finished; WaitProgress can be used instead to wait for specific update stage to avoid stalling draw thread

				GrafCommandList* grafCmdListCrnt = grafMainCmdList[frameIdx].get();
				grafCmdListCrnt->Begin();

				GrafViewportDesc grafViewport = {};
				grafViewport.Width = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.x;
				grafViewport.Height = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.y;
				grafViewport.Near = 0.0f;
				grafViewport.Far = 1.0f;
				grafCmdListCrnt->SetViewport(grafViewport, true);
				grafCmdListCrnt->SetScissorsRect({ 0, 0, (ur_int)grafViewport.Width, (ur_int)grafViewport.Height });
				//grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::TransferDst);
				//grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), { 1.0f, 0.0f, 0.0f, 1.0f }); // clear swap chain image directly

				{ // color & depth render pass

					updateFrameJob->WaitProgress(1); // make sure update job is done to this point
					GrafClearValue rtClearValues[] = {
						reinterpret_cast<GrafClearValue&>(crntClearColor), // color
						{ 1.0f, 0.0f, 0.0f, 0.0f }, // depth & stencil
					};
					
					// ray tracing scene
					
					if (rayTracingScene != ur_null)
					{
						rayTracingScene->Render(grafCmdListCrnt, grafRTImageColor.get(), crntClearColor, camera.GetViewProj(), camera.GetPosition());
					}

					// copy RT color result to swap chain image

					grafCmdListCrnt->ImageMemoryBarrier(grafRTImageColor.get(), GrafImageState::Current, GrafImageState::TransferSrc);
					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::TransferDst);
					grafCmdListCrnt->Copy(grafRTImageColor.get(), grafCanvas->GetCurrentImage());
					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::Common);
				}

				{ // foreground color render pass (drawing directly into swap chain image)

					grafCmdListCrnt->BeginRenderPass(grafRenderer->GetCanvasRenderPass(), grafRenderer->GetCanvasRenderTarget());

					// draw GUI

					static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
					static bool showGUI = true;
					showGUI = (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
					if (showGUI)
					{
						ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
						ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
						ImGui::ShowMetricsWindow();

						grafRenderer->ShowImgui();

						imguiRender->Render(*grafCmdListCrnt);
					}

					grafCmdListCrnt->EndRenderPass();
				}

				// finalize current command list
				grafCmdListCrnt->End();
				grafDevice->Record(grafCmdListCrnt);
			});

			drawFrameJob->Wait();

			// submit command list(s) to device execution queue
			grafDevice->Submit();

			// present & move to next frame
			grafRenderer->EndFrameAndPresent();
		}
	}

	if (grafRenderer != ur_null)
	{
		// make sure there are no resources still used on gpu before destroying
		grafRenderer->GetGrafDevice()->WaitIdle();
	}

	// destroy application objects
	rayTracingScene.reset();

	// destroy sample GRAF objects
	deinitializeGrafObjects();

	// destroy realm objects
	realm.RemoveComponent<ImguiRender>();
	realm.RemoveComponent<GenericRender>();
	realm.RemoveComponent<GrafRenderer>();

	return 0;
}