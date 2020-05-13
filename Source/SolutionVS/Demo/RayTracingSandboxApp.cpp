
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
#include "Atmosphere/Atmosphere.h"
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
	cameraControl.SetTargetPoint(ur_float3(0.0f));
	cameraControl.SetSpeed(4.0);
	camera.SetPosition(ur_float3(9.541f, 5.412f,-12.604f));
	camera.SetLookAt(cameraControl.GetTargetPoint(), cameraControl.GetWorldUp());

	// light source params
	LightDesc sunLight = {};
	sunLight.Color = { 1.0f, 1.0f, 1.0f };
	sunLight.Intensity = 200.0f;
	sunLight.Direction = { -0.8165f,-0.40825f,-0.40825f };
	LightDesc sunLight2 = {};
	sunLight2.Color = { 1.0f, 0.1f, 0.0f };
	sunLight2.Intensity = 200.0f;
	sunLight2.Direction = { 0.8018f,-0.26726f,-0.5345f };
	LightingDesc lightingDesc = {};
	lightingDesc.LightSourceCount = 2;
	lightingDesc.LightSources[0] = sunLight;
	lightingDesc.LightSources[1] = sunLight2;

	// atmosphere params
	Atmosphere::Desc atmosphereDesc = {
		6371.0e+3f,
		6381.0e+3f,
		0.250f,
		-0.980f,
		2.0e-7f,
		7.0e-7f,
		2.718f,
	};

	// HDR rendering manager
	std::unique_ptr<HDRRender> hdrRender(new HDRRender(realm));
	{
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.LumAdaptationMin = sunLight.Intensity * 0.5f;
		hdrParams.LumAdaptationMax = sunLight.Intensity * 1.5f;
		hdrParams.BloomThreshold = 1.0f;
		hdrParams.BloomIntensity = 0.05f;
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

		struct VertexSample
		{
			ur_float3 pos;
			ur_float3 norm;
		};

		typedef ur_uint32 IndexSample;

		struct MeshDescSample
		{
			ur_uint32 vertexBufferOfs;
			ur_uint32 indexBufferOfs;
		};

		class Mesh : public GrafEntity
		{
		public:

			Allocation vertexBufferRegion;
			Allocation indexBufferRegion;
			Allocation meshBufferRegion;
			std::unique_ptr<GrafAccelerationStructure> accelerationStructureBL;

			Mesh(GrafSystem &grafSystem) : GrafEntity(grafSystem) {}
			~Mesh() {}

			void Initialize(GrafRenderer* grafRenderer, GrafBuffer* meshBuffer, LinearAllocator& mbAllocator,
				GrafBuffer* vertexBuffer, LinearAllocator& vbAllocator, const ur_byte* vertices, ur_size verticesCount, ur_size vertexStride,
				GrafBuffer* indexBuffer, LinearAllocator& ibAllocator, const ur_byte* indices, ur_size indicesCount, GrafIndexType indexType)
			{
				GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
				GrafDevice* grafDevice = grafRenderer->GetGrafDevice();

				// allocate regions in shared mesh buffers and write

				this->meshBufferRegion = mbAllocator.Allocate(sizeof(MeshDescSample));
				this->vertexBufferRegion = vbAllocator.Allocate(verticesCount * vertexStride);
				this->indexBufferRegion = ibAllocator.Allocate(indicesCount * (GrafIndexType::UINT16 == indexType ? 2 : 4));
				if (0 == this->vertexBufferRegion.Size || 0 == this->indexBufferRegion.Size)
					return; // exceeeds reserved size

				MeshDescSample meshDesc = {};
				meshDesc.vertexBufferOfs = (ur_uint32)this->vertexBufferRegion.Offset;
				meshDesc.indexBufferOfs = (ur_uint32)this->indexBufferRegion.Offset;

				meshBuffer->Write((ur_byte*)&meshDesc, this->meshBufferRegion.Size, 0, this->meshBufferRegion.Offset);
				vertexBuffer->Write(vertices, this->vertexBufferRegion.Size, 0, this->vertexBufferRegion.Offset);
				indexBuffer->Write(indices, this->indexBufferRegion.Size, 0, this->indexBufferRegion.Offset);

				// initiaize bottom level acceleration structure container

				GrafAccelerationStructureGeometryDesc accelStructGeomDescBL = {};
				accelStructGeomDescBL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
				accelStructGeomDescBL.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
				accelStructGeomDescBL.IndexType = indexType;
				accelStructGeomDescBL.PrimitiveCountMax = ur_uint32(indicesCount / 3);
				accelStructGeomDescBL.VertexCountMax = ur_uint32(verticesCount);
				accelStructGeomDescBL.TransformsEnabled = false;

				GrafAccelerationStructure::InitParams accelStructParamsBL = {};
				accelStructParamsBL.StructureType = GrafAccelerationStructureType::BottomLevel;
				accelStructParamsBL.BuildFlags = GrafAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlag::PreferFastTrace);
				accelStructParamsBL.Geometry = &accelStructGeomDescBL;
				accelStructParamsBL.GeometryCount = 1;

				grafSystem->CreateAccelerationStructure(this->accelerationStructureBL);
				this->accelerationStructureBL->Initialize(grafDevice, accelStructParamsBL);

				// build bottom level acceleration structure for sample geometry

				GrafAccelerationStructureTrianglesData trianglesData = {};
				trianglesData.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
				trianglesData.VertexStride = vertexStride;
				trianglesData.VerticesDeviceAddress = vertexBuffer->GetDeviceAddress() + this->vertexBufferRegion.Offset;
				trianglesData.IndexType = indexType;
				trianglesData.IndicesDeviceAddress = indexBuffer->GetDeviceAddress() + this->indexBufferRegion.Offset;

				GrafAccelerationStructureGeometryData geometryDataBL = {};
				geometryDataBL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
				geometryDataBL.GeometryFlags = ur_uint(GrafAccelerationStructureGeometryFlag::Opaque);
				geometryDataBL.TrianglesData = &trianglesData;
				geometryDataBL.PrimitiveCount = ur_uint32(indicesCount / 3);

				GrafCommandList* cmdListBuildAccelStructBL = grafRenderer->GetTransientCommandList();
				cmdListBuildAccelStructBL->Begin();
				cmdListBuildAccelStructBL->BuildAccelerationStructure(this->accelerationStructureBL.get(), &geometryDataBL, 1);
				cmdListBuildAccelStructBL->End();
				grafDevice->Record(cmdListBuildAccelStructBL);
				//grafDevice->Submit();
				//grafDevice->WaitIdle();
			}

			inline ur_uint32 GetMeshID() const { return ur_uint32(this->meshBufferRegion.Offset / sizeof(MeshDescSample)); }
		};

		GrafRenderer* grafRenderer;
		std::unique_ptr<Mesh> cubeMesh;
		std::unique_ptr<Mesh> planeMesh;
		std::vector<std::unique_ptr<Mesh>> customMeshes;
		LinearAllocator vertexBufferAllocator;
		LinearAllocator indexBufferAllocator;
		LinearAllocator meshBufferAllocator;
		std::unique_ptr<GrafBuffer> vertexBuffer;
		std::unique_ptr<GrafBuffer> indexBuffer;
		std::unique_ptr<GrafBuffer> meshBuffer;
		std::unique_ptr<GrafBuffer> instanceBuffer;
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
		std::unique_ptr<GrafImage> occlusionBuffer[2];
		std::unique_ptr<GrafImage> depthBuffer[2];
		ur_float4x4 viewProjPrev;
		ur_bool accumulationEnabled;
		ur_uint accumulationFrameNumber;
		ur_uint accumulationFrameCount;
		
		ur_size sampleInstanceCount;
		ur_bool animationEnabled;
		ur_float animationCycleTime;
		ur_float animationElapsedTime;

		RayTracingScene(Realm& realm) : RealmEntity(realm), grafRenderer(ur_null)
		{
			this->sampleInstanceCount = 7;
			this->animationEnabled = true;
			this->animationCycleTime = 30.0f;
			this->animationElapsedTime = 0.0f;
			this->accumulationEnabled = true;
			this->accumulationFrameNumber = 0;
			this->accumulationFrameCount = 33;
		}
		~RayTracingScene()
		{
			if (ur_null == grafRenderer)
				return;

			grafRenderer->SafeDelete(cubeMesh.release());
			grafRenderer->SafeDelete(planeMesh.release());
			for (auto& customMesh : this->customMeshes)
			{
				grafRenderer->SafeDelete(customMesh.release());
			}
			grafRenderer->SafeDelete(vertexBuffer.release());
			grafRenderer->SafeDelete(indexBuffer.release());
			grafRenderer->SafeDelete(meshBuffer.release());
			grafRenderer->SafeDelete(instanceBuffer.release());
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
			for (ur_uint i = 0; i < 2; ++i)
			{
				grafRenderer->SafeDelete(occlusionBuffer[i].release());
				grafRenderer->SafeDelete(depthBuffer[i].release());
			}
		}

		void Initialize()
		{
			this->grafRenderer = this->GetRealm().GetComponent<GrafRenderer>();
			if (ur_null == grafRenderer)
				return;

			GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = grafRenderer->GetGrafDevice();
			const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());

			const ur_size VertexCountMax = (1 << 20);
			const ur_size IndexCountMax = (1 << 20);
			const ur_size MeshCountMax = (1 << 10);
			const ur_size InstanceCountMax = 128;

			// initialize common vertex attributes buffer

			GrafBuffer::InitParams VBParams;
			VBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			VBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			VBParams.BufferDesc.SizeInBytes = VertexCountMax * sizeof(VertexSample);

			grafSystem->CreateBuffer(this->vertexBuffer);
			this->vertexBuffer->Initialize(grafDevice, VBParams);
			this->vertexBufferAllocator.Init(VBParams.BufferDesc.SizeInBytes);

			// initialize common index buffer

			GrafBuffer::InitParams IBParams;
			IBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			IBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			IBParams.BufferDesc.SizeInBytes = IndexCountMax * sizeof(IndexSample);

			grafSystem->CreateBuffer(this->indexBuffer);
			this->indexBuffer->Initialize(grafDevice, IBParams);
			this->indexBufferAllocator.Init(IBParams.BufferDesc.SizeInBytes);

			// initialize common mesh description buffer

			GrafBuffer::InitParams MBParams;
			MBParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			MBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			MBParams.BufferDesc.SizeInBytes = MeshCountMax * sizeof(MeshDescSample);

			grafSystem->CreateBuffer(this->meshBuffer);
			this->meshBuffer->Initialize(grafDevice, MBParams);
			this->meshBufferAllocator.Init(MBParams.BufferDesc.SizeInBytes);

			// sample plane mesh

			ur_float ps = 1000.0f;
			ur_float ph = -2.0f;
			VertexSample planeVertices[] = {
				{ {-ps, ph,-ps }, { 0.0f, 1.0f, 0.0f } }, { { ps, ph,-ps}, { 0.0f, 1.0f, 0.0f } }, { {-ps, ph, ps}, { 0.0f, 1.0f, 0.0f } }, { { ps, ph, ps}, { 0.0f, 1.0f, 0.0f } },
			};
			IndexSample planeIndices[] = {
				2, 3, 1, 1, 0, 2,
			};

			this->planeMesh.reset(new Mesh(*grafSystem));
			this->planeMesh->Initialize(this->grafRenderer, this->meshBuffer.get(), this->meshBufferAllocator,
				this->vertexBuffer.get(), this->vertexBufferAllocator, (const ur_byte*)planeVertices, ur_array_size(planeVertices), sizeof(VertexSample),
				this->indexBuffer.get(), this->indexBufferAllocator, (const ur_byte*)planeIndices, ur_array_size(planeIndices), GrafIndexType::UINT32);

			// sample cube mesh

			const VertexSample cubeVertices[] = {
				{ {-1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } }, { { 1.0f,-1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } }, { {-1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } }, { { 1.0f, 1.0f,-1.0f }, { 0.0f, 0.0f,-1.0f } },
				{ {-1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, { { 1.0f,-1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, { {-1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
				{ { 1.0f,-1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f } }, { { 1.0f,-1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, { { 1.0f, 1.0f,-1.0f }, { 1.0f, 0.0f, 0.0f } }, { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
				{ {-1.0f,-1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f } }, { {-1.0f,-1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f } }, { {-1.0f, 1.0f, 1.0f }, {-1.0f, 0.0f, 0.0f } }, { {-1.0f, 1.0f,-1.0f }, {-1.0f, 0.0f, 0.0f } },
				{ {-1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f } }, { { 1.0f, 1.0f,-1.0f }, { 0.0f, 1.0f, 0.0f } }, { {-1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, { { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
				{ {-1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f } }, { { 1.0f,-1.0f, 1.0f }, { 0.0f,-1.0f, 0.0f } }, { {-1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f } }, { { 1.0f,-1.0f,-1.0f }, { 0.0f,-1.0f, 0.0f } },
			};
			const IndexSample cubeIndices[] = {
				2, 3, 1, 1, 0, 2,
				4, 5, 7, 7, 6, 4,
				10, 11, 9, 9, 8, 10,
				14, 15, 13, 13, 12, 14,
				18, 19, 17, 17, 16, 18,
				22, 23, 21, 21, 20, 22,
			};

			this->cubeMesh.reset(new Mesh(*grafSystem));
			this->cubeMesh->Initialize(this->grafRenderer, this->meshBuffer.get(), this->meshBufferAllocator,
				this->vertexBuffer.get(), this->vertexBufferAllocator, (const ur_byte*)cubeVertices, ur_array_size(cubeVertices), sizeof(VertexSample),
				this->indexBuffer.get(), this->indexBufferAllocator, (const ur_byte*)cubeIndices, ur_array_size(cubeIndices), GrafIndexType::UINT32);

			// load custom mesh(es)

			GrafUtils::MeshVertexElementFlags vertexMask = (ur_uint(GrafUtils::MeshVertexElementFlag::Position) | ur_uint(GrafUtils::MeshVertexElementFlag::Normal)); // load only subset of attributes
			std::string modelResName[] = {
				"../Res/Models/sphere.obj",
				"../Res/Models/wuson.obj",
				"../Res/Models/Medieval_building.obj",
			};
			for (ur_size ires = 0; ires < ur_array_size(modelResName); ++ires)
			{
				GrafUtils::ModelData modelData;
				if (GrafUtils::LoadModelFromFile(*grafDevice, modelResName[ires], modelData, vertexMask) == Success && modelData.Meshes.size() > 0)
				{
					const GrafUtils::MeshData& meshData = *modelData.Meshes[0];
					if (meshData.VertexElementFlags & vertexMask) // make sure all masked attributes available in the mesh
					{
						std::unique_ptr<Mesh> mesh(new Mesh(*grafSystem));
						mesh->Initialize(this->grafRenderer, this->meshBuffer.get(), this->meshBufferAllocator,
							this->vertexBuffer.get(), this->vertexBufferAllocator, meshData.Vertices.data(), meshData.Vertices.size() / sizeof(VertexSample), sizeof(VertexSample),
							this->indexBuffer.get(), this->indexBufferAllocator, meshData.Indices.data(), meshData.Indices.size() / sizeof(ur_uint32), GrafIndexType::UINT32);
						this->customMeshes.emplace_back(std::move(mesh));
					}
				}
			}

			// initiaize top level acceleration structure container

			GrafAccelerationStructureGeometryDesc accelStructGeomDescTL = {};
			accelStructGeomDescTL.GeometryType = GrafAccelerationStructureGeometryType::Instances;
			accelStructGeomDescTL.PrimitiveCountMax = InstanceCountMax;

			GrafAccelerationStructure::InitParams accelStructParamsTL = {};
			accelStructParamsTL.StructureType = GrafAccelerationStructureType::TopLevel;
			accelStructParamsTL.BuildFlags = GrafAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlag::PreferFastTrace);
			accelStructParamsTL.Geometry = &accelStructGeomDescTL;
			accelStructParamsTL.GeometryCount = 1;

			grafSystem->CreateAccelerationStructure(this->accelerationStructureTL);
			this->accelerationStructureTL->Initialize(grafDevice, accelStructParamsTL);

			// initialize common instance buffer for TLAS

			GrafBuffer::InitParams sampleInstanceParams;
			sampleInstanceParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			sampleInstanceParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			sampleInstanceParams.BufferDesc.SizeInBytes = InstanceCountMax * sizeof(GrafAccelerationStructureInstance);

			grafSystem->CreateBuffer(this->instanceBuffer);
			this->instanceBuffer->Initialize(grafDevice, sampleInstanceParams);

			// build TLAS

			this->BuildTopLevelAccelerationStructure();

			// shaders

			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleRaygen", GrafShaderType::RayGen, this->shaderRayGen);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleMiss", GrafShaderType::Miss, this->shaderMiss);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleMissShadow", GrafShaderType::Miss, this->shaderMissShadow);
			GrafUtils::CreateShaderFromFile(*grafDevice, "sample_raytracing_lib.spv", "SampleClosestHit", GrafShaderType::ClosestHit, this->shaderClosestHit);

			// global shader bindings

			GrafDescriptorRangeDesc bindingTableLayoutRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
				{ GrafDescriptorType::AccelerationStructure, 0, 1 },
				{ GrafDescriptorType::Buffer, 1, 3 },
				{ GrafDescriptorType::RWTexture, 0, 5 }
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

		void PrepareAccumulationData(GrafCommandList* grafCmdList, GrafImage* grafTargetImage, Camera& camera)
		{
			if (ur_null == this->grafRenderer)
				return;

			GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();

			// (re)create buffers

			ur_uint3 targetSize = (grafTargetImage ? grafTargetImage->GetDesc().Size : 0);
			ur_uint3 crntSize = (this->occlusionBuffer[0] ? this->occlusionBuffer[0]->GetDesc().Size : 0);
			if (targetSize != crntSize)
			{
				GrafImageDesc occlusionBufferDesc = {
					GrafImageType::Tex2D,
					GrafFormat::R16G16_UINT,
					targetSize, 1,
					ur_uint(GrafImageUsageFlag::ShaderReadWrite),
					ur_uint(GrafDeviceMemoryFlag::GpuLocal)
				};
				for (ur_uint i = 0; i < 2; ++i)
				{
					GrafImage* prevBuffer = this->occlusionBuffer[i].release();
					if (prevBuffer != ur_null) this->grafRenderer->SafeDelete(prevBuffer, grafCmdList);
					grafSystem->CreateImage(this->occlusionBuffer[i]);
					this->occlusionBuffer[i]->Initialize(grafDevice, { occlusionBufferDesc });
				}

				GrafImageDesc depthBufferDesc = {
					GrafImageType::Tex2D,
					GrafFormat::R32_SFLOAT,
					targetSize, 1,
					ur_uint(GrafImageUsageFlag::ShaderReadWrite),
					ur_uint(GrafDeviceMemoryFlag::GpuLocal)
				};
				for (ur_uint i = 0; i < 2; ++i)
				{
					GrafImage* prevBuffer = this->depthBuffer[i].release();
					if (prevBuffer != ur_null) this->grafRenderer->SafeDelete(prevBuffer, grafCmdList);
					grafSystem->CreateImage(this->depthBuffer[i]);
					this->depthBuffer[i]->Initialize(grafDevice, { depthBufferDesc });
				}
			}

			// reset temporal data

			if (targetSize != crntSize || !this->accumulationEnabled)
			{
				this->viewProjPrev = camera.GetViewProj();
				this->accumulationFrameNumber = 0;
			}
		}

		void BuildTopLevelAccelerationStructure(ur_float elapsedSeconds = 0.0f)
		{
			if (ur_null == this->grafRenderer)
				return;

			GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();

			if (this->animationEnabled) this->animationElapsedTime += elapsedSeconds;
			ur_float crntTimeFactor = (this->animationElapsedTime / this->animationCycleTime);
			ur_float modY;
			ur_float crntCycleFactor = std::modf(crntTimeFactor, &modY);
			ur_float animAngle = (MathConst<ur_float>::Pi * 2.0f) * crntCycleFactor;

			// update instances

			std::vector<GrafAccelerationStructureInstance> sampleInstances;
			GrafAccelerationStructureInstance planeInstance =
			{
				{
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f
				},
				this->planeMesh->GetMeshID(), 0xff, 0, (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable)),
				this->planeMesh->accelerationStructureBL->GetDeviceAddress()
			};
			sampleInstances.emplace_back(planeInstance);
			for (ur_size i = 0; i < this->sampleInstanceCount; ++i)
			{
				ur_float radius = 7.0f;
				ur_float height = 0.0f;
				GrafAccelerationStructureInstance meshInstance =
				{
					{
						1.0f, 0.0f, 0.0f, radius * cosf(ur_float(i) / this->sampleInstanceCount * MathConst<ur_float>::Pi * 2.0f + animAngle),
						0.0f, 1.0f, 0.0f, height + cosf(ur_float(i) / this->sampleInstanceCount * MathConst<ur_float>::Pi * 6.0f + animAngle) * 1.0f,
						0.0f, 0.0f, 1.0f, radius * sinf(ur_float(i) / this->sampleInstanceCount * MathConst<ur_float>::Pi * 2.0f + animAngle)
					},
					this->customMeshes[0]->GetMeshID(), 0xff, 0, (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable)),
					this->customMeshes[0]->accelerationStructureBL->GetDeviceAddress()
				};
				sampleInstances.emplace_back(meshInstance);
			}
			Mesh* customMesh = (this->customMeshes.empty() ? ur_null : this->customMeshes[2].get());
			if (customMesh != ur_null)
			{
				GrafAccelerationStructureInstance meshInstance =
				{
					{
						2.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 2.0f, 0.0f,-2.0f,
						0.0f, 0.0f, 2.0f, 0.0f
					},
					customMesh->GetMeshID(), 0xff, 0, (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable)),
					customMesh->accelerationStructureBL->GetDeviceAddress()
				};
				sampleInstances.emplace_back(meshInstance);
			}

			// write to upload buffer

			ur_size updateSize = sampleInstances.size() * sizeof(GrafAccelerationStructureInstance);
			if (updateSize > this->instanceBuffer->GetDesc().SizeInBytes)
				return; // too many instances

			Allocation uploadAllocation = grafRenderer->GetDynamicUploadBufferAllocation(updateSize);
			GrafBuffer* uploadBuffer = grafRenderer->GetDynamicUploadBuffer();
			uploadBuffer->Write((const ur_byte*)sampleInstances.data(), uploadAllocation.Size, 0, uploadAllocation.Offset);

			// build TLAS on device

			GrafAccelerationStructureInstancesData sampleInstancesData = {};
			sampleInstancesData.IsPointersArray = false;
			sampleInstancesData.DeviceAddress = this->instanceBuffer->GetDeviceAddress();

			GrafAccelerationStructureGeometryData sampleGeometryDataTL = {};
			sampleGeometryDataTL.GeometryType = GrafAccelerationStructureGeometryType::Instances;
			sampleGeometryDataTL.GeometryFlags = ur_uint(GrafAccelerationStructureGeometryFlag::Opaque);
			sampleGeometryDataTL.InstancesData = &sampleInstancesData;
			sampleGeometryDataTL.PrimitiveCount = ur_uint32(sampleInstances.size());

			GrafCommandList* cmdListBuildAccelStructTL = grafRenderer->GetTransientCommandList();
			cmdListBuildAccelStructTL->Begin();
			cmdListBuildAccelStructTL->Copy(uploadBuffer, this->instanceBuffer.get(), updateSize, uploadAllocation.Offset, 0);
			cmdListBuildAccelStructTL->BuildAccelerationStructure(this->accelerationStructureTL.get(), &sampleGeometryDataTL, 1);
			cmdListBuildAccelStructTL->End();
			grafDevice->Record(cmdListBuildAccelStructTL);
			//grafDevice->Submit();
			//grafDevice->WaitIdle();
		}

		void Update(ur_float elapsedSeconds)
		{
			this->BuildTopLevelAccelerationStructure(elapsedSeconds);
		}

		void Render(GrafCommandList* grafCmdList, GrafImage* grafTargetImage, const ur_float4 &clearColor, Camera& camera,
			const LightingDesc& lightingDesc, const Atmosphere::Desc& atmosphereDesc)
		{
			if (ur_null == this->grafRenderer)
				return;

			PrepareAccumulationData(grafCmdList, grafTargetImage, camera);
			ur_uint crntFrameDataId = (this->accumulationFrameNumber % 2);
			ur_uint prevFrameDataId = ((this->accumulationFrameNumber + 1) % 2);

			struct SceneConstants
			{
				ur_float4x4 viewProjInv;
				ur_float4x4 viewProjPrev;
				ur_float4 cameraPos;
				ur_float4 viewportSize;
				ur_float4 clearColor;
				ur_uint accumulationEnabled;
				ur_uint accumulationFrameNumber;
				ur_uint accumulationFrameCount;
				ur_uint _pad0;
				Atmosphere::Desc atmoDesc;
				LightingDesc lightingDesc;
			} cb;
			ur_uint3 targetSize = grafTargetImage->GetDesc().Size;
			cb.viewProjInv = camera.GetViewProjInv();
			cb.viewProjPrev = this->viewProjPrev;
			cb.cameraPos = camera.GetPosition();
			cb.viewportSize.x = (ur_float)targetSize.x;
			cb.viewportSize.y = (ur_float)targetSize.y;
			cb.viewportSize.z = 1.0f / cb.viewportSize.x;
			cb.viewportSize.w = 1.0f / cb.viewportSize.y;
			cb.clearColor = clearColor;
			cb.accumulationEnabled = this->accumulationEnabled;
			cb.accumulationFrameNumber = this->accumulationFrameNumber;
			cb.accumulationFrameCount = this->accumulationFrameCount;
			cb.atmoDesc = atmosphereDesc;
			cb.lightingDesc = lightingDesc;
			GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
			Allocation dynamicCBAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SceneConstants));
			dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

			GrafDescriptorTable* descriptorTable = this->bindingTables[this->grafRenderer->GetCurrentFrameId()].get();
			descriptorTable->SetConstantBuffer(0, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
			descriptorTable->SetAccelerationStructure(0, this->accelerationStructureTL.get());
			descriptorTable->SetBuffer(1, this->vertexBuffer.get());
			descriptorTable->SetBuffer(2, this->indexBuffer.get());
			descriptorTable->SetBuffer(3, this->meshBuffer.get());
			descriptorTable->SetRWImage(0, grafTargetImage);
			descriptorTable->SetRWImage(1, this->occlusionBuffer[crntFrameDataId].get());
			descriptorTable->SetRWImage(2, this->occlusionBuffer[prevFrameDataId].get());
			descriptorTable->SetRWImage(3, this->depthBuffer[crntFrameDataId].get());
			descriptorTable->SetRWImage(4, this->depthBuffer[prevFrameDataId].get());

			grafCmdList->ImageMemoryBarrier(grafTargetImage, GrafImageState::Current, GrafImageState::RayTracingReadWrite);
			grafCmdList->BindRayTracingPipeline(this->pipelineState.get());
			grafCmdList->BindRayTracingDescriptorTable(descriptorTable, this->pipelineState.get());
			grafCmdList->DispatchRays(targetSize.x, targetSize.y, 1, &this->rayGenShaderTable, &this->missShaderTable, &this->hitShaderTable, ur_null);

			// store data for next frame temporal accumulation
			this->viewProjPrev = camera.GetViewProj();
			this->accumulationFrameNumber += 1;
		}

		void ShowImgui()
		{
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::CollapsingHeader("RayTracingScene"))
			{
				ImGui::Checkbox("AccumulationEnabled", &this->accumulationEnabled);
				int editableFrameCount = (int)this->accumulationFrameCount;
				ImGui::InputInt("AccumulationFrames", &editableFrameCount);
				this->accumulationFrameCount = editableFrameCount;
				int instanceCount = (int)this->sampleInstanceCount;
				ImGui::InputInt("InstanceCount", &instanceCount);
				this->sampleInstanceCount = (ur_size)std::max(0, instanceCount);
				ImGui::Checkbox("InstancesAnimationEnabled", &this->animationEnabled);
				ImGui::InputFloat("InstancesCycleTime", &this->animationCycleTime);
			}
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

	// light source animation
	ur_float lightCycleTime = 60.0f;
	ur_float lightCrntCycleFactor = 0.0f;
	ur_bool lightAnimationEnabled = true;
	ur_float lightAnimationElapsedTime = 0.0f;

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

			// upadte light source(s)

			if (lightAnimationEnabled) lightAnimationElapsedTime += elapsedTime;
			else lightAnimationElapsedTime = lightCrntCycleFactor * lightCycleTime;
			ur_float crntTimeFactor = (lightAnimationEnabled ? (lightAnimationElapsedTime / lightCycleTime) : lightCrntCycleFactor);
			ur_float modY;
			lightCrntCycleFactor = std::modf(crntTimeFactor, &modY);
			ur_float3 sunDir;
			sunDir.x = -cos(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor);
			sunDir.z = -sin(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor);
			sunDir.y = -powf(fabs(sin(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor)), 2.0f) * 0.6f - 0.05f;
			sunDir.Normalize();
			LightDesc& sunLight = lightingDesc.LightSources[0];
			sunLight.Direction = sunDir;

			ctx.progress = 2;

			// update ray tracing scene

			if (rayTracingScene != ur_null)
			{
				rayTracingScene->Update(elapsedTime);
			}

			ctx.progress = 3;
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

				updateFrameJob->Wait();

				if (hdrRender != ur_null)
				{
					// HDR & depth render pass

					hdrRender->BeginRender(*grafCmdListCrnt);

					// note: non ray traced rendering can be done here

					hdrRender->EndRender(*grafCmdListCrnt);

					// ray traced scene

					if (rayTracingScene != ur_null)
					{
						rayTracingScene->Render(grafCmdListCrnt, hdrRender->GetHDRRenderTarget()->GetImage(0), (ur_float4&)rtClearValue.f32, camera,
							lightingDesc, atmosphereDesc);
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
						ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
						if (ImGui::CollapsingHeader("Lighting"))
						{
							ImGui::Checkbox("AnimationEnabled", &lightAnimationEnabled);
							ImGui::InputFloat("CycleTime", &lightCycleTime);
							ImGui::DragFloat("CrntCycleFactor", &lightCrntCycleFactor, 0.01f, 0.0f, 1.0f);
							ImGui::ColorEdit3("Color", &lightingDesc.LightSources[0].Color.x);
							ImGui::InputFloat("Intensity", &lightingDesc.LightSources[0].Intensity);
						}
						if (ImGui::CollapsingHeader("Atmosphere"))
						{
							ImGui::InputFloat("InnerRadius", &atmosphereDesc.InnerRadius);
							ImGui::InputFloat("OuterRadius", &atmosphereDesc.OuterRadius);
							ImGui::DragFloat("ScaleDepth", &atmosphereDesc.ScaleDepth, 0.01f, 0.0f, 1.0f);
							ImGui::InputFloat("Kr", &atmosphereDesc.Kr);
							ImGui::InputFloat("Km", &atmosphereDesc.Km);
							ImGui::DragFloat("G", &atmosphereDesc.G, 0.01f, -1.0f, 1.0f);
							ImGui::InputFloat("D", &atmosphereDesc.D);
						}
						if (rayTracingScene != ur_null)
						{
							rayTracingScene->ShowImgui();
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