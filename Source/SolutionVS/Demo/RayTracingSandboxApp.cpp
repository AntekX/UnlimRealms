
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
#include "HDRRender/HDRRender.h"
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

	// initialize GRAF objects
	std::vector<std::unique_ptr<GrafCommandList>> grafMainCmdList;
	std::unique_ptr<GrafRenderPass> grafPassColorDepth;
	std::unique_ptr<GrafImage> grafImageRTDepth;
	std::vector<std::unique_ptr<GrafRenderTarget>> grafTargetColorDepth;
	auto& initializeGrafRenderTargetObjects = [&]() -> Result
	{
		Result res(Success);
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// depth target image
		res = grafSystem->CreateImage(grafImageRTDepth);
		if (Failed(res)) return res;
		GrafImageDesc grafRTImageDepthDesc = {
			GrafImageType::Tex2D,
			GrafFormat::D24_UNORM_S8_UINT,
			ur_uint3(canvasWidth, canvasHeight, 1), 1,
			ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget),
			ur_uint(GrafDeviceMemoryFlag::GpuLocal)
		};
		res = grafImageRTDepth->Initialize(grafDevice, { grafRTImageDepthDesc });
		if (Failed(res)) return res;

		// initialize render target(s)
		grafTargetColorDepth.resize(grafRenderer->GetGrafCanvas()->GetSwapChainImageCount());
		for (ur_uint iimg = 0; iimg < grafRenderer->GetGrafCanvas()->GetSwapChainImageCount(); ++iimg)
		{
			res = grafSystem->CreateRenderTarget(grafTargetColorDepth[iimg]);
			if (Failed(res)) return res;
			GrafImage* grafColorDepthTargetImages[] = {
				grafRenderer->GetGrafCanvas()->GetSwapChainImage(iimg),
				grafImageRTDepth.get()
			};
			GrafRenderTarget::InitParams grafColorDepthTargetParams = {
				grafPassColorDepth.get(),
				grafColorDepthTargetImages,
				ur_array_size(grafColorDepthTargetImages)
			};
			res = grafTargetColorDepth[iimg]->Initialize(grafDevice, grafColorDepthTargetParams);
			if (Failed(res)) return res;
		}

		return res;
	};
	auto& deinitializeGrafRenderTargetObjects = [&](GrafCommandList* deferredDestroyCmdList = ur_null) -> Result
	{
		if (deferredDestroyCmdList != ur_null)
		{
			GrafImage *grafPrevRTImageDepth = grafImageRTDepth.release();
			grafRenderer->AddCommandListCallback(deferredDestroyCmdList, {},
				[grafPrevRTImageDepth](GrafCallbackContext& ctx) -> Result
			{
				delete grafPrevRTImageDepth;
				return Result(Success);
			});
		}
		grafTargetColorDepth.clear();
		grafImageRTDepth.reset();
		return Result(Success);
	};
	auto& initializeGrafObjects = [&]() -> Result
	{
		Result res(Success);
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// match number of recorded (in flight) frames to the GrafRenderer
		ur_uint frameCount = grafRenderer->GetRecordedFrameCount();

		// command lists (per frame)
		grafMainCmdList.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateCommandList(grafMainCmdList[iframe]);
			if (Failed(res)) break;
			res = grafMainCmdList[iframe]->Initialize(grafDevice);
			if (Failed(res)) break;
		}
		if (Failed(res)) return res;

		// color & depth render pass
		res = grafSystem->CreateRenderPass(grafPassColorDepth);
		if (Failed(res)) return res;
		GrafRenderPassImageDesc grafPassColorDepthImages[] = {
			{ // color
				GrafFormat::B8G8R8A8_UNORM,
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
		res = grafPassColorDepth->Initialize(grafDevice, { grafPassColorDepthImages, ur_array_size(grafPassColorDepthImages) });
		if (Failed(res)) return res;

		// initialize render target(s)
		res = initializeGrafRenderTargetObjects();

		return res;
	};
	auto& deinitializeGrafObjects = [&]() -> Result
	{
		deinitializeGrafRenderTargetObjects();
		grafPassColorDepth.reset();
		grafMainCmdList.clear();
		return Result(Success);
	};
	res = initializeGrafObjects();
	if (Failed(res))
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

	// HDR rendering manager
	std::unique_ptr<HDRRender> hdrRender(new HDRRender(realm));
	{
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.BloomThreshold = 4.0f;
		hdrRender->SetParams(hdrParams);
		res = hdrRender->Init(canvasWidth, canvasHeight, grafImageRTDepth.get());
		if (Failed(res))
		{
			realm.GetLog().WriteLine("RayTracingSandboxApp: failed to initialize HDRRender", Log::Error);
			hdrRender.reset();
		}
	}

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
		std::unique_ptr<GrafShader> shaderMissShadow;
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
			grafRenderer->SafeDelete(shaderMissShadow.release());
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
				ur_float3 norm;
			};
			typedef ur_uint32 IndexSample;
			VertexSample sampleVertices[] = {
				// cube
				{ {-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } }, { { 1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } }, { {-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } }, { { 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } },
				{ {-1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, { { 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, { {-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
				{ { 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f } }, { { 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, { { 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f } }, { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
				{ {-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f } }, { {-1.0f,-1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f } }, { {-1.0f, 1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f } }, { {-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f } },
				{ {-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f } }, { { 1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f } }, { {-1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, { { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
				{ {-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f } }, { { 1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f } }, { {-1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f } }, { { 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f } },
				// plane
				{ {-10.0f,-3.0f,-10.0f }, { 0.0f, 1.0f, 0.0f } }, { { 10.0f,-3.0f,-10.0f}, { 0.0f, 1.0f, 0.0f } }, { {-10.0f,-3.0f, 10.0f}, { 0.0f, 1.0f, 0.0f } }, { { 10.0f,-3.0f, 10.0f}, { 0.0f, 1.0f, 0.0f } },
			};
			IndexSample sampleIndices[] = {
				// cube
				2, 3, 1, 1, 0, 2,
				4, 5, 7, 7, 6, 4,
				10, 11, 9, 9, 8, 10,
				14, 15, 13, 13, 12, 14,
				18, 19, 17, 17, 16, 18,
				22, 23, 21, 21, 20, 22,
				// plane
				26, 27, 25, 25, 24, 26,
			};

			GrafBuffer::InitParams sampleVBParams;
			sampleVBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			sampleVBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			sampleVBParams.BufferDesc.SizeInBytes = sizeof(sampleVertices);
			grafSystem->CreateBuffer(this->vertexBuffer);
			this->vertexBuffer->Initialize(grafDevice, sampleVBParams);
			this->vertexBuffer->Write((ur_byte*)sampleVertices);

			GrafBuffer::InitParams sampleIBParams;
			sampleIBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
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
			accelStructGeomDescBL.PrimitiveCountMax = ur_array_size(sampleIndices) / 3;
			accelStructGeomDescBL.VertexCountMax = ur_array_size(sampleVertices);
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
			sampleGeometryDataBL.PrimitiveCount = ur_array_size(sampleIndices) / 3;

			GrafCommandList* cmdListBuildAccelStructBL = grafRenderer->GetTransientCommandList();
			cmdListBuildAccelStructBL->Begin();
			cmdListBuildAccelStructBL->BuildAccelerationStructure(this->accelerationStructureBL.get(), &sampleGeometryDataBL, 1);
			cmdListBuildAccelStructBL->End();
			grafDevice->Record(cmdListBuildAccelStructBL);
			grafDevice->Submit();
			grafDevice->WaitIdle();

			// initiaize top level acceleration structure container

			GrafAccelerationStructureGeometryDesc accelStructGeomDescTL = {};
			#if (1)
			accelStructGeomDescTL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
			accelStructGeomDescTL.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
			accelStructGeomDescTL.IndexType = GrafIndexType::UINT32;
			accelStructGeomDescTL.PrimitiveCountMax = ur_array_size(sampleIndices) / 3;
			accelStructGeomDescTL.VertexCountMax = ur_array_size(sampleVertices);
			accelStructGeomDescTL.TransformsEnabled = false;
			#else
			accelStructGeomDescTL.GeometryType = GrafAccelerationStructureGeometryType::Instances;
			accelStructGeomDescTL.PrimitiveCountMax = 1;
			#endif

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
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleMissShadow", GrafShaderType::Miss, this->shaderMissShadow);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleClosestHit", GrafShaderType::ClosestHit, this->shaderClosestHit);

			// global shader bindings

			GrafDescriptorRangeDesc bindingTableLayoutRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
				{ GrafDescriptorType::AccelerationStructure, 0, 1 },
				{ GrafDescriptorType::Buffer, 1, 2 },
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
				this->shaderMissShadow.get(),
				this->shaderClosestHit.get()
			};

			GrafRayTracingShaderGroupDesc shaderGroups[4];
			shaderGroups[0] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[0].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[0].GeneralShaderIdx = 0; // shaderRayGen
			shaderGroups[1] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[1].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[1].GeneralShaderIdx = 1; // shaderMiss
			shaderGroups[2] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[2].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[2].GeneralShaderIdx = 2; // shaderMissShadow
			shaderGroups[3] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[3].Type = GrafRayTracingShaderGroupType::TrianglesHit;
			shaderGroups[3].ClosestHitShaderIdx = 3; // shaderClosestHit

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
			this->rayGenShaderTable = { this->shaderHandlesBuffer.get(), 0 * shaderGroupHandleSize, 1 * shaderGroupHandleSize, shaderGroupHandleSize };
			this->missShaderTable = { this->shaderHandlesBuffer.get(), 1 * shaderGroupHandleSize, 2 * shaderGroupHandleSize, shaderGroupHandleSize };
			this->hitShaderTable = { this->shaderHandlesBuffer.get(), 3 * shaderGroupHandleSize, 1 * shaderGroupHandleSize, shaderGroupHandleSize };
		}

		void Render(GrafCommandList* grafCmdList, GrafImage* grafTargetImage, const ur_float4 &clearColor, Camera& camera)
		{
			if (ur_null == this->grafRenderer)
				return;

			struct SceneConstants
			{
				ur_float4x4 viewProjInv;
				ur_float4 cameraPos;
				ur_float4 viewportSize;
				ur_float4 clearColor;
				ur_float4 lightDirection;
			} cb;
			ur_uint3 targetSize = grafTargetImage->GetDesc().Size;
			cb.viewProjInv = camera.GetViewProjInv();
			cb.cameraPos = camera.GetPosition();
			cb.viewportSize.x = (ur_float)targetSize.x;
			cb.viewportSize.y = (ur_float)targetSize.y;
			cb.viewportSize.z = 1.0f / cb.viewportSize.x;
			cb.viewportSize.w = 1.0f / cb.viewportSize.y;
			cb.clearColor = clearColor;
			cb.lightDirection = ur_float3::Normalize(ur_float3(1.0f, 1.0f, 1.0f));
			GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
			Allocation dynamicCBAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SceneConstants));
			dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

			GrafDescriptorTable* descriptorTable = this->bindingTables[this->grafRenderer->GetCurrentFrameId()].get();
			descriptorTable->SetConstantBuffer(0, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
			descriptorTable->SetAccelerationStructure(0, this->accelerationStructureTL.get());
			descriptorTable->SetBuffer(1, this->vertexBuffer.get());
			descriptorTable->SetBuffer(2, this->indexBuffer.get());
			descriptorTable->SetRWImage(0, grafTargetImage);

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

			ctx.progress = 1;
		});

		// draw frame
		if (grafRenderer != ur_null)
		{
			// begin frame rendering
			grafRenderer->BeginFrame();

			// resize render target(s)
			if (canvasValid &&
				(canvasWidth != realm.GetCanvas()->GetClientBound().Width() ||
					canvasHeight != realm.GetCanvas()->GetClientBound().Height()))
			{
				canvasWidth = realm.GetCanvas()->GetClientBound().Width();
				canvasHeight = realm.GetCanvas()->GetClientBound().Height();
				// use prev frame command list to make sure RT objects are no longer used before destroying
				deinitializeGrafRenderTargetObjects(grafMainCmdList[grafRenderer->GetPrevFrameId()].get());
				// recreate RT objects for new canvas dimensions
				initializeGrafRenderTargetObjects();
				// reinit HDR renderer images
				if (hdrRender != ur_null)
				{
					hdrRender->Init(canvasWidth, canvasHeight, grafImageRTDepth.get());
				}
			}

			auto drawFrameJob = realm.GetJobSystem().Add(JobPriority::High, ur_null, [&](Job::Context& ctx) -> void
			{
				GrafDevice *grafDevice = grafRenderer->GetGrafDevice();
				GrafCanvas *grafCanvas = grafRenderer->GetGrafCanvas();
				GrafCommandList* grafCmdListCrnt = grafMainCmdList[grafRenderer->GetCurrentFrameId()].get();
				grafCmdListCrnt->Begin();

				GrafViewportDesc grafViewport = {};
				grafViewport.Width = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.x;
				grafViewport.Height = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.y;
				grafViewport.Near = 0.0f;
				grafViewport.Far = 1.0f;
				grafCmdListCrnt->SetViewport(grafViewport, true);

				GrafClearValue rtClearValue = { 0.8f, 0.9f, 1.0f, 0.0f };
				grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::TransferDst);
				grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), rtClearValue);

				if (hdrRender != ur_null)
				{
					// HDR & depth render pass

					hdrRender->BeginRender(*grafCmdListCrnt);

					// noet: non ray traced rendering can be done here

					hdrRender->EndRender(*grafCmdListCrnt);

					// ray traced scene

					if (rayTracingScene != ur_null)
					{
						rayTracingScene->Render(grafCmdListCrnt, hdrRender->GetHDRRenderTarget()->GetImage(0), (ur_float4&)rtClearValue.f32, camera);
					}

					// resolve hdr result

					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					hdrRender->Resolve(*grafCmdListCrnt, grafTargetColorDepth[grafCanvas->GetCurrentImageId()].get());
				}

				{ // foreground color render pass (drawing directly into swap chain image)

					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					grafCmdListCrnt->BeginRenderPass(grafRenderer->GetCanvasRenderPass(), grafRenderer->GetCanvasRenderTarget());
					grafCmdListCrnt->SetViewport(grafViewport, true);

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
						cameraControl.ShowImgui();
						if (hdrRender != ur_null)
						{
							hdrRender->ShowImgui();
						}

						imguiRender->Render(*grafCmdListCrnt);
					}

					grafCmdListCrnt->EndRenderPass();
				}

				// finalize current command list
				grafCmdListCrnt->End();
				grafDevice->Record(grafCmdListCrnt);
			});

			drawFrameJob->Wait();

			// finalize & move to next frame
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
	hdrRender.reset();

	// destroy sample GRAF objects
	deinitializeGrafObjects();

	// destroy realm objects
	realm.RemoveComponent<ImguiRender>();
	realm.RemoveComponent<GenericRender>();
	realm.RemoveComponent<GrafRenderer>();

	return 0;
}