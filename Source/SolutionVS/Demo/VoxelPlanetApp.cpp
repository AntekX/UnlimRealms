
#include "stdafx.h"
#include "VoxelPlanetApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Gfx/D3D11/GfxSystemD3D11.h"
#include "Gfx/D3D12/GfxSystemD3D12.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/GrafRenderer.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Resources/Resources.h"
#include "Camera/CameraControl.h"
#include "HDRRender/HDRRender.h"
#include "Isosurface/Isosurface.h"
#include "Atmosphere/Atmosphere.h"
#include "Multiverse/Multiverse.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

#if defined(UR_GRAF)

int VoxelPlanetApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"Voxel Planet Demo"));
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
		realm.GetLog().WriteLine("VoxelPlanetApp: failed to initialize GrafRenderer", Log::Error);
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
		realm.GetLog().WriteLine("VoxelPlanetApp: failed to initialize one or more graphics objects", Log::Error);
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
			realm.GetLog().WriteLine("VoxelPlanetApp: failed to initialize ImguiRender", Log::Error);
		}
	}

	// initialize generic primitives renderer
	GenericRender* genericRender = ur_null;
	if (grafRenderer != ur_null)
	{
		genericRender = realm.AddComponent<GenericRender>(realm);
		Result res = genericRender->Init(grafPassColorDepth.get());
		if (Failed(res))
		{
			realm.RemoveComponent<GenericRender>();
			genericRender = ur_null;
			realm.GetLog().WriteLine("VulkanSandbox: failed to initialize GenericRender", Log::Error);
		}
	}

	// HDR rendering manager
	std::unique_ptr<HDRRender> hdrRender(new HDRRender(realm));
	{
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.BloomThreshold = 4.0f;
		hdrRender->SetParams(hdrParams);
		res = hdrRender->Init(canvasWidth, canvasHeight, grafImageRTDepth.get());
		if (Failed(res))
		{
			realm.GetLog().WriteLine("VoxelPlanetApp: failed to initialize HDRRender", Log::Error);
			hdrRender.reset();
		}
	}

	// demo isosurface
	ur_float surfaceRadiusMin = 1000.0f;
	ur_float surfaceRadiusMax = 1100.0f;
	std::unique_ptr<Isosurface> isosurface(new Isosurface(realm));
	{
		ur_float r = surfaceRadiusMax;
		BoundingBox volumeBound(ur_float3(-r, -r, -r), ur_float3(r, r, r));

		Isosurface::ProceduralGenerator::SimplexNoiseParams generateParams;
		generateParams.bound = volumeBound;
		generateParams.radiusMin = surfaceRadiusMin;
		generateParams.radiusMax = surfaceRadiusMax;
		generateParams.octaves.assign({
			{ 0.875f, 7.5f, -1.0f, 0.5f },
			{ 0.345f, 30.0f, -0.5f, 0.1f },
			{ 0.035f, 120.0f, -1.0f, 0.2f },
		});

		std::unique_ptr<Isosurface::ProceduralGenerator> dataVolume(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::SimplexNoise, generateParams));

		Isosurface::HybridCubes::Desc desc;
		desc.CellSize = 2.0f;
		desc.LatticeResolution = 10;
		desc.DetailLevelDistance = desc.CellSize * desc.LatticeResolution.x * 1.0f;
		std::unique_ptr<Isosurface::HybridCubes> presentation(new Isosurface::HybridCubes(*isosurface.get(), desc));

		isosurface->Init(std::move(dataVolume), std::move(presentation), (hdrRender ? hdrRender->GetRenderPass() : grafPassColorDepth.get()));
	}

	// demo atmosphere
	std::unique_ptr<Atmosphere> atmosphere(new Atmosphere(realm));
	{
		Atmosphere::Desc desc = Atmosphere::Desc::Default;
		desc.InnerRadius = lerp(surfaceRadiusMin, surfaceRadiusMax, 0.75f);
		desc.OuterRadius = surfaceRadiusMin * 1.2f;
		desc.Kr = 0.00005f;
		desc.Km = 0.00005f;
		desc.ScaleDepth = 0.16f;
		atmosphere->Init(desc, (hdrRender ? hdrRender->GetRenderPass() : grafPassColorDepth.get()));
	}

	// main application camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::Free);
	camera.SetPosition(ur_float3(0.0f, 0.0f, -surfaceRadiusMax * 3.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

	// TEMP: ray tracing test
	class RayTracingTest : public RealmEntity
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

		RayTracingTest(Realm& realm) : RealmEntity(realm), grafRenderer(ur_null) {}
		~RayTracingTest()
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
			const ur_float vs = 10000.0;
			VertexSample sampleVertices[] = {
				{ {-0.5f * vs, 0.5f * vs, 1.0f } },
				{ { 0.5f * vs,-0.5f * vs, 1.0f } },
				{ {-0.5f * vs,-0.5f * vs, 1.0f } }
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
			accelStructGeomDescBL.PrimitiveCountMax = 12; // 1 cube
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

			// initiaize top level acceleration structure container

			GrafAccelerationStructureGeometryDesc accelStructGeomDescTL = {};
			accelStructGeomDescTL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
			accelStructGeomDescTL.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
			accelStructGeomDescTL.IndexType = GrafIndexType::UINT32;
			accelStructGeomDescTL.PrimitiveCountMax = 12; // 1 cube
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
					{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f },
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

			// shaders

			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleRaygen", GrafShaderType::RayGen, this->shaderRayGen);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleClosestHit", GrafShaderType::ClosestHit, this->shaderClosestHit);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleMiss", GrafShaderType::Miss, this->shaderMiss);

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
				this->shaderClosestHit.get(),
				this->shaderMiss.get()
			};

			GrafRayTracingShaderGroupDesc shaderGroups[3];
			shaderGroups[0] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[0].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[0].GeneralShaderIdx = 0; // shaderRayGen
			shaderGroups[1] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[1].Type = GrafRayTracingShaderGroupType::TrianglesHit;
			shaderGroups[1].ClosestHitShaderIdx = 1; // shaderClosestHit
			shaderGroups[2] = GrafRayTracingShaderGroupDesc::Default;
			shaderGroups[2].Type = GrafRayTracingShaderGroupType::General;
			shaderGroups[2].GeneralShaderIdx = 2; // shaderMiss

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
			this->missShaderTable = { this->shaderHandlesBuffer.get(), 2 * shaderGroupHandleSize, shaderGroupHandleSize, shaderGroupHandleSize };
			this->hitShaderTable = { this->shaderHandlesBuffer.get(), 1 * shaderGroupHandleSize, shaderGroupHandleSize, shaderGroupHandleSize };
		}

		void Render(GrafCommandList* grafCmdList, GrafImage* grafTargetImage, const ur_float4x4 &viewProj, const ur_float3 &cameraPos)
		{
			if (ur_null == this->grafRenderer)
				return;

			struct SceneConstants
			{
				ur_float4x4 viewProjInv;
				ur_float4 cameraPos;
				ur_float4 viewportSize;
			} cb;
			ur_uint3 targetSize = grafTargetImage->GetDesc().Size;
			cb.viewProjInv = ur_float4x4::Identity;// todo: calculate inverse matrix
			cb.cameraPos = cameraPos;
			cb.viewportSize.x = (ur_float)targetSize.x;
			cb.viewportSize.y = (ur_float)targetSize.y;
			cb.viewportSize.z = 1.0f / cb.viewportSize.x;
			cb.viewportSize.w = 1.0f / cb.viewportSize.y;
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
	std::unique_ptr<RayTracingTest> rayTracingTest;
	if (grafRenderer != ur_null)
	{
		GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = grafRenderer->GetGrafDevice();
		const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());
		if (grafDeviceDesc != ur_null && grafDeviceDesc->RayTracing.RayTraceSupported)
		{
			rayTracingTest.reset(new RayTracingTest(realm));
			rayTracingTest->Initialize();
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
		enum UpdateStageFence : ur_uint
		{
			UpdateStage_Beginning = 0,
			UpdateStage_IsosurfaceReady,
			UpdateStage_Finished
		};
		auto updateFrameJob = realm.GetJobSystem().Add(JobPriority::High, ur_null, [&](Job::Context& ctx) -> void
		{
			// reset update progress fence
			ctx.progress = UpdateStage_Beginning;

			// move timer
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f; // to seconds

			// update isosurface
			isosurface->GetPresentation()->Update(camera.GetPosition(), camera.GetViewProj());
			ctx.progress = UpdateStage_IsosurfaceReady;

			// update camera control speed depending on the distance to isosurface
			ur_float surfDist = (camera.GetPosition() - isosurface->GetData()->GetBound().Center()).Length() - surfaceRadiusMin;
			cameraControl.SetSpeed(std::max(5.0f, surfDist * 0.5f));

			ctx.progress = UpdateStage_Finished;
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

				GrafClearValue rtClearValue = { 0.025f, 0.025f, 0.05f, 0.0f };
				grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::TransferDst);
				grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), rtClearValue);

				if (hdrRender != ur_null)
				{ 
					// HDR & depth render pass

					hdrRender->BeginRender(*grafCmdListCrnt);

					// draw isosurface
					if (isosurface != ur_null)
					{
						updateFrameJob->WaitProgress(UpdateStage_IsosurfaceReady);
						isosurface->Render(*grafCmdListCrnt, camera.GetViewProj(), camera.GetPosition(), atmosphere.get());
					}

					// draw atmosphere
					if (atmosphere != ur_null)
					{
						atmosphere->Render(*grafCmdListCrnt, camera.GetViewProj(), camera.GetPosition());
					}

					hdrRender->EndRender(*grafCmdListCrnt);

					// atmospheric post effects
					if (atmosphere != ur_null)
					{
						atmosphere->RenderPostEffects(*grafCmdListCrnt, *hdrRender->GetHDRRenderTarget(), camera.GetViewProj(), camera.GetPosition());
					}

					// ray tracing test
					if (rayTracingTest != ur_null)
					{
						rayTracingTest->Render(grafCmdListCrnt, hdrRender->GetHDRRenderTarget()->GetImage(0), camera.GetViewProj(), camera.GetPosition());
					}

					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					hdrRender->Resolve(*grafCmdListCrnt, grafTargetColorDepth[grafCanvas->GetCurrentImageId()].get());
				}

				{ // color & depth render pass

					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					grafCmdListCrnt->BeginRenderPass(grafPassColorDepth.get(), grafTargetColorDepth[grafCanvas->GetCurrentImageId()].get());

					// render immediate mode generic primitives
					genericRender->Render(*grafCmdListCrnt, camera.GetViewProj());

					grafCmdListCrnt->EndRenderPass();
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
						if (isosurface != ur_null)
						{
							isosurface->ShowImgui();
						}
						if (atmosphere != ur_null)
						{
							atmosphere->ShowImgui();
						}
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
	rayTracingTest.reset();
	isosurface.reset();
	atmosphere.reset();
	hdrRender.reset();

	// destroy GRAF objects
	deinitializeGrafObjects();

	// destroy realm objects
	realm.RemoveComponent<ImguiRender>();
	realm.RemoveComponent<GenericRender>();
	realm.RemoveComponent<GrafRenderer>();

	return 0;
}

#else

int VoxelPlanetApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"Voxel Planet Demo"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// create gfx system
	std::unique_ptr<GfxSystemD3D11> gfx(new GfxSystemD3D11(realm));
	//std::unique_ptr<GfxSystemD3D12> gfx(new GfxSystemD3D12(realm));
	Result res = gfx->Initialize(realm.GetCanvas());
	realm.SetGfxSystem(std::move(gfx));

	// create swap chain
	ur_uint canvasWidth = realm.GetCanvas()->GetClientBound().Width();
	ur_uint canvasHeight = realm.GetCanvas()->GetClientBound().Height();
	std::unique_ptr<GfxSwapChain> gfxSwapChain;
	if (Succeeded(realm.GetGfxSystem()->CreateSwapChain(gfxSwapChain)))
	{
		res = gfxSwapChain->Initialize(canvasWidth, canvasHeight);
	}

	// create gfx context
	std::unique_ptr<GfxContext> gfxContext;
	if (Succeeded(realm.GetGfxSystem()->CreateContext(gfxContext)))
	{
		res = gfxContext->Initialize();
	}

	// initialize ImguiRender
	ImguiRender *imguiRender = realm.AddComponent<ImguiRender>(realm);
	if (imguiRender != ur_null)
	{
		imguiRender = realm.GetComponent<ImguiRender>();
		res = imguiRender->Init();
	}

	// initialize GenericRender
	GenericRender *genericRender = realm.AddComponent<GenericRender>(realm);
	if (genericRender != ur_null)
	{
		genericRender = realm.GetComponent<GenericRender>();
		genericRender->Init();
	}

	// HDR rendering
	HDRRender* hdrRender = realm.AddComponent<HDRRender>(realm);
	if (hdrRender != ur_null)
	{
		hdrRender = realm.GetComponent<HDRRender>();
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.BloomThreshold = 4.0f;
		hdrRender->SetParams(hdrParams);
		hdrRender->Init(canvasWidth, canvasHeight);
	}

	// demo isosurface
	ur_float surfaceRadiusMin = 1000.0f;
	ur_float surfaceRadiusMax = 1100.0f;
	std::unique_ptr<Isosurface> isosurface(new Isosurface(realm));
	{
		ur_float r = surfaceRadiusMax;
		BoundingBox volumeBound(ur_float3(-r, -r, -r), ur_float3(r, r, r));
#if 0
		Isosurface::ProceduralGenerator::SphericalDistanceFieldParams generateParams;
		generateParams.bound = volumeBound;
		generateParams.center = volumeBound.Center();
		generateParams.radius = surfaceRadiusMin;
		std::unique_ptr<Isosurface::ProceduralGenerator> dataVolume(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::SphericalDistanceField, generateParams));
#else
		Isosurface::ProceduralGenerator::SimplexNoiseParams generateParams;
		generateParams.bound = volumeBound;
		generateParams.radiusMin = surfaceRadiusMin;
		generateParams.radiusMax = surfaceRadiusMax;
		// smooth
		generateParams.octaves.assign({
			{ 0.875f, 7.5f, -1.0f, 0.5f },
			{ 0.345f, 30.0f, -0.5f, 0.1f },
			{ 0.035f, 120.0f, -1.0f, 0.2f },
			});
		// canyons
		/*generateParams.octaves.assign({
		{ 1.100f, 8.0f, -0.25f, 0.4f },
		{ 0.345f, 32.0f, -0.25f, 0.1f },
		{ 0.035f, 128.0f, -1.0f, 0.2f },
		});*/

		std::unique_ptr<Isosurface::ProceduralGenerator> dataVolume(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::SimplexNoise, generateParams));
#endif

		Isosurface::HybridCubes::Desc desc;
		desc.CellSize = 2.0f;
		desc.LatticeResolution = 10;
		desc.DetailLevelDistance = desc.CellSize * desc.LatticeResolution.x * 1.0f;
		std::unique_ptr<Isosurface::HybridCubes> presentation(new Isosurface::HybridCubes(*isosurface.get(), desc));

		//dataVolume->Save("test.isd", desc.CellSize, desc.LatticeResolution);

		isosurface->Init(std::move(dataVolume), std::move(presentation));
	}

	// demo atmosphere
	std::unique_ptr<Atmosphere> atmosphere(new Atmosphere(realm));
	{
		Atmosphere::Desc desc = Atmosphere::Desc::Default;
		desc.InnerRadius = lerp(surfaceRadiusMin, surfaceRadiusMax, 0.75f);
		desc.OuterRadius = surfaceRadiusMin * 1.2f;
		desc.Kr = 0.00005f;
		desc.Km = 0.00005f;
		desc.ScaleDepth = 0.16f;
		atmosphere->Init(desc);
	}

	// demo moon
	ur_float moonRadiusMin = 400.0f;
	ur_float moonRadiusMax = 420.0f;
	std::unique_ptr<Isosurface> moon(new Isosurface(realm));
	{
		ur_float r = moonRadiusMax;
		ur_float3 p = ur_float3(+1000, 1500.0f, 1500.0f);
		BoundingBox volumeBound(ur_float3(-r, -r, -r) + p, ur_float3(r, r, r) + p);
		Isosurface::ProceduralGenerator::SimplexNoiseParams generateParams;
		generateParams.bound = volumeBound;
		generateParams.radiusMin = moonRadiusMin;
		generateParams.radiusMax = moonRadiusMax;
		generateParams.octaves.assign({
			{ 1.000f, 4.0f, -0.0f, 1.0f },
			{ 0.400f, 16.0f, -1.0f, 0.2f },
			{ 0.035f, 128.0f, -1.0f, 0.2f },
			});
		std::unique_ptr<Isosurface::ProceduralGenerator> dataVolume(new Isosurface::ProceduralGenerator(*moon.get(),
			Isosurface::ProceduralGenerator::Algorithm::SimplexNoise, generateParams));

		Isosurface::HybridCubes::Desc desc;
		desc.CellSize = 2.0f;
		desc.LatticeResolution = 10;
		desc.DetailLevelDistance = desc.CellSize * desc.LatticeResolution.x * 1.0f;
		std::unique_ptr<Isosurface::HybridCubes> presentation(new Isosurface::HybridCubes(*moon.get(), desc));

		moon->Init(std::move(dataVolume), std::move(presentation));
	}

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::Free);
	camera.SetPosition(ur_float3(0.0f, 0.0f, -surfaceRadiusMax * 3.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

	// multiverse
	//Multiverse *multiverse = ur_null;
	//Space *space = ur_null;
	//if (realm.AddComponent<Multiverse>(realm))
	//{
	//	multiverse = realm.GetComponent<Multiverse>();
	//	// TODO: add a space here
	//}

	// Main message loop:
	realm.GetLog().WriteLine("Entering main message loop");
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		// process messages first
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) && msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// forward msg to WinInput system
			WinInput *winInput = static_cast<WinInput*>(realm.GetInput());
			winInput->ProcessMsg(msg);
		}

		// update canvas
		if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() ||
			canvasHeight != realm.GetCanvas()->GetClientBound().Height())
		{
			canvasWidth = realm.GetCanvas()->GetClientBound().Width();
			canvasHeight = realm.GetCanvas()->GetClientBound().Height();
			gfxSwapChain->Initialize(canvasWidth, canvasHeight);
			hdrRender->Init(canvasWidth, canvasHeight);
		}

		// update sub systems
		realm.GetInput()->Update();
		imguiRender->NewFrame();
		cameraControl.Update();
		camera.SetAspectRatio((float)canvasWidth / canvasHeight);

		// update isosurface
		isosurface->GetPresentation()->Update(camera.GetPosition(), camera.GetViewProj());
		isosurface->Update();
		moon->GetPresentation()->Update(camera.GetPosition(), camera.GetViewProj());

		// update camera control speed depending on the distance to isosurface
		ur_float surfDist = std::min(
			(camera.GetPosition() - isosurface->GetData()->GetBound().Center()).Length() - surfaceRadiusMin,
			(camera.GetPosition() - moon->GetData()->GetBound().Center()).Length() - moonRadiusMin);
		cameraControl.SetSpeed(std::max(5.0f, surfDist * 0.5f));

		{ // use context to draw
			gfxContext->Begin();

			// begin HDR rendering
			if (Succeeded(hdrRender->BeginRender(*gfxContext)))
			{
				// draw isosurface
				moon->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition(), ur_null);
				isosurface->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition(), atmosphere.get());

				// draw atmosphere
				atmosphere->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition());

				// end HDR rendering
				hdrRender->EndRender(*gfxContext);

				// atmospheric post effects
				atmosphere->RenderPostEffects(*gfxContext, *hdrRender->GetHDRTarget(), camera.GetViewProj(), camera.GetPosition());

				// resolve HDR image to back buffer
				gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer(), hdrRender->GetHDRTarget());
				hdrRender->Resolve(*gfxContext);

				// render batched generic primitives
				genericRender->Render(*gfxContext, camera.GetViewProj());
			}

			// expose demo gui
			static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
			static bool showGUI = true;
			showGUI = (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
			if (showGUI)
			{
				ImGui::SetNextWindowSize(imguiDemoWndSize, ImGuiSetCond_Once);
				ImGui::SetNextWindowPos({ canvasWidth - imguiDemoWndSize.x, 0.0f }, ImGuiSetCond_Once);
				ImGui::Begin("Control Panel");
				ImGui::Text("Gfx Adapter: %S", gfxContext->GetGfxSystem().GetActiveAdapterDesc().Description.c_str());
				cameraControl.ShowImgui();
				isosurface->ShowImgui();
				atmosphere->ShowImgui();
				hdrRender->ShowImgui();
				ImGui::End();

				// Imgui metrics
				ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
				ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
				ImGui::ShowMetricsWindow();
				imguiRender->Render(*gfxContext);
			}

			gfxContext->End();
		}

		realm.GetGfxSystem()->Render();

		gfxSwapChain->Present();
	}
	realm.GetLog().WriteLine("Left main message loop");

	// deinitialize explicitly before realm instance destroyed
	isosurface.reset(ur_null);
	moon.reset(ur_null);

	return 0;
}

#endif