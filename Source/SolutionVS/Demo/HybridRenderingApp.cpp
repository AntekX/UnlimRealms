
#include "stdafx.h"
#include "HybridRenderingApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Core/Math.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/DX12/GrafSystemDX12.h"
#include "Graf/GrafRenderer.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"
#include "HDRRender/HDRRender.h"
#include "Atmosphere/Atmosphere.h"
#include "HybridRenderingCommon.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

#define UR_GRAF_SYSTEM GrafSystemDX12

#define SCENE_TYPE_MEDIEVAL_BUILDINGS 0
#define SCENE_TYPE_SPONZA 1
#define SCENE_TYPE_SANMIGUEL 2
#define SCENE_TYPE_RUNGHOLT 3
#define SCENE_TYPE_FOREST 4
#define SCENE_TYPE SCENE_TYPE_SPONZA

#define UPDATE_ASYNC 1
#define RENDER_ASYNC 1

struct SpatialFilterSettings
{
	ur_uint PassCount;
	ur_float4 EdgeParams;
};

struct TemporalFilterSettings
{
	ur_uint AccumulationFrames;
	ur_float Tolerance;
	ur_float Threshold;
};

struct DirectShadowSettings
{
	ur_uint SamplesPerLight = 1;
	TemporalFilterSettings TemporalFilter = {
		16, // AccumulationFrames
		0.1f, // Tolerance
		0.5f, // Threshold
	};
	SpatialFilterSettings SpatialFilter = {
		2, // PassCount
		ur_float4(0.1f, 0.02f, 0.0f, 0.0f) // EdgeParams
	};
};

struct IndirectLightSettings
{
	ur_uint SamplesPerFrame = 1;
	ur_uint BouncesCount = 1;
	TemporalFilterSettings TemporalFilter = {
		64, // AccumulationFrames
		0.02f, // Tolerance
		0.80f, // Threshold
	};
	SpatialFilterSettings SpatialFilter = {
		4, // PassCount
		ur_float4(0.1f, 0.02f, 0.0f, 0.0f) // EdgeParams
	};
	SpatialFilterSettings SpatialPostFilter = {
		0, // PassCount
		ur_float4(0.1f, 0.02f, 0.0f, 0.0f) // EdgeParams
	};
};

struct RayTracingSettings
{
	// TODO: fix reprojection for downscaled light buffer
	// consider bilateral upscale of ray tracing result to full res before accumulation pass
	ur_uint LightingBufferDownscale = 1;
	ur_bool PerFrameJitter = true;
	DirectShadowSettings Shadow;
	IndirectLightSettings IndirectLight;
};

struct Settings
{
	ur_uint3 SkyImageSize = ur_uint3(512, 512, 1);
	RayTracingSettings RayTracing;
} g_Settings;

enum class SceneState : int
{
	Initialize = 0,
	Run,
	Finish,
	RecreateForVulkan,
	RecreateForDX12,
};

SceneState RunDemoScene(
	std::unique_ptr<GrafSystem> grafSystem,
	// TODO: make it a member function and declare all inputs as HybridRenderingApp members
	Realm& realm,
	LightDesc& sunLight, LightDesc& sunLight2, LightDesc& sphericalLight1, LightDesc* sphericalLight1Ptr, LightingDesc& lightingDesc,
	ur_float& lightCycleTime, ur_float& lightCrntCycleFactor, ur_bool& lightAnimationEnabled, ur_float& lightAnimationElapsedTime,
	AtmosphereDesc& atmosphereDesc,
	Camera& camera, CameraControl& cameraControl);

int HybridRenderingApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"HybridRenderingDemo"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// light source params
	LightDesc sunLight = {}; // main directional light source
	sunLight.Type = LightType_Directional;
	sunLight.Color = { 1.0f, 1.0f, 1.0f };
	sunLight.Intensity = SolarIlluminanceNoon;
	sunLight.IntensityTopAtmosphere = SolarIlluminanceTopAtmosphere;
	sunLight.Direction = { -0.8165f,-0.40825f,-0.40825f };
	sunLight.Size = SolarDiskHalfAngleTangent * 2.0f;
	LightDesc sunLight2 = {}; // artificial second star
	sunLight2.Type = LightType_Directional;
	sunLight2.Color = { 1.0f, 0.1f, 0.0f };
	sunLight2.Intensity = SolarIlluminanceNoon * 0.5;
	sunLight2.IntensityTopAtmosphere = SolarIlluminanceTopAtmosphere * 0.5;
	sunLight2.Direction = { 0.8018f,-0.26726f,-0.5345f };
	sunLight2.Size = SolarDiskHalfAngleTangent * 4.0f;
	LightDesc sphericalLight1 = {}; // main directional light source
	LightDesc* sphericalLight1Ptr = ur_null;
	sphericalLight1.Type = LightType_Spherical;
	sphericalLight1.Color = { 1.0f, 1.0f, 1.0f };
	#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE) || (SCENE_TYPE_FOREST == SCENE_TYPE)
	sphericalLight1.Position = { 10.0f, 10.0f, 10.0f };
	sphericalLight1.Intensity = SolarIlluminanceNoon * powf(sphericalLight1.Position.y, 2) * 2; // match illuminance to day light
	#elif (SCENE_TYPE_SPONZA == SCENE_TYPE)
	sphericalLight1.Position = { 2.0f, 2.0f, 2.0f };
	sphericalLight1.Intensity = SolarIlluminanceNoon * powf(sphericalLight1.Position.y, 2) * 1;
	#endif
	sphericalLight1.Size = 1.0f;
	LightingDesc lightingDesc = {};
	lightingDesc.LightSources[lightingDesc.LightSourceCount++] = sunLight;
	#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE)
	sphericalLight1Ptr = &lightingDesc.LightSources[lightingDesc.LightSourceCount];
	lightingDesc.LightSources[lightingDesc.LightSourceCount++] = sphericalLight1;
	#endif
	//lightingDesc.LightSources[lightingDesc.LightSourceCount++] = sunLight2;

	// light source animation
	ur_float lightCycleTime = 1600.0f;
	ur_float lightCrntCycleFactor = 0.16f;
	ur_bool lightAnimationEnabled = true;
	ur_float lightAnimationElapsedTime = 20.0f;

	// atmosphere params
	// temp: super low Mie & G to fake sun disc
	AtmosphereDesc atmosphereDesc = {
		6371.0e+3f,
		6381.0e+3f,
		0.250f,
		-0.980f,
		3.0e-8f,
		7.0e-7f,
		2.718f,
	};

	// setup main camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::FixedUp);
	cameraControl.SetTargetPoint(ur_float3(-10.0f, 2.0f, -1.0f));
	cameraControl.SetSpeed(4.0);
	camera.SetProjection(0.1f, 1.0e+4f, camera.GetFieldOFView(), camera.GetAspectRatio());
	#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE)
	camera.SetPosition(ur_float3(9.541f, 5.412f, -12.604f));
	#elif (SCENE_TYPE_SPONZA == SCENE_TYPE)
	camera.SetPosition(ur_float3(9.541f, 1.8f, -12.604f));
	#elif (SCENE_TYPE_SANMIGUEL == SCENE_TYPE)
	camera.SetPosition(ur_float3(10.0f, 1.8f, -13.0f));
	cameraControl.SetTargetPoint(ur_float3(5.0f, 1.8f, -15.0f));
	#elif (SCENE_TYPE_RUNGHOLT == SCENE_TYPE)
	camera.SetPosition(ur_float3(80.0f, 25.0f, -80.0f));
	#elif (SCENE_TYPE_FOREST == SCENE_TYPE)
	camera.SetPosition(ur_float3(6.0f, 2.0f, -14.0f));
	#endif
	camera.SetLookAt(cameraControl.GetTargetPoint(), cameraControl.GetWorldUp());

	SceneState sceneState = SceneState::Initialize;
	do
	{
		// (re)create GRAF system
		std::unique_ptr<GrafSystem> grafSystem;
		switch (sceneState)
		{
		case SceneState::Initialize: grafSystem.reset(new UR_GRAF_SYSTEM(realm)); break;
		case SceneState::RecreateForVulkan: grafSystem.reset(new GrafSystemVulkan(realm)); break;
		case SceneState::RecreateForDX12: grafSystem.reset(new GrafSystemDX12(realm)); break;
		};
		Result res = grafSystem->Initialize(realm.GetCanvas());
		if (Failed(res)) break;

		// run demo scene
		sceneState = RunDemoScene(
			std::move(grafSystem),
			realm,
			sunLight, sunLight2, sphericalLight1, sphericalLight1Ptr, lightingDesc,
			lightCycleTime, lightCrntCycleFactor, lightAnimationEnabled, lightAnimationElapsedTime,
			atmosphereDesc,
			camera, cameraControl);

	} while (SceneState::Finish != sceneState);

	return 0;
}

SceneState RunDemoScene(
	std::unique_ptr<GrafSystem> grafSystem,
	Realm& realm,
	LightDesc& sunLight, LightDesc& sunLight2, LightDesc& sphericalLight1, LightDesc* sphericalLight1Ptr, LightingDesc& lightingDesc,
	ur_float& lightCycleTime, ur_float& lightCrntCycleFactor, ur_bool& lightAnimationEnabled, ur_float& lightAnimationElapsedTime,
	AtmosphereDesc& atmosphereDesc,
	Camera& camera, CameraControl& cameraControl)
{
	ur_float canvasScale = 1.0f;
	ur_uint canvasWidth = ur_uint(realm.GetCanvas()->GetClientBound().Width() * canvasScale);
	ur_uint canvasHeight = ur_uint(realm.GetCanvas()->GetClientBound().Height() * canvasScale);
	ur_bool canvasValid = (realm.GetCanvas()->GetClientBound().Area() > 0);
	ur_bool canvasChanged = false;

	// create GRAF renderer
	GrafRenderer *grafRenderer = realm.AddComponent<GrafRenderer>(realm);
	Result res = Success;
	do
	{
		// initialize renderer
		GrafRenderer::InitParams grafRendererParams = GrafRenderer::InitParams::Default;
		grafRendererParams.DeviceId = grafSystem->GetRecommendedDeviceId();
		grafRendererParams.CanvasParams = GrafCanvas::InitParams::Default;
		grafRendererParams.CanvasParams.PresentMode = GrafPresentMode::Immediate;
		grafRendererParams.DynamicUploadBufferSize = 1024 * (1 << 20);
		res = grafRenderer->Initialize(std::move(grafSystem), grafRendererParams);
		if (Failed(res)) break;

	} while (false);
	if (Failed(res))
	{
		realm.RemoveComponent<GrafRenderer>();
		grafRenderer = ur_null;
		realm.GetLog().WriteLine("HybridRenderingApp: failed to initialize GrafRenderer", Log::Error);
	}

	// initialize GRAF objects

	enum RenderTargetImageUsage
	{
		RenderTargetImageUsage_Depth = 0,
		RenderTargetImageUsage_Geometry0, // xyz: baseColor; w: materialTypeID;
		RenderTargetImageUsage_Geometry1, // xyz: worldNormal; w: unused;
		RenderTargetImageUsage_Geometry2, // X: roughness; y: reflectance; zw: unused;
		RenderTargetImageCount,
		RenderTargetColorImageCount = RenderTargetImageCount - 1,
		
		RenderTargetHistoryFirst = RenderTargetImageCount,
		RenderTargetImageUsage_DepthHistory = RenderTargetHistoryFirst,
		RenderTargetImageCountWithHistory,
		RenderTargetFrameCount = 2
	};
	
	static const GrafFormat RenderTargetImageFormat[RenderTargetImageCount] = {
		GrafFormat::D24_UNORM_S8_UINT,
		GrafFormat::R8G8B8A8_UNORM,
		GrafFormat::R32G32B32A32_SFLOAT,
		GrafFormat::R8G8B8A8_UNORM,
	};

	static GrafClearValue RenderTargetClearValues[RenderTargetImageCount] = {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
	};

	enum LightingImageUsage
	{
		LightingImageUsage_DirectShadow = 0,
		LightingImageUsage_TracingInfo, // x = sub sample pos, y = counter
		LightingImageUsage_DirectShadowBlured,
		LightingImageUsage_IndirectLight,
		LightingImageUsage_IndirectLightBlured,
		LightingImageCount,
		LightingImageHistoryFirst = LightingImageCount,
		LightingImageUsage_DirectShadowHistory = LightingImageHistoryFirst,
		LightingImageUsage_TracingInfoHistory,
		LightingImageUsage_DirectShadowBluredHistory,
		LightingImageUsage_IndirectLightHistory,
		LightingImageUsage_IndirectLightBluredHistory,
		LightingImageCountWithHistory,
	};

	static const GrafFormat LightingImageFormat[LightingImageCount] = {
		GrafFormat::R16G16B16A16_UNORM,
		GrafFormat::R8G8B8A8_UINT,
		GrafFormat::R16G16B16A16_UNORM,
		GrafFormat::R16G16B16A16_SFLOAT,
		GrafFormat::R16G16B16A16_SFLOAT,
	};

	static const ur_int LightingImageGenerateMips[LightingImageCount] = {
		5,
		1,
		1,
		1,
		1,
	};

	static GrafClearValue LightingBufferClearValues[LightingImageCount] = {
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
	};

	struct RenderTargetSet
	{
		std::unique_ptr<GrafImage> imageStorage[RenderTargetImageCountWithHistory];
		std::unique_ptr<GrafRenderTarget> renderTargetStorage[RenderTargetFrameCount];
		GrafImage* images[RenderTargetImageCount * RenderTargetFrameCount];
		GrafRenderTarget* renderTarget;
		ur_bool resetHistory;
	};

	struct LightingBufferSet
	{
		std::unique_ptr<GrafImage> images[LightingImageCountWithHistory];
		std::vector<std::unique_ptr<GrafImageSubresource>> subresources[LightingImageCountWithHistory];
		std::unique_ptr<GrafImageSubresource> mipsSubresource[LightingImageCountWithHistory];
	};

	static const ur_uint BlurPassCountPerFrame = 32; // reserved descriptor tables per frame

	std::unique_ptr<GrafManagedCommandList> managedCommandList;
	std::unique_ptr<GrafRenderPass> rasterRenderPass;
	std::unique_ptr<RenderTargetSet> renderTargetSet;
	std::unique_ptr<LightingBufferSet> lightingBufferSet;

	auto DestroyFrameBufferObjects = [&](GrafCommandList* syncCmdList = ur_null)
	{
		if (renderTargetSet != ur_null)
		{
			if (syncCmdList != ur_null)
			{
				auto objectToDestroy = renderTargetSet.release();
				grafRenderer->AddCommandListCallback(syncCmdList, {}, [objectToDestroy](GrafCallbackContext& ctx) -> Result
				{
					delete objectToDestroy;
					return Result(Success);
				});
			}
			renderTargetSet.reset();
		}
		if (lightingBufferSet != ur_null)
		{
			if (syncCmdList != ur_null)
			{
				auto objectToDestroy = lightingBufferSet.release();
				grafRenderer->AddCommandListCallback(syncCmdList, {}, [objectToDestroy](GrafCallbackContext& ctx) -> Result
				{
					delete objectToDestroy;
					return Result(Success);
				});
			}
			lightingBufferSet.reset();
		}
	};

	auto InitFrameBufferObjects = [&](ur_uint width, ur_uint height) -> Result
	{
		Result res(Success);
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		DestroyFrameBufferObjects(ur_null);

		// rasterization buffer images
		renderTargetSet.reset(new RenderTargetSet());
		for (ur_uint imageId = 0; imageId < RenderTargetImageCount * RenderTargetFrameCount; ++imageId)
		{
			ur_uint imageUsage = (imageId % RenderTargetImageCount);
			ur_uint imageFrame = (imageId / RenderTargetImageCount);
			if (imageId >= RenderTargetImageCountWithHistory)
			{
				// no history for this image type, reference image from frame zero
				renderTargetSet->images[imageUsage + imageFrame * RenderTargetImageCount] = renderTargetSet->imageStorage[imageUsage].get();
				continue;
			}
			
			auto& image = renderTargetSet->imageStorage[imageId];
			res = grafSystem->CreateImage(image);
			if (Failed(res)) return res;
			ur_uint imageUsageFlags = ur_uint(GrafImageUsageFlag::TransferDst) | ur_uint(GrafImageUsageFlag::ShaderRead);
			if (RenderTargetImageUsage_Depth == imageUsage)
			{
				imageUsageFlags |= ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget);
			}
			else
			{
				imageUsageFlags |= ur_uint(GrafImageUsageFlag::ColorRenderTarget);
			}
			GrafImageDesc imageDesc = {
				GrafImageType::Tex2D,
				RenderTargetImageFormat[imageUsage],
				ur_uint3(width, height, 1), 1,
				imageUsageFlags,
				ur_uint(GrafDeviceMemoryFlag::GpuLocal)
			};
			res = image->Initialize(grafDevice, { imageDesc });
			if (Failed(res)) return res;
			renderTargetSet->images[imageUsage + imageFrame * RenderTargetImageCount] = image.get();
		}

		// rasterization pass render target(s)
		for (ur_uint frameId = 0; frameId < RenderTargetFrameCount; ++frameId)
		{
			res = grafSystem->CreateRenderTarget(renderTargetSet->renderTargetStorage[frameId]);
			if (Failed(res)) return res;
			
			GrafRenderTarget::InitParams renderTargetParams = {
				rasterRenderPass.get(),
				&renderTargetSet->images[frameId * RenderTargetImageCount],
				RenderTargetImageCount
			};
			res = renderTargetSet->renderTargetStorage[frameId]->Initialize(grafDevice, renderTargetParams);
			if (Failed(res)) return res;
		}
		renderTargetSet->renderTarget = renderTargetSet->renderTargetStorage[0].get();

		// lighting buffer images
		ur_uint lightingBufferWidth = width / std::max(1u, g_Settings.RayTracing.LightingBufferDownscale);
		ur_uint lightingBufferHeight = height / std::max(1u, g_Settings.RayTracing.LightingBufferDownscale);
		ur_uint lightBufferMipCount = (ur_uint)std::log2f(ur_float(std::max(lightingBufferWidth, lightingBufferHeight))) + 1;
		lightingBufferSet.reset(new LightingBufferSet());
		for (ur_uint imageId = 0; imageId < LightingImageCountWithHistory; ++imageId)
		{
			ur_uint imageUsage = (imageId % LightingImageCount);
			auto& image = lightingBufferSet->images[imageId];
			ur_uint imageMipCount = ur_uint(LightingImageGenerateMips[imageUsage] < 1 ? lightBufferMipCount : std::min((ur_uint)LightingImageGenerateMips[imageUsage], lightBufferMipCount));

			res = grafSystem->CreateImage(image);
			if (Failed(res)) return res;

			GrafImageDesc imageDesc = {
				GrafImageType::Tex2D,
				LightingImageFormat[imageUsage],
				ur_uint3(lightingBufferWidth, lightingBufferHeight, 1), imageMipCount,
				ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::TransferDst) | ur_uint(GrafImageUsageFlag::ShaderRead) | ur_uint(GrafImageUsageFlag::ShaderReadWrite),
				ur_uint(GrafDeviceMemoryFlag::GpuLocal)
			};
			res = image->Initialize(grafDevice, { imageDesc });
			if (Failed(res)) return res;

			// subreources for mip chain generation
			auto& imageSubresources = lightingBufferSet->subresources[imageId];
			imageSubresources.resize(imageMipCount);
			for (ur_uint mipId = 0; mipId < imageMipCount; ++mipId)
			{
				auto& imageMip = imageSubresources[mipId];
				res = grafSystem->CreateImageSubresource(imageMip);
				if (Failed(res)) return res;

				GrafImageSubresourceDesc imageMipDesc = {
					mipId, 1, 0, 1
				};
				res = imageMip->Initialize(grafDevice, { image.get(), imageMipDesc });
				if (Failed(res)) return res;
			}

			// common subresources for all mips (except zero)
			if (imageMipCount > 1)
			{
				auto& mipsSubresource = lightingBufferSet->mipsSubresource[imageId];
				res = grafSystem->CreateImageSubresource(mipsSubresource);
				if (Failed(res)) return res;
				GrafImageSubresourceDesc subresourceDesc = {
					1, imageMipCount - 1, 0, 1
				};
				res = mipsSubresource->Initialize(grafDevice, { image.get(), subresourceDesc });
				if (Failed(res)) return res;
			}
		}

		renderTargetSet->resetHistory = true;

		return res;
	};

	auto DestroyGrafObjects = [&]()
	{
		// here we expect that device is idle
		DestroyFrameBufferObjects();
		rasterRenderPass.reset();
		managedCommandList.reset();
	};

	auto InitGrafObjects = [&]() -> Result
	{
		Result res(Success);
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// main command list (managed per frame)
		managedCommandList.reset(new GrafManagedCommandList(*grafRenderer));
		res = managedCommandList->Initialize();
		if (Failed(res)) return res;

		// rasterization render pass
		res = grafSystem->CreateRenderPass(rasterRenderPass);
		if (Failed(res)) return res;
		GrafRenderPassImageDesc rasterRenderPassImageDesc[RenderTargetImageCount];
		for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
		{
			auto& passImageDesc = rasterRenderPassImageDesc[imageId];
			RenderTargetImageUsage imageUsage = RenderTargetImageUsage(imageId);
			passImageDesc.Format = RenderTargetImageFormat[imageId];
			passImageDesc.InitialState = (RenderTargetImageUsage_Depth == imageUsage ? GrafImageState::DepthStencilWrite : GrafImageState::ColorWrite);
			passImageDesc.FinalState = passImageDesc.InitialState;
			passImageDesc.LoadOp = GrafRenderPassDataOp::Load;
			passImageDesc.StoreOp = GrafRenderPassDataOp::Store;
			passImageDesc.StencilLoadOp = GrafRenderPassDataOp::DontCare;
			passImageDesc.StencilStoreOp = GrafRenderPassDataOp::DontCare;
		}
		res = rasterRenderPass->Initialize(grafDevice, { rasterRenderPassImageDesc, ur_array_size(rasterRenderPassImageDesc) });
		if (Failed(res)) return res;

		return res;
	};

	res = InitGrafObjects();
	if (Succeeded(res))
	{
		res = InitFrameBufferObjects(canvasWidth, canvasHeight);
	}
	if (Failed(res))
	{
		DestroyGrafObjects();
		realm.GetLog().WriteLine("HybridRenderingApp: failed to initialize graphics object(s)", Log::Error);
	}

	// initialize demo scene
	class DemoScene : public RealmEntity
	{
	public:

		typedef GrafAccelerationStructureInstance Instance; // HW ray tracing compatible instance structure

		enum MeshId
		{
			MeshId_Plane = 0,
			MeshId_Sphere,
			MeshId_Wuson,
			#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE)
			MeshId_MedievalBuilding,
			#elif (SCENE_TYPE_SPONZA == SCENE_TYPE)
			MeshId_Sponza,
			#elif (SCENE_TYPE_SANMIGUEL == SCENE_TYPE)
			MeshId_SanMiguel,
			#elif (SCENE_TYPE_RUNGHOLT == SCENE_TYPE)
			MeshId_Rungholt,
			#elif (SCENE_TYPE_FOREST == SCENE_TYPE)
			MeshId_Banana,
			MeshId_BananaSmall,
			MeshId_House,
			#endif
			MeshCount
		};

		class Mesh : public GrafEntity
		{
		public:

			struct Vertex
			{
				ur_float3 Pos;
				ur_float3 Norm;
				ur_float3 Tangent;
				ur_float2 TexCoord;
			};

			typedef ur_uint32 Index;

			struct SubMesh
			{
				ur_uint primitivesOffset;
				ur_uint primitivesCount;
				ur_uint materialID;
				std::unique_ptr<GrafImage> colorImage;
				std::unique_ptr<GrafImage> maskImage;
				std::unique_ptr<GrafImage> normalImage;
				std::unique_ptr<GrafManagedDescriptorTable> descriptorTable;
				std::unique_ptr<GrafAccelerationStructure> accelerationStructureBL;
				ur_uint64 accelerationStructureHandle;
				ur_uint32 gpuRegistryIdx;
				ur_uint instanceCount;
				ur_uint instanceOfs;

				inline ur_uint64 GetBLASHandle() const { return this->accelerationStructureHandle; }
				inline ur_uint32 GetGpuRegistryIdx() const { return this->gpuRegistryIdx; }
			};

			MeshId meshId;
			std::unique_ptr<GrafBuffer> vertexBuffer;
			std::unique_ptr<GrafBuffer> indexBuffer;
			ur_uint verticesCount;
			ur_uint indicesCount;
			std::vector<MeshMaterialDesc> materials;
			std::vector<SubMesh> subMeshes;

			Mesh(GrafSystem &grafSystem) : GrafEntity(grafSystem) {}
			~Mesh() {}

			void Initialize(DemoScene* demoScene, MeshId meshId, const GrafUtils::MeshData& meshData)
			{
				GrafRenderer* grafRenderer = demoScene->grafRenderer;
				GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
				GrafDevice* grafDevice = grafRenderer->GetGrafDevice();
				const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());

				this->meshId = meshId;
				this->verticesCount = ur_uint(meshData.Vertices.size() / sizeof(Mesh::Vertex));
				this->indicesCount = ur_uint(meshData.Indices.size() / sizeof(Mesh::Index));
				ur_size vertexStride = sizeof(Vertex);
				ur_size indexStride = sizeof(Index);
				GrafIndexType indexType = (indexStride == 2 ? GrafIndexType::UINT16 : GrafIndexType::UINT32);
				Mesh::Vertex* vertices = (Mesh::Vertex*)meshData.Vertices.data();
				Mesh::Index* indices = (Mesh::Index*)meshData.Indices.data();

				// vertex attributes buffer

				GrafBuffer::InitParams VBParams;
				VBParams.BufferDesc.Usage =
					ur_uint(GrafBufferUsageFlag::VertexBuffer) |
					ur_uint(GrafBufferUsageFlag::TransferDst) |
					ur_uint(GrafBufferUsageFlag::StorageBuffer) |
					ur_uint(GrafBufferUsageFlag::RayTracing) |
					ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
				VBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
				VBParams.BufferDesc.SizeInBytes = verticesCount * vertexStride;
				VBParams.BufferDesc.ElementSize = vertexStride;

				grafSystem->CreateBuffer(this->vertexBuffer);
				this->vertexBuffer->Initialize(grafDevice, VBParams);
				
				grafRenderer->Upload((ur_byte*)vertices, this->vertexBuffer.get(), VBParams.BufferDesc.SizeInBytes);

				// index buffer

				GrafBuffer::InitParams IBParams;
				IBParams.BufferDesc.Usage =
					ur_uint(GrafBufferUsageFlag::IndexBuffer) |
					ur_uint(GrafBufferUsageFlag::TransferDst) |
					ur_uint(GrafBufferUsageFlag::StorageBuffer) |
					ur_uint(GrafBufferUsageFlag::RayTracing) |
					ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
				IBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
				IBParams.BufferDesc.SizeInBytes = indicesCount * indexStride;
				IBParams.BufferDesc.ElementSize = indexStride;

				grafSystem->CreateBuffer(this->indexBuffer);
				this->indexBuffer->Initialize(grafDevice, IBParams);

				grafRenderer->Upload((ur_byte*)indices, this->indexBuffer.get(), IBParams.BufferDesc.SizeInBytes);

				// mesh materials

				this->materials.reserve(meshData.Materials.size());
				for (auto& srcMaterial : meshData.Materials)
				{
					this->materials.emplace_back();
					auto& material = this->materials.back();
					material = {};
					material.BaseColor = srcMaterial.BaseColor;
					material.Roughness = srcMaterial.Roughness;
					material.EmissiveColor = srcMaterial.EmissiveColor;
					material.Metallic = srcMaterial.Metallic;
					material.SheenColor = srcMaterial.SheenColor;
					material.Reflectance = srcMaterial.Reflectance;
					material.AnisotropyDirection = srcMaterial.AnisotropyDirection;
					material.Anisotropy = srcMaterial.Anisotropy;
					material.ClearCoat = srcMaterial.ClearCoat;
					material.ClearCoatRoughness = srcMaterial.ClearCoatRoughness;
				}

				// per material sub mesh data

				auto initializeGrafImage = [](GrafRenderer* grafRenderer, GrafCommandList* uploadCmdList,
					const std::string& resName, std::unique_ptr<GrafImage>& resImage) -> Result
				{
					GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
					GrafDevice* grafDevice = grafRenderer->GetGrafDevice();

					if (resName.empty())
						return Result(InvalidArgs);

					std::unique_ptr<GrafUtils::ImageData> imageData(new GrafUtils::ImageData());
					Result result = GrafUtils::LoadImageFromFile(*grafDevice, resName, *imageData);
					if (Failed(result))
						return result;

					grafSystem->CreateImage(resImage);
					result = resImage->Initialize(grafDevice, { imageData->Desc });
					if (Failed(result))
					{
						resImage.reset();
						return result;
					}

					std::vector<std::unique_ptr<GrafImageSubresource>> resImageMips;
					resImageMips.resize(imageData->Desc.MipLevels);
					for (ur_uint imip = 0; imip < imageData->Desc.MipLevels; ++imip)
					{
						auto& imageMip = resImageMips[imip];
						GrafImageSubresourceDesc mipDesc = {
							imip, 1, 0, 1
						};
						grafSystem->CreateImageSubresource(imageMip);
						imageMip->Initialize(grafDevice, { resImage.get(), mipDesc });
					}

					ur_uint3 mipSize = imageData->Desc.Size;
					for (ur_uint imip = 0; imip < imageData->Desc.MipLevels; ++imip)
					{
						auto& imageMip = resImageMips[imip];
						uploadCmdList->ImageMemoryBarrier(imageMip.get(), GrafImageState::Current, GrafImageState::TransferDst);
						uploadCmdList->Copy(imageData->MipBuffers[imip].get(), imageMip.get());
					}
					uploadCmdList->ImageMemoryBarrier(resImage.get(), GrafImageState::TransferDst, GrafImageState::ShaderRead);
					GrafUtils::ImageData* imageDataPtr = imageData.release();
					grafRenderer->AddCommandListCallback(uploadCmdList, { imageDataPtr }, [](GrafCallbackContext& ctx) -> Result
					{
						GrafUtils::ImageData* imageDataPtr = reinterpret_cast<GrafUtils::ImageData*>(ctx.DataPtr);
						delete imageDataPtr;
						return Result(Success);
					});

					return result;
				};

				GrafCommandList* cmdList = grafRenderer->GetTransientCommandList();
				cmdList->Begin();
				subMeshes.resize(meshData.Surfaces.size());
				for (ur_size surfaceIdx = 0; surfaceIdx < meshData.Surfaces.size(); ++surfaceIdx)
				{
					auto& surfaceData = meshData.Surfaces[surfaceIdx];
					auto& subMesh = subMeshes[surfaceIdx];
					auto& materialDesc = meshData.Materials[surfaceData.MaterialID];
					
					subMesh.primitivesOffset = surfaceData.PrimitivesOffset;
					subMesh.primitivesCount = surfaceData.PrimtivesCount;
					subMesh.materialID = surfaceData.MaterialID;
					subMesh.accelerationStructureHandle = 0;
					subMesh.gpuRegistryIdx = ur_uint32(-1);
					subMesh.instanceCount = 0;
					subMesh.instanceOfs = ur_uint32(-1);

					// textures

					initializeGrafImage(grafRenderer, cmdList, materialDesc.ColorTexName, subMesh.colorImage);
					initializeGrafImage(grafRenderer, cmdList, materialDesc.MaskTexName, subMesh.maskImage);
					initializeGrafImage(grafRenderer, cmdList, materialDesc.NormalTexName, subMesh.normalImage);

					if (subMesh.colorImage != ur_null)
					{
						// use material color only if there is no color texture
						// otherwise set it to white (colors are multiplied in shaders)
						this->materials[subMesh.materialID].BaseColor = 1.0f;
					}

					// descriptor table

					subMesh.descriptorTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
					subMesh.descriptorTable->Initialize({ demoScene->rasterDescTableLayout.get() });

					// ray tracing data

					if (grafDeviceDesc->RayTracing.RayTraceSupported)
					{
						// initialize bottom level acceleration structure container

						GrafAccelerationStructureGeometryDesc accelStructGeomDescBL = {};
						accelStructGeomDescBL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
						accelStructGeomDescBL.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
						accelStructGeomDescBL.VertexStride = ur_uint32(vertexStride);
						accelStructGeomDescBL.IndexType = indexType;
						accelStructGeomDescBL.PrimitiveCountMax = ur_uint32(subMesh.primitivesCount);
						accelStructGeomDescBL.VertexCountMax = ur_uint32(verticesCount);
						accelStructGeomDescBL.TransformsEnabled = false;

						GrafAccelerationStructure::InitParams accelStructParamsBL = {};
						accelStructParamsBL.StructureType = GrafAccelerationStructureType::BottomLevel;
						accelStructParamsBL.BuildFlags = GrafAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlag::PreferFastTrace);
						accelStructParamsBL.Geometry = &accelStructGeomDescBL;
						accelStructParamsBL.GeometryCount = 1;

						grafSystem->CreateAccelerationStructure(subMesh.accelerationStructureBL);
						subMesh.accelerationStructureBL->Initialize(grafDevice, accelStructParamsBL);
						subMesh.accelerationStructureHandle = subMesh.accelerationStructureBL->GetDeviceAddress();

						// build bottom level acceleration structure(s)

						GrafAccelerationStructureTrianglesData trianglesData = {};
						trianglesData.VertexFormat = GrafFormat::R32G32B32_SFLOAT;
						trianglesData.VertexStride = ur_uint32(vertexStride);
						trianglesData.VertexCount = ur_uint32(verticesCount);
						trianglesData.VerticesDeviceAddress = vertexBuffer->GetDeviceAddress();
						trianglesData.IndexType = indexType;
						trianglesData.IndicesDeviceAddress = indexBuffer->GetDeviceAddress();

						GrafAccelerationStructureGeometryData geometryDataBL = {};
						geometryDataBL.GeometryType = GrafAccelerationStructureGeometryType::Triangles;
						geometryDataBL.GeometryFlags = ur_uint(GrafAccelerationStructureGeometryFlag::Opaque);
						geometryDataBL.TrianglesData = &trianglesData;
						geometryDataBL.PrimitiveCount = ur_uint32(subMesh.primitivesCount);
						geometryDataBL.PrimitivesOffset = ur_uint32(subMesh.primitivesOffset * indexStride);
						geometryDataBL.FirstVertexIndex = 0;

						cmdList->BuildAccelerationStructure(subMesh.accelerationStructureBL.get(), &geometryDataBL, 1);
					}
				}
				cmdList->End();
				grafDevice->Record(cmdList);

				// add to gpu resource registry for dynamic indexing
				demoScene->gpuResourceRegistry->AddMesh(this);
			}
		};

		class GPUResourceRegistry : public GrafEntity
		{
		public:

			GPUResourceRegistry(DemoScene& scene) :
				GrafEntity(*scene.grafRenderer->GetGrafSystem()),
				scene(scene)
			{
				this->subMeshDescArray.reserve(16);
			}
			
			~GPUResourceRegistry()
			{
			}

			void Initialize()
			{
				// register fallback textures
				this->defaultImageWhiteIdx = AddImage2D(scene.defaultImageWhite.get());
				this->defaultImageBlackIdx = AddImage2D(scene.defaultImageBlack.get());
				this->defaultImageNormalIdx = AddImage2D(scene.defaultImageNormal.get());
			}

			ur_uint32 AddImage2D(GrafImage* grafImage)
			{
				this->image2DArray.emplace_back(grafImage);
				return ur_uint32(this->image2DArray.size() - 1);
			}

			ur_uint32 AddBuffer(GrafBuffer* grafBuffer)
			{
				this->bufferArray.emplace_back(grafBuffer);
				return ur_uint32(this->bufferArray.size() - 1);
			}

			ur_uint32 AddMaterial(const MeshMaterialDesc& materialDesc)
			{
				this->materialDescArray.emplace_back(materialDesc);
				return ur_uint32(this->materialDescArray.size() - 1);
			}

			void AddMesh(Mesh* mesh)
			{
				ur_uint32 meshVBDescriptorIdx = AddBuffer(mesh->vertexBuffer.get());
				ur_uint32 meshIBDescriptorIdx = AddBuffer(mesh->indexBuffer.get());
				ur_uint32 meshFirstMaterialIdx = AddMaterial(mesh->materials[0]); // at least one default material always exists
				for (ur_size imat = 1; imat < mesh->materials.size(); ++imat)
				{
					AddMaterial(mesh->materials[imat]);
				}
				for (Mesh::SubMesh& subMesh : mesh->subMeshes)
				{
					SubMeshDesc subMeshDesc = {};
					subMeshDesc.VertexBufferDescriptor = meshVBDescriptorIdx;
					subMeshDesc.IndexBufferDescriptor = meshIBDescriptorIdx;
					subMeshDesc.PrimitivesOffset = subMesh.primitivesOffset;
					subMeshDesc.ColorMapDescriptor = (subMesh.colorImage.get() ? AddImage2D(subMesh.colorImage.get()) : this->defaultImageWhiteIdx);
					subMeshDesc.NormalMapDescriptor = (subMesh.normalImage.get() ? AddImage2D(subMesh.normalImage.get()) : this->defaultImageNormalIdx);
					subMeshDesc.MaskMapDescriptor = (subMesh.maskImage.get() ? AddImage2D(subMesh.maskImage.get()) : this->defaultImageWhiteIdx);
					subMeshDesc.MaterialBufferIndex = meshFirstMaterialIdx + subMesh.materialID;
					this->subMeshDescArray.emplace_back(subMeshDesc);
					subMesh.gpuRegistryIdx = ur_uint32(this->subMeshDescArray.size() - 1);
				}
			}

			void Upload()
			{
				GrafRenderer* grafRenderer = scene.grafRenderer;
				GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
				GrafDevice* grafDevice = grafRenderer->GetGrafDevice();

				grafRenderer->SafeDelete(this->subMeshDescBuffer.release());
				if (!this->subMeshDescArray.empty())
				{
					grafSystem->CreateBuffer(this->subMeshDescBuffer);
					GrafBuffer::InitParams bufferParams = {};
					bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferDst) | ur_uint(GrafBufferUsageFlag::RayTracing);
					bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
					bufferParams.BufferDesc.SizeInBytes = this->subMeshDescArray.size() * sizeof(SubMeshDesc);
					this->subMeshDescBuffer->Initialize(grafDevice, bufferParams);

					grafRenderer->Upload((ur_byte*)this->subMeshDescArray.data(), this->subMeshDescBuffer.get(), bufferParams.BufferDesc.SizeInBytes);
				}

				grafRenderer->SafeDelete(this->materialDescBuffer.release());
				if (!this->materialDescArray.empty())
				{
					grafSystem->CreateBuffer(this->materialDescBuffer);
					GrafBuffer::InitParams bufferParams = {};
					bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferDst) | ur_uint(GrafBufferUsageFlag::RayTracing);
					bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
					bufferParams.BufferDesc.SizeInBytes = this->materialDescArray.size() * sizeof(MeshMaterialDesc);
					this->materialDescBuffer->Initialize(grafDevice, bufferParams);

					grafRenderer->Upload((ur_byte*)this->materialDescArray.data(), this->materialDescBuffer.get(), bufferParams.BufferDesc.SizeInBytes);
				}
			}

			inline GrafBuffer* GetSubMeshDescBuffer() const { return this->subMeshDescBuffer.get(); }
			inline GrafBuffer* GetMaterialDescBuffer() const { return this->materialDescBuffer.get(); }
			inline std::vector<GrafImage*>& GetImage2DArray() { return this->image2DArray; }
			inline std::vector<GrafBuffer*>& GetBufferArray() { return this->bufferArray; }

		private:

			DemoScene& scene;
			std::vector<SubMeshDesc> subMeshDescArray;
			std::unique_ptr<GrafBuffer> subMeshDescBuffer;
			std::vector<MeshMaterialDesc> materialDescArray;
			std::unique_ptr<GrafBuffer> materialDescBuffer;
			std::vector<GrafImage*> image2DArray;
			std::vector<GrafBuffer*> bufferArray;
			ur_uint32 defaultImageWhiteIdx;
			ur_uint32 defaultImageBlackIdx;
			ur_uint32 defaultImageNormalIdx;
		};

		GrafRenderer* grafRenderer;
		std::unique_ptr<GrafImage> defaultImageWhite;
		std::unique_ptr<GrafImage> defaultImageBlack;
		std::unique_ptr<GrafImage> defaultImageNormal;
		std::unique_ptr<GrafImage> blueNoiseImage;
		std::unique_ptr<GPUResourceRegistry> gpuResourceRegistry;
		std::vector<std::unique_ptr<Mesh>> meshes;
		std::unique_ptr<GrafBuffer> instanceBuffer;
		std::unique_ptr<GrafDescriptorTableLayout> rasterDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> rasterDescTable;
		std::unique_ptr<GrafPipeline> rasterPipelineState;
		std::unique_ptr<GrafImage> skyImage;
		std::unique_ptr<GrafDescriptorTableLayout> skyPrecomputeDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> skyPrecomputeDescTable;
		std::unique_ptr<GrafComputePipeline> skyPrecomputePipelineState;
		std::unique_ptr<GrafDescriptorTableLayout> lightingDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> lightingDescTable;
		std::unique_ptr<GrafComputePipeline> lightingPipelineState;
		std::unique_ptr<GrafAccelerationStructure> accelerationStructureTL;
		std::unique_ptr<GrafDescriptorTableLayout> raytraceDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> raytraceDescTable;
		std::unique_ptr<GrafRayTracingPipeline> raytracePipelineState;
		std::unique_ptr<GrafBuffer> raytraceShaderHandlesBuffer;
		GrafStridedBufferRegionDesc rayGenShaderTable;
		GrafStridedBufferRegionDesc rayMissShaderTable;
		GrafStridedBufferRegionDesc rayHitShaderTable;
		std::unique_ptr<GrafDescriptorTableLayout> shadowMipsDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> shadowMipsDescTable;
		std::unique_ptr<GrafComputePipeline> shadowMipsPipelineState;
		std::unique_ptr<GrafDescriptorTableLayout> blurDescTableLayout;
		std::vector<std::unique_ptr<GrafManagedDescriptorTable>> blurDescTables;
		std::unique_ptr<GrafComputePipeline> blurPipelineState;
		std::unique_ptr<GrafDescriptorTableLayout> accumulationDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> accumulationDescTable;
		std::unique_ptr<GrafComputePipeline> accumulationPipelineState;
		std::unique_ptr<GrafShaderLib> shaderLib;
		std::unique_ptr<GrafShaderLib> shaderLibRT;
		std::unique_ptr<GrafShader> shaderVertex;
		std::unique_ptr<GrafShader> shaderPixel;
		std::unique_ptr<GrafSampler> samplerBilinear;
		std::unique_ptr<GrafSampler> samplerBilinearWrap;
		std::unique_ptr<GrafSampler> samplerTrilinear;
		std::unique_ptr<GrafSampler> samplerTrilinearWrap;
		SceneConstants sceneConstants;
		Allocation sceneCBCrntFrameAlloc;
		ur_uint blurDescTableIdx;
		ur_float4 debugVec0;
		ur_float4 debugVec1;
		ur_float4 debugVec2;
		ur_float4 debugVec3;

		std::vector<Instance> sampleInstances;
		ur_uint sampleInstanceCount;
		ur_float sampleInstanceScatterRadius;
		ur_bool animationEnabled;
		ur_float animationCycleTime;
		ur_float animationElapsedTime;

		DemoScene(Realm& realm) : RealmEntity(realm), grafRenderer(ur_null)
		{
			this->sceneConstants = {};
			this->sceneConstants.FrameNumber = 0;
			this->sceneConstants.DirectLightFactor = 1.0f;
			this->sceneConstants.IndirectLightFactor = 1.0f;
			this->blurDescTableIdx = 0;
			this->debugVec0 = ur_float4(0.0f, 0.5f, 0.01f, 2.0f);
			this->debugVec1 = ur_float4(0.01f, 0.0f, 0.1f, 16.0f);
			this->debugVec2 = ur_float4(0.1f, 0.02f, 1.0f, 1.0f);
			this->debugVec3 = ur_float4(0.0f, 0.0f, 0.0f, 0.0f);
			
			// default material override
			this->sceneConstants.OverrideMaterial = false;
			this->sceneConstants.Material.BaseColor = { 0.5f, 0.5f, 0.5f };
			this->sceneConstants.Material.Roughness = 0.5f;
			this->sceneConstants.Material.Reflectance = 0.04f;

			this->sampleInstanceScatterRadius = 40.0f;
			this->sampleInstanceCount = 0;
			#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE)
			this->sampleInstanceCount = 16;
			#elif (SCENE_TYPE_SPONZA == SCENE_TYPE)
			this->sampleInstanceCount = 1;
			#elif (SCENE_TYPE_SANMIGUEL == SCENE_TYPE)
			this->sampleInstanceCount = 1;
			#elif (SCENE_TYPE_RUNGHOLT == SCENE_TYPE)
			this->sampleInstanceCount = 1;
			#elif (SCENE_TYPE_FOREST == SCENE_TYPE)
			this->sampleInstanceCount = 320;
			this->sampleInstanceScatterRadius = 100.0f;
			#endif
			this->animationEnabled = false;
			this->animationCycleTime = 30.0f;
			this->animationElapsedTime = 0.0f;
		}
		~DemoScene()
		{
		}

		void Initialize(GrafRenderPass* rasterRenderPass)
		{
			this->grafRenderer = this->GetRealm().GetComponent<GrafRenderer>();
			if (ur_null == grafRenderer)
				return;
		
			GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = grafRenderer->GetGrafDevice();
			const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());

			// instance buffer

			static const ur_size InstanceCountMax = 1024 * 4;

			GrafBuffer::InitParams instanceBufferParams = {};
			instanceBufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferDst) | ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
			instanceBufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
			instanceBufferParams.BufferDesc.SizeInBytes = InstanceCountMax * sizeof(Instance);

			grafSystem->CreateBuffer(this->instanceBuffer);
			this->instanceBuffer->Initialize(grafDevice, instanceBufferParams);

			// load shaders

			enum ShaderLibId
			{
				ShaderLibId_ComputeSky,
				ShaderLibId_ComputeLighting,
				ShaderLibId_ComputeShadowMips,
				ShaderLibId_BlurLightingResult,
				ShaderLibId_AccumulateLightingResult,
			};
			GrafShaderLib::EntryPoint ShaderLibEntries[] = {
				{ "ComputeSky", GrafShaderType::Compute },
				{ "ComputeLighting", GrafShaderType::Compute },
				{ "ComputeShadowMips", GrafShaderType::Compute },
				{ "BlurLightingResult", GrafShaderType::Compute },
				{ "AccumulateLightingResult", GrafShaderType::Compute },
			};
			enum RTShaderLibId
			{
				RTShaderLibId_RayGenMain,
				RTShaderLibId_MissDirect,
				RTShaderLibId_MissIndirect,
				RTShaderLibId_ClosestHitDirect,
				RTShaderLibId_ClosestHitIndirect,
				RTShaderLibId_AnyHitDirect,
				RTShaderLibId_AnyHitIndirect
			};
			GrafShaderLib::EntryPoint RTShaderLibEntries[] = {
				{ "RayGenMain", GrafShaderType::RayGen },
				{ "MissDirect", GrafShaderType::Miss },
				{ "MissIndirect", GrafShaderType::Miss },
				{ "ClosestHitDirect", GrafShaderType::ClosestHit },
				{ "ClosestHitIndirect", GrafShaderType::ClosestHit },
				{ "AnyHitDirect", GrafShaderType::AnyHit },
				{ "AnyHitIndirect", GrafShaderType::AnyHit },
			};
			GrafUtils::CreateShaderLibFromFile(*grafDevice, "HybridRendering_cs_lib", ShaderLibEntries, ur_array_size(ShaderLibEntries), this->shaderLib);
			GrafUtils::CreateShaderLibFromFile(*grafDevice, "HybridRendering_rt_lib", RTShaderLibEntries, ur_array_size(RTShaderLibEntries), this->shaderLibRT);
			GrafUtils::CreateShaderFromFile(*grafDevice, "HybridRendering_vs", GrafShaderType::Vertex, this->shaderVertex);
			GrafUtils::CreateShaderFromFile(*grafDevice, "HybridRendering_ps", GrafShaderType::Pixel, this->shaderPixel);

			// samplers

			GrafSamplerDesc samplerBilinearDesc = {
				GrafFilterType::Linear, GrafFilterType::Linear, GrafFilterType::Nearest,
				GrafAddressMode::Clamp, GrafAddressMode::Clamp, GrafAddressMode::Clamp,
				false, 1.0f, 0.0f, 0.0f, 128.0f
			};
			grafSystem->CreateSampler(this->samplerBilinear);
			this->samplerBilinear->Initialize(grafDevice, { samplerBilinearDesc });

			GrafSamplerDesc samplerBilinearWrapDesc = {
				GrafFilterType::Linear, GrafFilterType::Linear, GrafFilterType::Nearest,
				GrafAddressMode::Wrap, GrafAddressMode::Wrap, GrafAddressMode::Wrap,
				false, 1.0f, 0.0f, 0.0f, 128.0f
			};
			grafSystem->CreateSampler(this->samplerBilinearWrap);
			this->samplerBilinearWrap->Initialize(grafDevice, { samplerBilinearWrapDesc });

			GrafSamplerDesc samplerTrilinearDesc = {
				GrafFilterType::Linear, GrafFilterType::Linear, GrafFilterType::Linear,
				GrafAddressMode::Clamp, GrafAddressMode::Clamp, GrafAddressMode::Clamp,
				false, 1.0f, 0.0f, 0.0f, 128.0f
			};
			grafSystem->CreateSampler(this->samplerTrilinear);
			this->samplerTrilinear->Initialize(grafDevice, { samplerTrilinearDesc });

			GrafSamplerDesc samplerTrilinearWrapDesc = {
				GrafFilterType::Linear, GrafFilterType::Linear, GrafFilterType::Linear,
				GrafAddressMode::Wrap, GrafAddressMode::Wrap, GrafAddressMode::Wrap,
				false, 1.0f, 0.0f, 0.0f, 128.0f
			};
			grafSystem->CreateSampler(this->samplerTrilinearWrap);
			this->samplerTrilinearWrap->Initialize(grafDevice, { samplerTrilinearWrapDesc });

			// rasterization descriptor table

			GrafDescriptorRangeDesc rasterDescTableLayoutRanges[] = {
				g_SceneCBDescriptor,
				g_SubMeshCBDescriptor,
				g_InstanceBufferDescriptor,
				g_MeshDescBufferDescriptor,
				g_MaterialDescBufferDescriptor,
				g_SamplerTrilinearWrapDescriptor,
				g_ColorTextureDescriptor,
				g_NormalTextureDescriptor,
				g_MaskTextureDescriptor,
			};
			GrafDescriptorTableLayoutDesc rasterDescTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Vertex) |
				ur_uint(GrafShaderStageFlag::Pixel),
				rasterDescTableLayoutRanges, ur_array_size(rasterDescTableLayoutRanges)
			};
			grafSystem->CreateDescriptorTableLayout(this->rasterDescTableLayout);
			this->rasterDescTableLayout->Initialize(grafDevice, { rasterDescTableLayoutDesc });
			this->rasterDescTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
			this->rasterDescTable->Initialize({ this->rasterDescTableLayout.get() });

			// rasterization pipeline configuration
			
			grafSystem->CreatePipeline(this->rasterPipelineState);
			{
				GrafShader* shaderStages[] = {
					this->shaderVertex.get(),
					this->shaderPixel.get(),
				};
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					this->rasterDescTableLayout.get(),
				};
				GrafVertexElementDesc vertexElements[] = {
					{ GrafFormat::R32G32B32_SFLOAT, 0, "POSITION" },
					{ GrafFormat::R32G32B32_SFLOAT, 12, "NORMAL" },
					{ GrafFormat::R32G32B32_SFLOAT, 24, "TANGENT" },
					{ GrafFormat::R32G32_SFLOAT, 36, "TEXCOORD" }
				};
				GrafVertexInputDesc vertexInputs[] = { {
					GrafVertexInputType::PerVertex, 0, sizeof(Mesh::Vertex),
					vertexElements, ur_array_size(vertexElements)
				} };
				GrafColorBlendOpDesc colorTargetBlendOpDesc[RenderTargetColorImageCount];
				for (ur_uint imageId = 0; imageId < RenderTargetColorImageCount; ++imageId)
				{
					colorTargetBlendOpDesc[imageId] = GrafColorBlendOpDesc::Default;
				}
				GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
				pipelineParams.RenderPass = rasterRenderPass;
				pipelineParams.ShaderStages = shaderStages;
				pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				pipelineParams.DescriptorTableLayouts = descriptorLayouts;
				pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				pipelineParams.VertexInputDesc = vertexInputs;
				pipelineParams.VertexInputCount = ur_array_size(vertexInputs);
				pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleList;
				pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
				pipelineParams.CullMode = GrafCullMode::None;
				pipelineParams.DepthTestEnable = true;
				pipelineParams.DepthWriteEnable = true;
				pipelineParams.DepthCompareOp = GrafCompareOp::LessOrEqual;
				pipelineParams.ColorBlendOpDesc = colorTargetBlendOpDesc;
				pipelineParams.ColorBlendOpDescCount = ur_array_size(colorTargetBlendOpDesc);
				this->rasterPipelineState->Initialize(grafDevice, pipelineParams);
			}

			// sky precompute descriptor table

			GrafDescriptorRangeDesc skyPrecomputeDescTableLayoutRanges[] = {
				g_SceneCBDescriptor,
				g_PrecomputedSkyTargetDescriptor,
			};
			GrafDescriptorTableLayoutDesc skyPrecomputeDescTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute),
				skyPrecomputeDescTableLayoutRanges, ur_array_size(skyPrecomputeDescTableLayoutRanges)
			};
			grafSystem->CreateDescriptorTableLayout(this->skyPrecomputeDescTableLayout);
			this->skyPrecomputeDescTableLayout->Initialize(grafDevice, { skyPrecomputeDescTableLayoutDesc });
			this->skyPrecomputeDescTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
			this->skyPrecomputeDescTable->Initialize({ this->skyPrecomputeDescTableLayout.get() });

			// sky precompute pipeline configuration

			grafSystem->CreateComputePipeline(this->skyPrecomputePipelineState);
			{
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					this->skyPrecomputeDescTableLayout.get()
				};
				GrafComputePipeline::InitParams computePipelineParams = GrafComputePipeline::InitParams::Default;
				computePipelineParams.ShaderStage = this->shaderLib->GetShader(ShaderLibId_ComputeSky);
				computePipelineParams.DescriptorTableLayouts = descriptorLayouts;
				computePipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				this->skyPrecomputePipelineState->Initialize(grafDevice, computePipelineParams);
			}

			// lighting descriptor table

			GrafDescriptorRangeDesc lightingDescTableLayoutRanges[] = {
				g_SceneCBDescriptor,
				g_SamplerBilinearWrapDescriptor,
				g_SamplerTrilinearDescriptor,
				g_BlueNoiseImageDescriptor,
				g_GeometryDepthDescriptor,
				g_GeometryImage0Descriptor,
				g_GeometryImage1Descriptor,
				g_GeometryImage2Descriptor,
				g_ShadowResultDescriptor,
				g_TracingInfoDescriptor,
				g_PrecomputedSkyDescriptor,
				g_IndirectLightDescriptor,
				g_LightingTargetDescriptor,
			};
			GrafDescriptorTableLayoutDesc lightingDescTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute),
				lightingDescTableLayoutRanges, ur_array_size(lightingDescTableLayoutRanges)
			};
			grafSystem->CreateDescriptorTableLayout(this->lightingDescTableLayout);
			this->lightingDescTableLayout->Initialize(grafDevice, { lightingDescTableLayoutDesc });
			this->lightingDescTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
			this->lightingDescTable->Initialize({ this->lightingDescTableLayout.get() });

			// lighting compute pipeline configuration

			grafSystem->CreateComputePipeline(this->lightingPipelineState);
			{
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					this->lightingDescTableLayout.get()
				};
				GrafComputePipeline::InitParams computePipelineParams = GrafComputePipeline::InitParams::Default;
				computePipelineParams.ShaderStage = this->shaderLib->GetShader(ShaderLibId_ComputeLighting);
				computePipelineParams.DescriptorTableLayouts = descriptorLayouts;
				computePipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				this->lightingPipelineState->Initialize(grafDevice, computePipelineParams);
			}

			if (grafDeviceDesc->RayTracing.RayTraceSupported)
			{
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

				// ray tracing descriptor table

				GrafDescriptorRangeDesc raytraceDescTableLayoutRanges[] = {
					g_SceneCBDescriptor,
					g_SamplerBilinearWrapDescriptor,
					g_BlueNoiseImageDescriptor,
					g_GeometryDepthDescriptor,
					g_GeometryImage0Descriptor,
					g_GeometryImage1Descriptor,
					g_GeometryImage2Descriptor,
					g_DepthHistoryDescriptor,
					g_ShadowHistoryDescriptor,
					g_TracingHistoryDescriptor,
					g_PrecomputedSkyDescriptor,
					g_SceneStructureDescriptor,
					g_InstanceBufferDescriptor,
					g_MeshDescBufferDescriptor,
					g_MaterialDescBufferDescriptor,
					g_Texture2DArrayDescriptor,
					g_BufferArrayDescriptor,
					g_ShadowTargetDescriptor,
					g_TracingInfoTargetDescriptor,
					g_IndirectLightTargetDescriptor,
					// TODO: texture2d and buffer arrays are used in the same descriptor set, which is against the specification;
					// however, it works fine while the bindings are sufficiently spaced from one another (buffer array binding slot > texture array slot + array size)
				};
				GrafDescriptorTableLayoutDesc raytraceDescTableLayoutDesc = {
					ur_uint(GrafShaderStageFlag::AllRayTracing),
					raytraceDescTableLayoutRanges, ur_array_size(raytraceDescTableLayoutRanges)
				};
				grafSystem->CreateDescriptorTableLayout(this->raytraceDescTableLayout);
				this->raytraceDescTableLayout->Initialize(grafDevice, { raytraceDescTableLayoutDesc });
				this->raytraceDescTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
				this->raytraceDescTable->Initialize({ this->raytraceDescTableLayout.get() });

				// ray tracing pipeline

				GrafShader* shaderStages[] = {
					this->shaderLibRT->GetShader(RTShaderLibId_RayGenMain),
					this->shaderLibRT->GetShader(RTShaderLibId_MissDirect),
					this->shaderLibRT->GetShader(RTShaderLibId_MissIndirect),
					this->shaderLibRT->GetShader(RTShaderLibId_ClosestHitDirect),
					this->shaderLibRT->GetShader(RTShaderLibId_ClosestHitIndirect),
					this->shaderLibRT->GetShader(RTShaderLibId_AnyHitDirect),
					this->shaderLibRT->GetShader(RTShaderLibId_AnyHitIndirect),
				};
				GrafRayTracingShaderGroupDesc shaderGroups[5];
				shaderGroups[0] = GrafRayTracingShaderGroupDesc::Default;
				shaderGroups[0].Type = GrafRayTracingShaderGroupType::General;
				shaderGroups[0].GeneralShaderIdx = RTShaderLibId_RayGenMain;
				shaderGroups[1] = GrafRayTracingShaderGroupDesc::Default;
				shaderGroups[1].Type = GrafRayTracingShaderGroupType::General;
				shaderGroups[1].GeneralShaderIdx = RTShaderLibId_MissDirect;
				shaderGroups[2] = GrafRayTracingShaderGroupDesc::Default;
				shaderGroups[2].Type = GrafRayTracingShaderGroupType::General;
				shaderGroups[2].GeneralShaderIdx = RTShaderLibId_MissIndirect;
				shaderGroups[3] = GrafRayTracingShaderGroupDesc::Default;
				shaderGroups[3].Type = GrafRayTracingShaderGroupType::TrianglesHit;
				shaderGroups[3].ClosestHitShaderIdx = RTShaderLibId_ClosestHitDirect;
				shaderGroups[3].AnyHitShaderIdx = RTShaderLibId_AnyHitDirect;
				shaderGroups[4] = GrafRayTracingShaderGroupDesc::Default;
				shaderGroups[4].Type = GrafRayTracingShaderGroupType::TrianglesHit;
				shaderGroups[4].ClosestHitShaderIdx = RTShaderLibId_ClosestHitIndirect;
				shaderGroups[4].AnyHitShaderIdx = RTShaderLibId_AnyHitIndirect;

				GrafDescriptorTableLayout* descriptorLayouts[] = {
					this->raytraceDescTableLayout.get()
				};
				GrafRayTracingPipeline::InitParams raytracePipelineParams = GrafRayTracingPipeline::InitParams::Default;
				raytracePipelineParams.ShaderLib = this->shaderLibRT.get();
				raytracePipelineParams.ShaderStages = shaderStages;
				raytracePipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				raytracePipelineParams.ShaderGroups = shaderGroups;
				raytracePipelineParams.ShaderGroupCount = ur_array_size(shaderGroups);
				raytracePipelineParams.DescriptorTableLayouts = descriptorLayouts;
				raytracePipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				raytracePipelineParams.MaxRecursionDepth = 8;
				grafSystem->CreateRayTracingPipeline(this->raytracePipelineState);
				this->raytracePipelineState->Initialize(grafDevice, raytracePipelineParams);

				// ray tracing shader table

				grafSystem->CreateBuffer(this->raytraceShaderHandlesBuffer);

				ur_size sgHandleSize = grafDeviceDesc->RayTracing.ShaderGroupHandleSize;
				ur_size sgTableAlign = grafDeviceDesc->RayTracing.ShaderGroupBaseAlignment;
				ur_size sgTableOfs = 0;
				this->rayGenShaderTable		= { this->raytraceShaderHandlesBuffer.get(), sgTableOfs, 1 * sgHandleSize, sgHandleSize };
				sgTableOfs = ur_align(sgTableOfs + this->rayGenShaderTable.Size, sgTableAlign);
				this->rayMissShaderTable	= { this->raytraceShaderHandlesBuffer.get(), sgTableOfs, 2 * sgHandleSize, sgHandleSize };
				sgTableOfs = ur_align(sgTableOfs + this->rayMissShaderTable.Size, sgTableAlign);
				this->rayHitShaderTable		= { this->raytraceShaderHandlesBuffer.get(), sgTableOfs, 2 * sgHandleSize, sgHandleSize };
				sgTableOfs = ur_align(sgTableOfs + this->rayHitShaderTable.Size, sgTableAlign);
				ur_size shaderBufferSize = sgTableOfs;

				GrafBuffer::InitParams shaderBufferParams = {};
				shaderBufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::RayTracing) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress) | ur_uint(GrafBufferUsageFlag::TransferSrc);
				shaderBufferParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
				shaderBufferParams.BufferDesc.SizeInBytes = shaderBufferSize;
				
				this->raytraceShaderHandlesBuffer->Initialize(grafDevice, shaderBufferParams);
				this->raytraceShaderHandlesBuffer->Write([this, raytracePipelineParams, shaderBufferParams](ur_byte* mappedDataPtr) -> Result
				{
					GrafRayTracingPipeline* rayTracingPipeline = this->raytracePipelineState.get();
					rayTracingPipeline->GetShaderGroupHandles(0, 1, shaderBufferParams.BufferDesc.SizeInBytes, mappedDataPtr + this->rayGenShaderTable.Offset);
					rayTracingPipeline->GetShaderGroupHandles(1, 2, shaderBufferParams.BufferDesc.SizeInBytes, mappedDataPtr + this->rayMissShaderTable.Offset);
					rayTracingPipeline->GetShaderGroupHandles(3, 2, shaderBufferParams.BufferDesc.SizeInBytes, mappedDataPtr + this->rayHitShaderTable.Offset);
					return Result(Success);
				});

				// ray tracing result mips generation pipeline

				GrafDescriptorRangeDesc shadowMipsDescTableLayoutRanges[] = {
					g_SceneCBDescriptor,
					g_ShadowResultDescriptor,
					g_ShadowMip1Descriptor,
					g_ShadowMip2Descriptor,
					g_ShadowMip3Descriptor,
					g_ShadowMip4Descriptor,
				};
				GrafDescriptorTableLayoutDesc shadowMipsDescTableLayoutDesc = {
					ur_uint(GrafShaderStageFlag::Compute),
					shadowMipsDescTableLayoutRanges, ur_array_size(shadowMipsDescTableLayoutRanges)
				};
				grafSystem->CreateDescriptorTableLayout(this->shadowMipsDescTableLayout);
				this->shadowMipsDescTableLayout->Initialize(grafDevice, { shadowMipsDescTableLayoutDesc });
				this->shadowMipsDescTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
				this->shadowMipsDescTable->Initialize({ this->shadowMipsDescTableLayout.get() });

				grafSystem->CreateComputePipeline(this->shadowMipsPipelineState);
				{
					GrafDescriptorTableLayout* descriptorLayouts[] = {
						this->shadowMipsDescTableLayout.get()
					};
					GrafComputePipeline::InitParams computePipelineParams = GrafComputePipeline::InitParams::Default;
					computePipelineParams.ShaderStage = this->shaderLib->GetShader(ShaderLibId_ComputeShadowMips);
					computePipelineParams.DescriptorTableLayouts = descriptorLayouts;
					computePipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
					this->shadowMipsPipelineState->Initialize(grafDevice, computePipelineParams);
				}

				// ray tracing result blur filter

				GrafDescriptorRangeDesc blurDescTableLayoutRanges[] = {
					g_SceneCBDescriptor,
					g_BlurPassCBDescriptor,
					g_GeometryDepthDescriptor,
					g_GeometryImage0Descriptor,
					g_GeometryImage1Descriptor,
					g_GeometryImage2Descriptor,
					g_BlurSourceDescriptor,
					g_BlurTargetDescriptor,
				};
				GrafDescriptorTableLayoutDesc blurDescTableLayoutDesc = {
					ur_uint(GrafShaderStageFlag::Compute),
					blurDescTableLayoutRanges, ur_array_size(blurDescTableLayoutRanges)
				};
				grafSystem->CreateDescriptorTableLayout(this->blurDescTableLayout);
				this->blurDescTableLayout->Initialize(grafDevice, { blurDescTableLayoutDesc });
				this->blurDescTables.resize(BlurPassCountPerFrame);
				for (auto& descTable : this->blurDescTables)
				{
					descTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
					descTable->Initialize({ this->blurDescTableLayout.get() });
				}

				grafSystem->CreateComputePipeline(this->blurPipelineState);
				{
					GrafDescriptorTableLayout* descriptorLayouts[] = {
						this->blurDescTableLayout.get()
					};
					GrafComputePipeline::InitParams computePipelineParams = GrafComputePipeline::InitParams::Default;
					computePipelineParams.ShaderStage = this->shaderLib->GetShader(ShaderLibId_BlurLightingResult);
					computePipelineParams.DescriptorTableLayouts = descriptorLayouts;
					computePipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
					this->blurPipelineState->Initialize(grafDevice, computePipelineParams);
				}

				// ray tracing result temporal accumulation filter

				GrafDescriptorRangeDesc accumulationDescTableLayoutRanges[] = {
					g_SceneCBDescriptor,
					g_SamplerPointDescriptor,
					g_SamplerBilinearDescriptor,
					g_SamplerTrilinearDescriptor,
					g_GeometryDepthDescriptor,
					g_GeometryImage0Descriptor,
					g_GeometryImage1Descriptor,
					g_GeometryImage2Descriptor,
					g_DepthHistoryDescriptor,
					g_ShadowHistoryDescriptor,
					g_TracingHistoryDescriptor,
					g_ShadowMipsDescriptor,
					g_IndirectLightHistoryDescriptor,
					g_ShadowTargetDescriptor,
					g_TracingInfoTargetDescriptor,
					g_IndirectLightTargetDescriptor,
				};
				GrafDescriptorTableLayoutDesc accumulationDescTableLayoutDesc = {
					ur_uint(GrafShaderStageFlag::Compute),
					accumulationDescTableLayoutRanges, ur_array_size(accumulationDescTableLayoutRanges)
				};
				grafSystem->CreateDescriptorTableLayout(this->accumulationDescTableLayout);
				this->accumulationDescTableLayout->Initialize(grafDevice, { accumulationDescTableLayoutDesc });
				this->accumulationDescTable.reset(new GrafManagedDescriptorTable(*grafRenderer));
				this->accumulationDescTable->Initialize({ this->accumulationDescTableLayout.get() });

				grafSystem->CreateComputePipeline(this->accumulationPipelineState);
				{
					GrafDescriptorTableLayout* descriptorLayouts[] = {
						this->accumulationDescTableLayout.get()
					};
					GrafComputePipeline::InitParams computePipelineParams = GrafComputePipeline::InitParams::Default;
					computePipelineParams.ShaderStage = this->shaderLib->GetShader(ShaderLibId_AccumulateLightingResult);
					computePipelineParams.DescriptorTableLayouts = descriptorLayouts;
					computePipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
					this->accumulationPipelineState->Initialize(grafDevice, computePipelineParams);
				}
			}

			// default images

			{
				GrafCommandList* cmdList = grafRenderer->GetTransientCommandList();
				cmdList->Begin();

				auto createDefaultImage = [&grafSystem, &grafDevice, &cmdList]
					(std::unique_ptr<GrafImage>& defaultImage, GrafFormat format, GrafClearValue fillColor) -> Result
				{
					GrafImageDesc imageDesc = {
						GrafImageType::Tex2D,
						format,
						ur_uint3(4, 4, 1), 1,
						ur_uint(GrafImageUsageFlag::ShaderRead) | ur_uint(GrafImageUsageFlag::TransferDst) | ur_uint(GrafImageUsageFlag::ColorRenderTarget),
						ur_uint(GrafDeviceMemoryFlag::GpuLocal)
					};
					grafSystem->CreateImage(defaultImage);
					Result res = defaultImage->Initialize(grafDevice, { imageDesc });
					if (Succeeded(res))
					{
						cmdList->ImageMemoryBarrier(defaultImage.get(), GrafImageState::Current, GrafImageState::ColorClear);
						cmdList->ClearColorImage(defaultImage.get(), fillColor);
						cmdList->ImageMemoryBarrier(defaultImage.get(), GrafImageState::Current, GrafImageState::ShaderRead);
					}
					return res;
				};
				createDefaultImage(this->defaultImageWhite, GrafFormat::R8G8B8A8_UNORM, { 1.0f, 1.0f, 1.0f, 1.0f });
				createDefaultImage(this->defaultImageBlack, GrafFormat::R8G8B8A8_UNORM, { 0.0f, 0.0f, 0.0f, 0.0f });
				createDefaultImage(this->defaultImageNormal, GrafFormat::R8G8B8A8_UNORM, { 0.5f, 0.5f, 0.0f, 0.0f });

				// blue noise image
				{
					std::unique_ptr<GrafUtils::ImageData> imageData(new GrafUtils::ImageData());
					Result result = GrafUtils::LoadImageFromFile(*grafDevice, "../Res/BlueNoise64_R8G8_0.dds", *imageData);
					if (Succeeded(result))
					{
						grafSystem->CreateImage(this->blueNoiseImage);
						result = this->blueNoiseImage->Initialize(grafDevice, { imageData->Desc });
						if (Succeeded(result))
						{
							cmdList->ImageMemoryBarrier(this->blueNoiseImage.get(), GrafImageState::Current, GrafImageState::TransferDst);
							cmdList->Copy(imageData->MipBuffers[0].get(), this->blueNoiseImage.get());
							cmdList->ImageMemoryBarrier(this->blueNoiseImage.get(), GrafImageState::Current, GrafImageState::ShaderRead);
							GrafUtils::ImageData* imageDataPtr = imageData.release();
							grafRenderer->AddCommandListCallback(cmdList, { imageDataPtr }, [](GrafCallbackContext& ctx) -> Result
							{
								GrafUtils::ImageData* imageDataPtr = reinterpret_cast<GrafUtils::ImageData*>(ctx.DataPtr);
								delete imageDataPtr;
								return Result(Success);
							});
						}
						else
						{
							this->blueNoiseImage.reset();
						}
					}
				}

				cmdList->End();
				grafDevice->Record(cmdList);
			}

			// gpu resource registry

			this->gpuResourceRegistry.reset(new GPUResourceRegistry(*this));
			this->gpuResourceRegistry->Initialize();

			// load mesh(es)

			GrafUtils::MeshVertexElementFlags meshVertexMask = // must be compatible with Mesh::Vertex
				ur_uint(GrafUtils::MeshVertexElementFlag::Position) |
				ur_uint(GrafUtils::MeshVertexElementFlag::Normal) |
				ur_uint(GrafUtils::MeshVertexElementFlag::Tangent) |
				ur_uint(GrafUtils::MeshVertexElementFlag::TexCoord);
			struct MeshResDesc
			{
				MeshId Id;
				std::string Name;
			};
			MeshResDesc meshResDesc[] = {
				{ MeshId_Plane, "../Res/Models/Plane.obj" },
				{ MeshId_Sphere, "../Res/Models/Sphere.obj" },
				{ MeshId_Wuson, "../Res/Models/Wuson.obj" },
				#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE)
				{ MeshId_MedievalBuilding, "../Res/Models/Medieval_building.obj" },
				#elif (SCENE_TYPE_SPONZA == SCENE_TYPE)
				{ MeshId_Sponza, "../Res/Models/Sponza/sponza.obj" },
				#elif (SCENE_TYPE_SANMIGUEL == SCENE_TYPE)
				{ MeshId_SanMiguel, "../Res/Models/SanMiguel/san-miguel-low-poly.obj" },
				#elif (SCENE_TYPE_RUNGHOLT == SCENE_TYPE)
				{ MeshId_Rungholt, "../Res/Models/Rungholt/rungholt.obj" },
				#elif (SCENE_TYPE_FOREST == SCENE_TYPE)
				{ MeshId_Banana, "../Res/Models/Banana/Banana.obj" },
				{ MeshId_BananaSmall, "../Res/Models/BananaSmall/Banana-small.obj" },
				{ MeshId_House, "../Res/Models/House/House.obj" },
				#endif
			};
			for (ur_size ires = 0; ires < ur_array_size(meshResDesc); ++ires)
			{
				GrafUtils::ModelData modelData;
				if (GrafUtils::LoadModelFromFile(*grafDevice, meshResDesc[ires].Name, modelData, meshVertexMask) == Success && modelData.Meshes.size() > 0)
				{
					const GrafUtils::MeshData& meshData = *modelData.Meshes[0];
					if (meshData.VertexElementFlags & meshVertexMask) // make sure all masked attributes available in the mesh
					{
						std::unique_ptr<Mesh> mesh(new Mesh(*grafSystem));
						mesh->Initialize(this, meshResDesc[ires].Id, meshData);
						this->meshes.emplace_back(std::move(mesh));
					}
				}
			}

			// all meshes & images are expected to be initialized at this point
			// upload registry data to gpu
			this->gpuResourceRegistry->Upload();
		}

		void Update(ur_float elapsedSeconds)
		{
			if (ur_null == this->grafRenderer)
				return;

			GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();
			const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());

			if (this->animationEnabled) this->animationElapsedTime += elapsedSeconds;
			ur_float crntTimeFactor = (this->animationElapsedTime / this->animationCycleTime);
			ur_float modY;
			ur_float crntCycleFactor = std::modf(crntTimeFactor, &modY);
			ur_float animAngle = MathConst<ur_float>::Pi * 2.0f * crntCycleFactor;

			// update local frame counter

			this->sceneConstants.FrameNumber += 1;

			// update instances

			for (auto& mesh : this->meshes)
			{
				for (auto& subNesh : mesh->subMeshes)
				{
					subNesh.instanceCount = 0;
				}
			}
			sampleInstances.clear();
			ur_uint instanceBufferOfs = 0;
			std::vector<ur_float4x4> transforms;

			// plane
			static const float planeScale = 100.0f;
			#if (SCENE_TYPE_SPONZA == SCENE_TYPE)
			static const float planeHeight = -2.0f;
			#elif (SCENE_TYPE_SANMIGUEL == SCENE_TYPE)
			static const float planeHeight = -0.5;
			#elif (SCENE_TYPE_RUNGHOLT == SCENE_TYPE)
			static const float planeHeight = -0.0;
			#else
			static const float planeHeight = 0.0f;
			#endif
			ur_float4x4 planeTransform = {
				planeScale, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, planeHeight,
				0.0f, 0.0f, planeScale, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
			for (auto& subMesh : this->meshes[MeshId_Plane]->subMeshes)
			{
				GrafAccelerationStructureInstance meshInstance;
				memcpy(meshInstance.Transform, &planeTransform, sizeof(ur_float4) * 3);
				meshInstance.Index = ur_uint32(subMesh.GetGpuRegistryIdx());
				meshInstance.Mask = 0xff;
				meshInstance.ShaderTableRecordOffset = 0;
				meshInstance.Flags = (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable));
				meshInstance.AccelerationStructureHandle = ur_uint64(subMesh.GetBLASHandle());
				subMesh.instanceCount = 1;
				subMesh.instanceOfs = instanceBufferOfs;
				instanceBufferOfs += 1;
				sampleInstances.emplace_back(meshInstance);
			}

			// animated spheres
			static const ur_uint sphereInstanceCount = 16;
			transforms.clear();
			transforms.resize(sphereInstanceCount);
			ur_float radius = 7.0f;
			ur_float height = 2.0f;
			for (ur_size i = 0; i < sphereInstanceCount; ++i)
			{
				ur_float4x4& transform = transforms[i];
				transform = {
					1.0f, 0.0f, 0.0f, radius* cosf(ur_float(i) / sphereInstanceCount * MathConst<ur_float>::Pi * 2.0f + animAngle),
					0.0f, 1.0f, 0.0f, height + cosf(ur_float(i) / sphereInstanceCount * MathConst<ur_float>::Pi * 6.0f + animAngle),
					0.0f, 0.0f, 1.0f, radius* sinf(ur_float(i) / sphereInstanceCount * MathConst<ur_float>::Pi * 2.0f + animAngle),
					0.0f, 0.0f, 0.0f, 1.0f
				};
			}
			for (auto& subMesh : this->meshes[MeshId_Sphere]->subMeshes)
			{
				for (ur_size i = 0; i < sphereInstanceCount; ++i)
				{
					GrafAccelerationStructureInstance meshInstance;
					memcpy(meshInstance.Transform, &transforms[i], sizeof(ur_float4) * 3);
					meshInstance.Index = ur_uint32(subMesh.GetGpuRegistryIdx());
					meshInstance.Mask = 0xff;
					meshInstance.ShaderTableRecordOffset = 0;
					meshInstance.Flags = (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable));
					meshInstance.AccelerationStructureHandle = ur_uint64(subMesh.GetBLASHandle());
					sampleInstances.emplace_back(meshInstance);
				}
				subMesh.instanceCount = sphereInstanceCount;
				subMesh.instanceOfs = instanceBufferOfs;
				instanceBufferOfs += sphereInstanceCount;
			}

			// static randomly scarttered meshes
			auto ScatterMeshInstances = [this, &instanceBufferOfs, &transforms](Mesh* mesh, ur_uint instanceCount, ur_float radius, ur_float size, ur_float sizeRndDelta, ur_float posOfs, ur_bool firstInstanceRnd = true) -> void
			{
				if (ur_null == mesh)
					return;

				// generate per instance transformations
				transforms.clear();
				transforms.resize(instanceCount);
				ur_float3 pos(0.0f, posOfs, 0.0f);
				ur_float3 frameI, frameJ, frameK;
				frameJ = ur_float3(0.0f, 1.0, 0.0f);
				for (ur_size i = 0; i < instanceCount; ++i)
				{
					ur_float r = (firstInstanceRnd || i > 0 ? 1.0f : 0.0f);
					ur_float a = ur_float(std::rand()) / RAND_MAX * MathConst<ur_float>::Pi * 2.0f * r;
					ur_float d = radius * sqrtf(ur_float(std::rand()) / RAND_MAX) * r;
					pos.x = cosf(a) * d;
					pos.z = sinf(a) * d;

					a = (ur_float(std::rand()) / RAND_MAX /*+ crntCycleFactor*/) * MathConst<ur_float>::Pi * 2.0f;
					frameI.x = cosf(a);
					frameI.y = 0.0f;
					frameI.z = sinf(a);
					frameK = ur_float3::Cross(frameI, frameJ);

					ur_float scale = size + sizeRndDelta * (2.0f * ur_float(std::rand()) / RAND_MAX - 1.0f) * r;
					ur_float4x4& transform = transforms[i];
					transform = {
						frameI.x * scale, frameJ.x * scale, frameK.x * scale, pos.x,
						frameI.y * scale, frameJ.y * scale, frameK.y * scale, pos.y,
						frameI.z * scale, frameJ.z * scale, frameK.z * scale, pos.z,
						0.0f, 0.0f, 0.0f, 1.0f
					};
				}
				// fill sub meshes (share transformations but have different Index)
				for (auto& subMesh : mesh->subMeshes)
				{
					for (ur_size i = 0; i < instanceCount; ++i)
					{
						GrafAccelerationStructureInstance meshInstance;
						memcpy(meshInstance.Transform, &transforms[i], sizeof(ur_float4) * 3);
						meshInstance.Index = ur_uint32(subMesh.GetGpuRegistryIdx());
						meshInstance.Mask = 0xff;
						meshInstance.ShaderTableRecordOffset = 0;
						meshInstance.Flags = (ur_uint(GrafAccelerationStructureInstanceFlag::ForceOpaque) | ur_uint(GrafAccelerationStructureInstanceFlag::TriangleFacingCullDisable));
						meshInstance.AccelerationStructureHandle = ur_uint64(subMesh.GetBLASHandle());
						this->sampleInstances.emplace_back(meshInstance);
					}
					subMesh.instanceCount = instanceCount;
					subMesh.instanceOfs = instanceBufferOfs;
					instanceBufferOfs += instanceCount;
				}
			};
			std::srand(58911192);
			#if (SCENE_TYPE_MEDIEVAL_BUILDINGS == SCENE_TYPE)
			ScatterMeshInstances(this->meshes[MeshId_MedievalBuilding].get(), this->sampleInstanceCount, this->sampleInstanceScatterRadius, 2.0f, 0.0f, 0.0f, false);
			#elif (SCENE_TYPE_SPONZA == SCENE_TYPE)
			ScatterMeshInstances(this->meshes[MeshId_Sponza].get(), this->sampleInstanceCount, this->sampleInstanceScatterRadius, 0.02f, 0.0f, 0.0f, false);
			#elif (SCENE_TYPE_SANMIGUEL == SCENE_TYPE)
			ScatterMeshInstances(this->meshes[MeshId_SanMiguel].get(), this->sampleInstanceCount, this->sampleInstanceScatterRadius, 1.0f, 0.0f, 0.0f, false);
			#elif (SCENE_TYPE_RUNGHOLT == SCENE_TYPE)
			ScatterMeshInstances(this->meshes[MeshId_Rungholt].get(), this->sampleInstanceCount, this->sampleInstanceScatterRadius, 1.0f, 0.0f, 0.0f, false);
			#elif (SCENE_TYPE_FOREST == SCENE_TYPE)
			ScatterMeshInstances(this->meshes[MeshId_Banana].get(), this->sampleInstanceCount, this->sampleInstanceScatterRadius, 0.035f, 0.01f, 0.0f, true);
			ScatterMeshInstances(this->meshes[MeshId_BananaSmall].get(), this->sampleInstanceCount * 2, this->sampleInstanceScatterRadius, 0.02f, 0.005f, 0.0f, true);
			ScatterMeshInstances(this->meshes[MeshId_House].get(), this->sampleInstanceCount / 8, this->sampleInstanceScatterRadius, 0.01f, 0.0f, 0.0f, false);
			#endif

			// upload instances

			ur_size updateSize = sampleInstances.size() * sizeof(Instance);
			if (updateSize > this->instanceBuffer->GetDesc().SizeInBytes)
				return; // too many instances

			GrafCommandList* grafCmdList = grafRenderer->GetTransientCommandList();
			grafCmdList->Begin();

			Allocation uploadAllocation = grafRenderer->GetDynamicUploadBufferAllocation(updateSize);
			GrafBuffer* uploadBuffer = grafRenderer->GetDynamicUploadBuffer();
			uploadBuffer->Write((const ur_byte*)sampleInstances.data(), uploadAllocation.Size, 0, uploadAllocation.Offset);
			grafCmdList->BufferMemoryBarrier(this->instanceBuffer.get(), GrafBufferState::Current, GrafBufferState::TransferDst);
			grafCmdList->Copy(uploadBuffer, this->instanceBuffer.get(), updateSize, uploadAllocation.Offset, 0);

			if (grafDeviceDesc->RayTracing.RayTraceSupported)
			{
				// build TLAS on device

				GrafAccelerationStructureInstancesData asInstanceData = {};
				asInstanceData.IsPointersArray = false;
				asInstanceData.DeviceAddress = this->instanceBuffer->GetDeviceAddress();

				GrafAccelerationStructureGeometryData sampleGeometryDataTL = {};
				sampleGeometryDataTL.GeometryType = GrafAccelerationStructureGeometryType::Instances;
				sampleGeometryDataTL.GeometryFlags = ur_uint(GrafAccelerationStructureGeometryFlag::Opaque);
				sampleGeometryDataTL.InstancesData = &asInstanceData;
				sampleGeometryDataTL.PrimitiveCount = ur_uint32(sampleInstances.size());

				grafCmdList->BufferMemoryBarrier(this->instanceBuffer.get(), GrafBufferState::Current, GrafBufferState::RayTracingRead);
				grafCmdList->BuildAccelerationStructure(this->accelerationStructureTL.get(), &sampleGeometryDataTL, 1);
				grafCmdList->BufferMemoryBarrier(this->instanceBuffer.get(), GrafBufferState::Current, GrafBufferState::ShaderRead);
			}

			grafCmdList->End();
			grafDevice->Record(grafCmdList);
		}

		void Render(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, LightingBufferSet* lightingBufferSet,
			Camera& camera, const LightingDesc& lightingDesc, const AtmosphereDesc& atmosphereDesc)
		{
			// update & upload frame constants

			const ur_uint3& targetSize = renderTargetSet->renderTarget->GetImage(0)->GetDesc().Size;
			const ur_uint3& lightBufferSize = lightingBufferSet->images[0]->GetDesc().Size;
			sceneConstants.ViewProjPrev = (renderTargetSet->resetHistory ? camera.GetViewProj() : sceneConstants.ViewProj);
			sceneConstants.ViewProjInvPrev = (renderTargetSet->resetHistory ? camera.GetViewProjInv() : sceneConstants.ViewProjInv);
			sceneConstants.ProjPrev = (renderTargetSet->resetHistory ? camera.GetProj() : sceneConstants.Proj);
			sceneConstants.ViewPrev = (renderTargetSet->resetHistory ? camera.GetView() : sceneConstants.View);
			sceneConstants.ViewProj = camera.GetViewProj();
			sceneConstants.ViewProjInv = camera.GetViewProjInv();
			sceneConstants.Proj = camera.GetProj();
			sceneConstants.View = camera.GetView();
			sceneConstants.CameraPos = camera.GetPosition();
			sceneConstants.CameraDir = camera.GetDirection();
			sceneConstants.TargetSize.x = (ur_float)targetSize.x;
			sceneConstants.TargetSize.y = (ur_float)targetSize.y;
			sceneConstants.TargetSize.z = 1.0f / sceneConstants.TargetSize.x;
			sceneConstants.TargetSize.w = 1.0f / sceneConstants.TargetSize.y;
			sceneConstants.LightBufferSize.x = (ur_float)lightBufferSize.x;
			sceneConstants.LightBufferSize.y = (ur_float)lightBufferSize.y;
			sceneConstants.LightBufferSize.z = 1.0f / sceneConstants.LightBufferSize.x;
			sceneConstants.LightBufferSize.w = 1.0f / sceneConstants.LightBufferSize.y;
			sceneConstants.PrecomputedSkySize.x = (ur_float)g_Settings.SkyImageSize.x;
			sceneConstants.PrecomputedSkySize.y = (ur_float)g_Settings.SkyImageSize.y;
			sceneConstants.PrecomputedSkySize.z = 1.0f / sceneConstants.PrecomputedSkySize.x;
			sceneConstants.PrecomputedSkySize.w = 1.0f / sceneConstants.PrecomputedSkySize.y;
			sceneConstants.LightBufferDownscale.x = (ur_float)g_Settings.RayTracing.LightingBufferDownscale;
			sceneConstants.LightBufferDownscale.y = 1.0f / sceneConstants.LightBufferDownscale.x;
			sceneConstants.DebugVec0 = this->debugVec0;
			sceneConstants.DebugVec1 = this->debugVec1;
			sceneConstants.DebugVec2 = this->debugVec2;
			sceneConstants.DebugVec3 = this->debugVec3;
			sceneConstants.Lighting = lightingDesc;
			sceneConstants.Atmosphere = atmosphereDesc;
			sceneConstants.ShadowSamplesPerLight = g_Settings.RayTracing.Shadow.SamplesPerLight;
			sceneConstants.ShadowAccumulationFrames = g_Settings.RayTracing.Shadow.TemporalFilter.AccumulationFrames;
			sceneConstants.ShadowTemporalTolerance = g_Settings.RayTracing.Shadow.TemporalFilter.Tolerance;
			sceneConstants.ShadowTemporalThreshold = g_Settings.RayTracing.Shadow.TemporalFilter.Threshold;
			sceneConstants.IndirectSamplesPerFrame = g_Settings.RayTracing.IndirectLight.SamplesPerFrame;
			sceneConstants.IndirectBouncesCount = g_Settings.RayTracing.IndirectLight.BouncesCount;
			sceneConstants.IndirectAccumulationFrames = g_Settings.RayTracing.IndirectLight.TemporalFilter.AccumulationFrames;
			sceneConstants.IndirectTemporalTolerance = g_Settings.RayTracing.IndirectLight.TemporalFilter.Tolerance;
			sceneConstants.IndirectTemporalThreshold = g_Settings.RayTracing.IndirectLight.TemporalFilter.Threshold;
			sceneConstants.PerFrameJitter = g_Settings.RayTracing.PerFrameJitter;

			GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
			this->sceneCBCrntFrameAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SceneConstants));
			dynamicCB->Write((ur_byte*)&sceneConstants, sizeof(sceneConstants), 0, this->sceneCBCrntFrameAlloc.Offset);

			// rasterize meshes

			grafCmdList->BindPipeline(this->rasterPipelineState.get());
			for (auto& mesh : this->meshes)
			{
				grafCmdList->BindVertexBuffer(mesh->vertexBuffer.get(), 0);
				grafCmdList->BindIndexBuffer(mesh->indexBuffer.get(), (sizeof(Mesh::Index) == 2 ? GrafIndexType::UINT16 : GrafIndexType::UINT32));
				for (auto& subMesh : mesh->subMeshes)
				{
					if (0 == subMesh.instanceCount)
						continue;

					// update & upload sub mesh constants

					SubMeshConstants subMeshConstants;
					subMeshConstants.InstanceOfs = subMesh.instanceOfs;

					Allocation subMeshCBAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SceneConstants));
					dynamicCB->Write((ur_byte*)&subMeshConstants, sizeof(SubMeshConstants), 0, subMeshCBAlloc.Offset);

					// update descriptor table

					GrafImage* colorImage = (subMesh.colorImage.get() ? subMesh.colorImage.get() : this->defaultImageWhite.get());
					GrafImage* normalImage = (subMesh.normalImage.get() ? subMesh.normalImage.get() : this->defaultImageNormal.get());
					GrafImage* maskImage = (subMesh.maskImage.get() ? subMesh.maskImage.get() : this->defaultImageWhite.get());
					GrafDescriptorTable* descriptorTable = subMesh.descriptorTable->GetFrameObject();
					descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, dynamicCB, this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
					descriptorTable->SetConstantBuffer(g_SubMeshCBDescriptor, dynamicCB, subMeshCBAlloc.Offset, subMeshCBAlloc.Size);
					descriptorTable->SetBuffer(g_InstanceBufferDescriptor, this->instanceBuffer.get());
					descriptorTable->SetBuffer(g_MeshDescBufferDescriptor, this->gpuResourceRegistry->GetSubMeshDescBuffer());
					descriptorTable->SetBuffer(g_MaterialDescBufferDescriptor, this->gpuResourceRegistry->GetMaterialDescBuffer());
					descriptorTable->SetSampler(g_SamplerTrilinearWrapDescriptor, this->samplerTrilinearWrap.get());
					descriptorTable->SetImage(g_ColorTextureDescriptor, colorImage);
					descriptorTable->SetImage(g_NormalTextureDescriptor, normalImage);
					descriptorTable->SetImage(g_MaskTextureDescriptor, maskImage);
					
					// draw

					grafCmdList->BindDescriptorTable(descriptorTable, this->rasterPipelineState.get());
					grafCmdList->DrawIndexed(subMesh.primitivesCount * 3, subMesh.instanceCount,
						subMesh.primitivesOffset, 0, 0);
				}
			}
		}

		void UpdatePrecomputedSkyResources(GrafCommandList* grafCmdList)
		{
			GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
			GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();

			// (re)initialize image if required

			if (ur_null == this->skyImage.get() || this->skyImage->GetDesc().Size != g_Settings.SkyImageSize)
			{
				GrafImage* prevImagePtr = this->skyImage.release();
				if (prevImagePtr != ur_null)
				{
					this->grafRenderer->SafeDelete(prevImagePtr, grafCmdList);
				}

				GrafImageDesc imageDesc = {
					GrafImageType::Tex2D,
					GrafFormat::R16G16B16A16_SFLOAT,
					ur_uint3(g_Settings.SkyImageSize.x, g_Settings.SkyImageSize.y, 1), 1,
					ur_uint(GrafImageUsageFlag::ShaderRead) | ur_uint(GrafImageUsageFlag::ShaderReadWrite),
					ur_uint(GrafDeviceMemoryFlag::GpuLocal)
				};
				grafSystem->CreateImage(this->skyImage);
				this->skyImage->Initialize(grafDevice, { imageDesc });
			}
		}

		void PrecomputeSky(GrafCommandList * grafCmdList)
		{
			// update descriptor table
			// common constant buffer is expected to be uploaded during rasterization pass

			GrafDescriptorTable* descriptorTable = this->skyPrecomputeDescTable->GetFrameObject();
			descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
			descriptorTable->SetRWImage(g_PrecomputedSkyTargetDescriptor, this->skyImage.get());

			// compute

			grafCmdList->BindComputePipeline(this->skyPrecomputePipelineState.get());
			grafCmdList->BindComputeDescriptorTable(descriptorTable, this->skyPrecomputePipelineState.get());

			static const ur_uint3 groupSize = { 8, 8, 1 };
			const ur_uint3& targetSize = this->skyImage->GetDesc().Size;
			grafCmdList->Dispatch((targetSize.x - 1) / groupSize.x + 1, (targetSize.y - 1) / groupSize.y + 1, 1);
		}

		void RayTrace(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, LightingBufferSet* lightingBufferSet)
		{
			// update descriptor table
			// common constant buffer is expected to be uploaded during rasterization pass

			GrafDescriptorTable* descriptorTable = this->raytraceDescTable->GetFrameObject();
			descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
			descriptorTable->SetSampler(g_SamplerBilinearWrapDescriptor, this->samplerBilinearWrap.get());
			descriptorTable->SetImage(g_BlueNoiseImageDescriptor, this->blueNoiseImage.get());
			descriptorTable->SetImage(g_GeometryDepthDescriptor, renderTargetSet->images[RenderTargetImageUsage_Depth]);
			descriptorTable->SetImage(g_GeometryImage0Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry0]);
			descriptorTable->SetImage(g_GeometryImage1Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry1]);
			descriptorTable->SetImage(g_GeometryImage2Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry2]);
			descriptorTable->SetImage(g_DepthHistoryDescriptor, renderTargetSet->images[RenderTargetImageUsage_DepthHistory]);
			descriptorTable->SetImage(g_ShadowHistoryDescriptor, lightingBufferSet->images[LightingImageUsage_DirectShadowHistory].get());
			descriptorTable->SetImage(g_TracingHistoryDescriptor, lightingBufferSet->images[LightingImageUsage_TracingInfoHistory].get());
			descriptorTable->SetImage(g_PrecomputedSkyDescriptor, this->skyImage.get());
			descriptorTable->SetAccelerationStructure(g_SceneStructureDescriptor, this->accelerationStructureTL.get());
			descriptorTable->SetBuffer(g_InstanceBufferDescriptor, this->instanceBuffer.get());
			descriptorTable->SetBuffer(g_MeshDescBufferDescriptor, this->gpuResourceRegistry->GetSubMeshDescBuffer());
			descriptorTable->SetBuffer(g_MaterialDescBufferDescriptor, this->gpuResourceRegistry->GetMaterialDescBuffer());
			descriptorTable->SetImageArray(g_Texture2DArrayDescriptor, this->gpuResourceRegistry->GetImage2DArray().data(), std::min((ur_uint32)this->gpuResourceRegistry->GetImage2DArray().size(), g_TextureArraySize));
			descriptorTable->SetBufferArray(g_BufferArrayDescriptor, this->gpuResourceRegistry->GetBufferArray().data(), std::min((ur_uint32)this->gpuResourceRegistry->GetBufferArray().size(), g_BufferArraySize));
			descriptorTable->SetRWImage(g_ShadowTargetDescriptor, lightingBufferSet->images[LightingImageUsage_DirectShadow].get());
			descriptorTable->SetRWImage(g_TracingInfoTargetDescriptor, lightingBufferSet->images[LightingImageUsage_TracingInfo].get());
			descriptorTable->SetRWImage(g_IndirectLightTargetDescriptor, lightingBufferSet->images[LightingImageUsage_IndirectLight].get());

			// trace rays

			grafCmdList->BindRayTracingPipeline(this->raytracePipelineState.get());
			grafCmdList->BindRayTracingDescriptorTable(descriptorTable, this->raytracePipelineState.get());

			const ur_uint3& bufferSize = lightingBufferSet->images[0]->GetDesc().Size;
			grafCmdList->DispatchRays(bufferSize.x, bufferSize.y, 1, &this->rayGenShaderTable, &this->rayMissShaderTable, &this->rayHitShaderTable, ur_null);
		}

		void ComputeShadowMips(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, LightingBufferSet* lightingBufferSet)
		{
			ur_uint mipCount = (ur_uint)lightingBufferSet->subresources[LightingImageUsage_DirectShadow].size();
			ur_uint lastMipId = mipCount - 1;
			
			// update descriptor table
			// common constant buffer is expected to be uploaded during rasterization pass

			GrafDescriptorTable* descriptorTable = this->shadowMipsDescTable->GetFrameObject();
			descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
			descriptorTable->SetImage(g_ShadowResultDescriptor, lightingBufferSet->subresources[LightingImageUsage_DirectShadow][0].get());
			descriptorTable->SetRWImage(g_ShadowMip1Descriptor, lightingBufferSet->subresources[LightingImageUsage_DirectShadow][std::min(1u, lastMipId)].get());
			descriptorTable->SetRWImage(g_ShadowMip2Descriptor, lightingBufferSet->subresources[LightingImageUsage_DirectShadow][std::min(2u, lastMipId)].get());
			descriptorTable->SetRWImage(g_ShadowMip3Descriptor, lightingBufferSet->subresources[LightingImageUsage_DirectShadow][std::min(3u, lastMipId)].get());
			descriptorTable->SetRWImage(g_ShadowMip4Descriptor, lightingBufferSet->subresources[LightingImageUsage_DirectShadow][std::min(4u, lastMipId)].get());

			// compute

			grafCmdList->BindComputePipeline(this->shadowMipsPipelineState.get());
			grafCmdList->BindComputeDescriptorTable(descriptorTable, this->shadowMipsPipelineState.get());

			static const ur_uint3 groupSize = { 16, 16, 1 };
			const ur_uint3& bufferSize = lightingBufferSet->images[0]->GetDesc().Size;
			grafCmdList->Dispatch((bufferSize.x - 1) / groupSize.x + 1, (bufferSize.y - 1) / groupSize.y + 1, 1);
		}

		void BlurLightingResult(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet,
			GrafImage* sourceImage, GrafImage* targetImage, SpatialFilterSettings filterSettings)
		{
			GrafImage* workImages[] = {
				sourceImage,
				targetImage
			};
			ur_uint srcImageId = 0;
			ur_uint dstImageId = 1;
			ur_uint passCount = (filterSettings.PassCount / 2) * 2; // multiple of 2 to avoid copying blured image back to source
			for (ur_uint ipass = 0; ipass < passCount; ++ipass)
			{
				grafCmdList->ImageMemoryBarrier(workImages[srcImageId], GrafImageState::Current, GrafImageState::ComputeRead);
				grafCmdList->ImageMemoryBarrier(workImages[dstImageId], GrafImageState::Current, GrafImageState::ComputeReadWrite);

				// update & upload pass constants

				BlurPassConstants passConstants;
				passConstants.PassIdx = ipass;
				passConstants.KernelSizeFactor = (1 << ipass);
				passConstants.EdgeParams = filterSettings.EdgeParams;

				GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
				Allocation passCBAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(BlurPassConstants));
				dynamicCB->Write((ur_byte*)&passConstants, sizeof(passConstants), 0, passCBAlloc.Offset);

				// update descriptor table
				// common constant buffer is expected to be uploaded during rasterization pass

				this->blurDescTableIdx = (this->blurDescTableIdx + 1) % BlurPassCountPerFrame;
				GrafDescriptorTable* descriptorTable = this->blurDescTables[this->blurDescTableIdx]->GetFrameObject();
				descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
				descriptorTable->SetConstantBuffer(g_BlurPassCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), passCBAlloc.Offset, passCBAlloc.Size);
				descriptorTable->SetImage(g_GeometryDepthDescriptor, renderTargetSet->images[RenderTargetImageUsage_Depth]);
				descriptorTable->SetImage(g_GeometryImage0Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry0]);
				descriptorTable->SetImage(g_GeometryImage1Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry1]);
				descriptorTable->SetImage(g_GeometryImage2Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry2]);
				descriptorTable->SetImage(g_BlurSourceDescriptor, workImages[srcImageId]);
				descriptorTable->SetRWImage(g_BlurTargetDescriptor, workImages[dstImageId]);

				// compute

				grafCmdList->BindComputePipeline(this->blurPipelineState.get());
				grafCmdList->BindComputeDescriptorTable(descriptorTable, this->blurPipelineState.get());

				static const ur_uint3 groupSize = { 8, 8, 1 };
				const ur_uint3& bufferSize = sourceImage->GetDesc().Size;
				grafCmdList->Dispatch((bufferSize.x - 1) / groupSize.x + 1, (bufferSize.y - 1) / groupSize.y + 1, 1);

				// swap
				srcImageId = !srcImageId;
				dstImageId = !dstImageId;
			}
		}

		void AccumulateLightingResult(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, LightingBufferSet* lightingBufferSet)
		{
			// update descriptor table
			// common constant buffer is expected to be uploaded during rasterization pass

			GrafDescriptorTable* descriptorTable = this->accumulationDescTable->GetFrameObject();
			descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
			descriptorTable->SetSampler(g_SamplerBilinearDescriptor, this->samplerBilinear.get());
			descriptorTable->SetSampler(g_SamplerTrilinearDescriptor, this->samplerTrilinear.get());
			descriptorTable->SetImage(g_GeometryDepthDescriptor, renderTargetSet->images[RenderTargetImageUsage_Depth]);
			descriptorTable->SetImage(g_GeometryImage0Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry0]);
			descriptorTable->SetImage(g_GeometryImage1Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry1]);
			descriptorTable->SetImage(g_GeometryImage2Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry2]);
			descriptorTable->SetImage(g_DepthHistoryDescriptor, renderTargetSet->images[RenderTargetImageUsage_DepthHistory]);
			descriptorTable->SetImage(g_ShadowHistoryDescriptor, lightingBufferSet->images[LightingImageUsage_DirectShadowHistory].get());
			descriptorTable->SetImage(g_TracingHistoryDescriptor, lightingBufferSet->images[LightingImageUsage_TracingInfoHistory].get());
			descriptorTable->SetImage(g_ShadowMipsDescriptor, lightingBufferSet->mipsSubresource[LightingImageUsage_DirectShadow].get());
			descriptorTable->SetImage(g_IndirectLightHistoryDescriptor, lightingBufferSet->images[LightingImageUsage_IndirectLightHistory].get());
			descriptorTable->SetRWImage(g_ShadowTargetDescriptor, lightingBufferSet->subresources[LightingImageUsage_DirectShadow][0].get());
			descriptorTable->SetRWImage(g_TracingInfoTargetDescriptor, lightingBufferSet->images[LightingImageUsage_TracingInfo].get());
			descriptorTable->SetRWImage(g_IndirectLightTargetDescriptor, lightingBufferSet->images[LightingImageUsage_IndirectLight].get());
			
			// compute

			grafCmdList->BindComputePipeline(this->accumulationPipelineState.get());
			grafCmdList->BindComputeDescriptorTable(descriptorTable, this->accumulationPipelineState.get());

			static const ur_uint3 groupSize = { 8, 8, 1 };
			const ur_uint3& bufferSize = lightingBufferSet->images[0]->GetDesc().Size;
			grafCmdList->Dispatch((bufferSize.x - 1) / groupSize.x + 1, (bufferSize.y - 1) / groupSize.y + 1, 1);

			// TEMP: reprojection precision test
			#if (0)
			static float clipDepth = 0.12f;
			static ur_int2 imagePos = ur_int2(25, 37);
			for (ur_int iy = 0; iy < (ur_int)sceneConstants.TargetSize.y; ++iy)
			{
				for (ur_int ix = 0; ix < (ur_int)sceneConstants.TargetSize.x; ++ix)
				{
					imagePos.x = ix;
					imagePos.y = iy;
					ur_float2 uvPos;
					uvPos.x = (ur_float(imagePos.x) + 0.5f) * sceneConstants.TargetSize.z;
					uvPos.y = (ur_float(imagePos.y) + 0.5f) * sceneConstants.TargetSize.w;
					ur_float4 clipPos;
					clipPos.x = uvPos.x * 2.0f - 1.0f;
					clipPos.y = (1.0f - uvPos.y) * 2.0f - 1.0f;
					clipPos.z = clipDepth;
					clipPos.w = 1.0f;
					ur_float4 worldPos;
					worldPos = sceneConstants.ViewProjInv.Multiply(clipPos);
					worldPos /= worldPos.w;
					ur_float4 clipPosPrev;
					clipPosPrev = sceneConstants.ViewProjPrev.Multiply(worldPos);
					clipPosPrev /= clipPosPrev.w;
					ur_float2 imagePosPrev;
					imagePosPrev.x = (clipPosPrev.x + 1.0f) * 0.5f * sceneConstants.TargetSize.x;
					imagePosPrev.y = (clipPosPrev.y * -1.0f + 1.0f) * 0.5f * sceneConstants.TargetSize.y;
					ur_int2 imagePos2;
					imagePos2.x = (ur_int)imagePosPrev.x;
					imagePos2.y = (ur_int)imagePosPrev.y;
					if (imagePos.x != imagePos2.x || imagePos.y != imagePos2.y)
					{
						int h = 0;
					}
				}
			}
			#endif
		}

		void ComputeLighting(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, LightingBufferSet* lightingBufferSet, GrafRenderTarget* lightingTarget)
		{
			// update descriptor table
			// common constant buffer is expected to be uploaded during rasterization pass

			GrafDescriptorTable* descriptorTable = this->lightingDescTable->GetFrameObject();
			descriptorTable->SetConstantBuffer(g_SceneCBDescriptor, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
			descriptorTable->SetSampler(g_SamplerBilinearWrapDescriptor, this->samplerBilinearWrap.get());
			descriptorTable->SetSampler(g_SamplerTrilinearDescriptor, this->samplerTrilinear.get());
			descriptorTable->SetImage(g_BlueNoiseImageDescriptor, this->blueNoiseImage.get());
			descriptorTable->SetImage(g_GeometryDepthDescriptor, renderTargetSet->images[RenderTargetImageUsage_Depth]);
			descriptorTable->SetImage(g_GeometryImage0Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry0]);
			descriptorTable->SetImage(g_GeometryImage1Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry1]);
			descriptorTable->SetImage(g_GeometryImage2Descriptor, renderTargetSet->images[RenderTargetImageUsage_Geometry2]);
			descriptorTable->SetImage(g_ShadowResultDescriptor, lightingBufferSet->images[LightingImageUsage_DirectShadow].get());
			descriptorTable->SetImage(g_TracingInfoDescriptor, lightingBufferSet->images[LightingImageUsage_TracingInfo].get());
			descriptorTable->SetImage(g_PrecomputedSkyDescriptor, this->skyImage.get());
			descriptorTable->SetImage(g_IndirectLightDescriptor, lightingBufferSet->images[LightingImageUsage_IndirectLight].get());
			descriptorTable->SetRWImage(g_LightingTargetDescriptor, lightingTarget->GetImage(0));

			// compute

			grafCmdList->BindComputePipeline(this->lightingPipelineState.get());
			grafCmdList->BindComputeDescriptorTable(descriptorTable, this->lightingPipelineState.get());

			static const ur_uint3 groupSize = { 8, 8, 1 };
			const ur_uint3& targetSize = lightingTarget->GetImage(0)->GetDesc().Size;
			grafCmdList->Dispatch((targetSize.x - 1) / groupSize.x + 1, (targetSize.y - 1) / groupSize.y + 1, 1);

			// swap current and history render targets and lighting buffers

			renderTargetSet->renderTargetStorage[0].swap(renderTargetSet->renderTargetStorage[1]);
			renderTargetSet->renderTarget = renderTargetSet->renderTargetStorage[0].get();
			for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
			{
				std::swap(renderTargetSet->images[imageId], renderTargetSet->images[RenderTargetHistoryFirst + imageId]);
			}
			for (ur_uint imageId = 0; imageId < LightingImageCount; ++imageId)
			{
				lightingBufferSet->images[imageId].swap(lightingBufferSet->images[LightingImageHistoryFirst + imageId]);
				ur_uint imageMipCount = (ur_uint)lightingBufferSet->subresources[imageId].size();
				for (ur_uint mipId = 0; mipId < imageMipCount; ++mipId)
				{
					lightingBufferSet->subresources[imageId][mipId].swap(lightingBufferSet->subresources[LightingImageHistoryFirst + imageId][mipId]);
				}
				lightingBufferSet->mipsSubresource[imageId].swap(lightingBufferSet->mipsSubresource[LightingImageHistoryFirst + imageId]);
			}
			renderTargetSet->resetHistory = false;
		}

		void DisplayImgui()
		{
			ur_int editableInt = 0;
			ur_float editableFloat3[3];
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::CollapsingHeader("Scene"))
			{
				ImGui::Checkbox("InstancesAnimationEnabled", &this->animationEnabled);
				editableInt = (int)this->sampleInstanceCount;
				ImGui::InputInt("InstanceCount", &editableInt);
				this->sampleInstanceCount = (ur_size)std::max(0, editableInt);
				ImGui::InputFloat("InstancesScatterRadius", &this->sampleInstanceScatterRadius);
				ImGui::InputFloat("InstancesCycleTime", &this->animationCycleTime);
				ImGui::DragFloat("DirectLightFactor", &this->sceneConstants.DirectLightFactor, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("IndirectLightFactor", &this->sceneConstants.IndirectLightFactor, 0.01f, 0.0f, 1.0f);
				if (ImGui::CollapsingHeader("Tweakers"))
				{
					ImGui::InputFloat4("DebugVec0", &this->debugVec0.x);
					ImGui::InputFloat4("DebugVec1", &this->debugVec1.x);
					ImGui::InputFloat4("DebugVec2", &this->debugVec2.x);
					ImGui::InputFloat4("DebugVec3", &this->debugVec3.x);
				}
				if (ImGui::CollapsingHeader("Material"))
				{
					ur_bool editableBool = (ur_bool)this->sceneConstants.OverrideMaterial;
					ImGui::Checkbox("Override", &editableBool);
					this->sceneConstants.OverrideMaterial = (ur_uint)editableBool;
					memcpy(editableFloat3, &this->sceneConstants.Material.BaseColor, sizeof(ur_float3));
					ImGui::InputFloat3("BaseColor", editableFloat3);
					this->sceneConstants.Material.BaseColor = editableFloat3;
					ImGui::InputFloat("Roughness", &this->sceneConstants.Material.Roughness);
					ImGui::InputFloat("Metallic", &this->sceneConstants.Material.Metallic);
					ImGui::InputFloat("Reflectance", &this->sceneConstants.Material.Reflectance);
				}
			}
		}
	};
	std::unique_ptr<DemoScene> demoScene;
	if (grafRenderer != ur_null)
	{
		demoScene.reset(new DemoScene(realm));
		demoScene->Initialize(rasterRenderPass.get());
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
			realm.GetLog().WriteLine("HybridRenderingApp: failed to initialize ImguiRender", Log::Error);
		}
	}

	// HDR rendering manager
	std::unique_ptr<HDRRender> hdrRender(new HDRRender(realm));
	{
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.LumWhite = 3.0f;
		hdrParams.LumAdaptationMin = 100.0;
		hdrParams.LumAdaptationMax = 1000.0;
		hdrParams.BloomThreshold = 4.0f;
		hdrParams.BloomIntensity = 0.5f;
		hdrRender->SetParams(hdrParams);
		res = hdrRender->Init(canvasWidth, canvasHeight, ur_null);
		if (Failed(res))
		{
			realm.GetLog().WriteLine("HybridRenderingApp: failed to initialize HDRRender", Log::Error);
			hdrRender.reset();
		}
	}

	// Main loop
	SceneState sceneState = SceneState::Run;
	ClockTime timer = Clock::now();
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (SceneState::Run == sceneState)
	{
		// process messages & input

		ClockTime timeBefore = Clock::now();
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) && msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// forward msg to WinInput system
			WinInput* winInput = static_cast<WinInput*>(realm.GetInput());
			winInput->ProcessMsg(msg);
		}
		if (msg.message == WM_QUIT || realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::Escape))
		{
			sceneState = SceneState::Finish;
			break;
		}
		auto inputDelay = Clock::now() - timeBefore;
		timer += inputDelay; // skip input delay

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

		auto UpdateFrameJobFunc = [&](Job::Context& ctx) -> void
		{
			// reset update progress fence
			ctx.progress = 0;

			// move timer
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f;  // to seconds

			ctx.progress++;

			// upadte light source(s)

			lightAnimationElapsedTime = lightCrntCycleFactor * lightCycleTime;
			if (lightAnimationEnabled) lightAnimationElapsedTime += elapsedTime;
			ur_float crntTimeFactor = (lightAnimationEnabled ? (lightAnimationElapsedTime / lightCycleTime) : lightCrntCycleFactor);
			ur_float modY;
			lightCrntCycleFactor = std::modf(crntTimeFactor, &modY);
			ur_float3 sunDir;
			const ur_float SunInclinationScale = 2.6f;
			const ur_float SunInclinationBias = 0.02f;
			sunDir.x = -cos(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor);
			sunDir.z = -sin(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor);
			sunDir.y = -powf(fabs(sin(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor)), 2.0f) * SunInclinationScale - SunInclinationBias;
			sunDir.Normalize();
			LightDesc& sunLight = lightingDesc.LightSources[0];
			if (LightType_Directional == sunLight.Type)
			{
				// main sun light
				sunLight.Direction = sunDir;
				//sunLight.Intensity = lerp(SolarIlluminanceEvening, SolarIlluminanceNoon, std::min(1.0f, -sunLight.Direction.y / SunInclinationScale));
				//sunLight.IntensityTopAtmosphere = SolarIlluminanceTopAtmosphere;
			}
			for (ur_uint i = 0; i < lightingDesc.LightSourceCount; ++i)
			{
				// point light(s)
				LightDesc& light = lightingDesc.LightSources[i];
				if (LightType_Spherical == light.Type)
				{
					ur_float2 pos2d = ur_float2(light.Position.x, light.Position.z);
					ur_float dist2d = pos2d.Length();
					ur_float2 dir2d;
					light.Position.x = -dist2d * cos(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor);
					light.Position.z = -dist2d * sin(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor);
					#if (SCENE_TYPE_SPONZA == SCENE_TYPE)
					light.Position.y = 2.0f + (cos(MathConst<ur_float>::Pi * 2.0f * crntTimeFactor) + 1.0f) * 4.0f;
					#endif
				}
			}

			ctx.progress++;

			// update demo scene

			if (demoScene != ur_null)
			{
				demoScene->Update(elapsedTime);
			}

			ctx.progress++;

		}; // end update func
		
		Job* updateFrameJob = ur_null;
		#if (UPDATE_ASYNC)
		auto updateFrameJobRef = realm.GetJobSystem().Add(JobPriority::High, ur_null, UpdateFrameJobFunc);
		updateFrameJob = updateFrameJobRef.get();
		#else
		Job updateFrameJobImmediate(realm.GetJobSystem(), ur_null, UpdateFrameJobFunc);
		updateFrameJobImmediate.Execute();
		updateFrameJob = &updateFrameJobImmediate;
		#endif

		// render frame

		auto RenderFrameJobFunc = [&](Job::Context& ctx) -> void
		{
			updateFrameJob->Wait(); // make sure async update is done

			static const ur_float4 DebugLabelColorMain = { 1.0f, 0.8f, 0.7f, 1.0f };
			static const ur_float4 DebugLabelColorPass = { 0.6f, 1.0f, 0.6f, 1.0f };
			static const ur_float4 DebugLabelColorRender = { 0.8f, 0.8f, 1.0f, 1.0f };

			GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
			GrafDevice *grafDevice = grafRenderer->GetGrafDevice();
			GrafCanvas *grafCanvas = grafRenderer->GetGrafCanvas();
			const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());
			
			GrafCommandList* grafCmdListCrnt = managedCommandList->GetFrameObject();
			grafCmdListCrnt->Begin();
			grafCmdListCrnt->BeginDebugLabel("DrawFrame", DebugLabelColorMain);

			GrafViewportDesc grafViewport = {};
			grafViewport.Width = (ur_float)canvasWidth;
			grafViewport.Height = (ur_float)canvasHeight;
			grafViewport.Near = 0.0f;
			grafViewport.Far = 1.0f;
			grafCmdListCrnt->SetViewport(grafViewport, true);

			GrafClearValue rtClearValue = { 0.8f, 0.9f, 1.0f, 0.0f };
			grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorClear);
			grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), rtClearValue);

			// rasterization pass
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "RasterizePass", DebugLabelColorPass);
				
				for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
				{
					ur_uint imageUsage = (imageId % RenderTargetImageCount);
					if (RenderTargetImageUsage_Depth == imageUsage)
					{
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::DepthStencilClear);
						grafCmdListCrnt->ClearDepthStencilImage(renderTargetSet->images[imageId], RenderTargetClearValues[imageUsage]);
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::DepthStencilWrite);
					}
					else
					{
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ColorClear);
						grafCmdListCrnt->ClearColorImage(renderTargetSet->images[imageId], RenderTargetClearValues[imageUsage]);
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ColorWrite);
					}
				}

				if (demoScene != ur_null)
				{
					for (auto& mesh : demoScene->meshes)
					{
						grafCmdListCrnt->BufferMemoryBarrier(mesh->vertexBuffer.get(), GrafBufferState::Current, GrafBufferState::VertexBuffer);
						grafCmdListCrnt->BufferMemoryBarrier(mesh->indexBuffer.get(), GrafBufferState::Current, GrafBufferState::IndexBuffer);
					}
				}

				grafCmdListCrnt->BeginRenderPass(rasterRenderPass.get(), renderTargetSet->renderTarget);

				if (demoScene != ur_null)
				{
					demoScene->Render(grafCmdListCrnt, renderTargetSet.get(), lightingBufferSet.get(), camera, lightingDesc, atmosphereDesc);
				}

				grafCmdListCrnt->EndRenderPass();
			}

			// sky precompute pass
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "SkyPrecomputePass", DebugLabelColorPass);

				if (demoScene != ur_null)
				{
					demoScene->UpdatePrecomputedSkyResources(grafCmdListCrnt);

					grafCmdListCrnt->ImageMemoryBarrier(demoScene->skyImage.get(), GrafImageState::Current, GrafImageState::ComputeReadWrite);

					demoScene->PrecomputeSky(grafCmdListCrnt);
				}
			}

			// ray tracing pass
			if (grafDeviceDesc->RayTracing.RayTraceSupported)
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "RayTracePass", DebugLabelColorPass);
				
				for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::RayTracingRead);
				}
				for (ur_uint imageId = RenderTargetHistoryFirst; imageId < RenderTargetImageCountWithHistory; ++imageId)
				{
					if (renderTargetSet->resetHistory)
					{
						ur_uint imageUsage = (imageId % RenderTargetImageCount);
						if (RenderTargetImageUsage_Depth == imageUsage)
						{
							grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::DepthStencilClear);
							grafCmdListCrnt->ClearDepthStencilImage(renderTargetSet->images[imageId], RenderTargetClearValues[imageUsage]);
						}
						else
						{
							grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ColorClear);
							grafCmdListCrnt->ClearColorImage(renderTargetSet->images[imageId], RenderTargetClearValues[imageUsage]);
						}
					}
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::RayTracingRead);
				}
				for (ur_uint imageId = 0; imageId < LightingImageCount; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::RayTracingReadWrite);
				}
				for (ur_uint imageId = LightingImageHistoryFirst; imageId < LightingImageCountWithHistory; ++imageId)
				{
					if (renderTargetSet->resetHistory)
					{
						ur_uint imageUsage = (imageId % LightingImageCount);
						grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ColorClear);
						grafCmdListCrnt->ClearColorImage(lightingBufferSet->images[imageId].get(), LightingBufferClearValues[imageUsage]);
					}
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::RayTracingRead);
				}

				if (demoScene != ur_null)
				{
					grafCmdListCrnt->ImageMemoryBarrier(demoScene->skyImage.get(), GrafImageState::Current, GrafImageState::RayTracingRead);

					for (auto& mesh : demoScene->meshes)
					{
						grafCmdListCrnt->BufferMemoryBarrier(mesh->vertexBuffer.get(), GrafBufferState::Current, GrafBufferState::RayTracingRead);
						grafCmdListCrnt->BufferMemoryBarrier(mesh->indexBuffer.get(), GrafBufferState::Current, GrafBufferState::RayTracingRead);
					}

					demoScene->RayTrace(grafCmdListCrnt, renderTargetSet.get(), lightingBufferSet.get());
				}
			}

			// denosing
			if (grafDeviceDesc->RayTracing.RayTraceSupported)
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "DenoisingPass", DebugLabelColorPass);

				for (ur_uint imageId = 0; imageId < RenderTargetImageCountWithHistory; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ComputeRead);
				}

				// blur current frame result
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "BlurPass", DebugLabelColorPass);

					for (ur_uint imageId = 0; imageId < LightingImageCount; ++imageId)
					{
						grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ComputeRead);
					}
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[LightingImageUsage_DirectShadowBlured].get(), GrafImageState::Current, GrafImageState::ComputeReadWrite);
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[LightingImageUsage_IndirectLightBlured].get(), GrafImageState::Current, GrafImageState::ComputeReadWrite);

					if (demoScene != ur_null)
					{
						demoScene->BlurLightingResult(grafCmdListCrnt, renderTargetSet.get(),
							lightingBufferSet->images[LightingImageUsage_DirectShadow].get(),
							lightingBufferSet->images[LightingImageUsage_DirectShadowBlured].get(),
							g_Settings.RayTracing.Shadow.SpatialFilter);
						demoScene->BlurLightingResult(grafCmdListCrnt, renderTargetSet.get(),
							lightingBufferSet->images[LightingImageUsage_IndirectLight].get(),
							lightingBufferSet->images[LightingImageUsage_IndirectLightBlured].get(),
							g_Settings.RayTracing.IndirectLight.SpatialFilter);
					}
				}

				// compute mips hierarchy
				auto& shadowResultMips = lightingBufferSet->subresources[LightingImageUsage_DirectShadow];
				const ur_size shadowResultMipCount = shadowResultMips.size();
				if (shadowResultMipCount > 1)
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "ShadowMipsPass", DebugLabelColorPass);

					grafCmdListCrnt->ImageMemoryBarrier(shadowResultMips[0].get(), shadowResultMips[0]->GetImage()->GetState(), GrafImageState::ComputeRead);
					for (ur_uint mipId = 1; mipId < shadowResultMipCount; ++mipId)
					{
						grafCmdListCrnt->ImageMemoryBarrier(shadowResultMips[mipId].get(), shadowResultMips[0]->GetImage()->GetState(), GrafImageState::ComputeReadWrite);
					}

					if (demoScene != ur_null)
					{
						demoScene->ComputeShadowMips(grafCmdListCrnt, renderTargetSet.get(), lightingBufferSet.get());
					}

					for (ur_uint mipId = 1; mipId < shadowResultMipCount; ++mipId)
					{
						grafCmdListCrnt->ImageMemoryBarrier(shadowResultMips[mipId].get(), GrafImageState::ComputeReadWrite, GrafImageState::ComputeRead);
					}
					// all mips are expected to be in ComputeRead state at this point, following call forces global image state update
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[LightingImageUsage_DirectShadow].get(), GrafImageState::ComputeRead, GrafImageState::ComputeRead);
				}

				// apply temporal accumulation filter
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "AccumulationPass", DebugLabelColorPass);

					for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
					{
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ComputeRead);
					}
					for (ur_uint imageId = RenderTargetHistoryFirst; imageId < RenderTargetImageCountWithHistory; ++imageId)
					{
						if (renderTargetSet->resetHistory)
						{
							ur_uint imageUsage = (imageId % RenderTargetImageCount);
							if (RenderTargetImageUsage_Depth == imageUsage)
							{
								grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::DepthStencilClear);
								grafCmdListCrnt->ClearDepthStencilImage(renderTargetSet->images[imageId], RenderTargetClearValues[imageUsage]);
							}
							else
							{
								grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ColorClear);
								grafCmdListCrnt->ClearColorImage(renderTargetSet->images[imageId], RenderTargetClearValues[imageUsage]);
							}
						}
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ComputeRead);
					}
					for (ur_uint imageId = LightingImageHistoryFirst; imageId < LightingImageCountWithHistory; ++imageId)
					{
						if (renderTargetSet->resetHistory)
						{
							ur_uint imageUsage = (imageId % LightingImageCount);
							grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ColorClear);
							grafCmdListCrnt->ClearColorImage(lightingBufferSet->images[imageId].get(), LightingBufferClearValues[imageUsage]);
						}
						grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ComputeRead);
					}
					for (ur_uint imageId = 0; imageId < LightingImageCount; ++imageId)
					{
						grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ComputeReadWrite);
					}
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->subresources[LightingImageUsage_DirectShadow][0].get(), lightingBufferSet->images[LightingImageUsage_DirectShadow]->GetState(), GrafImageState::ComputeReadWrite);
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->mipsSubresource[LightingImageUsage_DirectShadow].get(), lightingBufferSet->images[LightingImageUsage_DirectShadow]->GetState(), GrafImageState::ComputeRead);

					if (demoScene != ur_null)
					{
						demoScene->AccumulateLightingResult(grafCmdListCrnt, renderTargetSet.get(), lightingBufferSet.get());
					}

					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->subresources[LightingImageUsage_DirectShadow][0].get(), GrafImageState::Current, GrafImageState::ComputeRead);
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[LightingImageUsage_DirectShadow].get(), GrafImageState::ComputeRead, GrafImageState::ComputeRead);
				}

				// blur accumulated result
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "BlurPass", DebugLabelColorPass);

					for (ur_uint imageId = 0; imageId < LightingImageCount; ++imageId)
					{
						grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ComputeRead);
					}
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[LightingImageUsage_DirectShadowBlured].get(), GrafImageState::Current, GrafImageState::ComputeReadWrite);
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[LightingImageUsage_IndirectLightBlured].get(), GrafImageState::Current, GrafImageState::ComputeReadWrite);

					if (demoScene != ur_null)
					{
						demoScene->BlurLightingResult(grafCmdListCrnt, renderTargetSet.get(),
							lightingBufferSet->images[LightingImageUsage_IndirectLight].get(),
							lightingBufferSet->images[LightingImageUsage_IndirectLightBlured].get(),
							g_Settings.RayTracing.IndirectLight.SpatialPostFilter);
					}
				}
			}

			// lighting pass
			if (hdrRender != ur_null)
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "LightingPass", DebugLabelColorPass);
				
				for (ur_uint imageId = 0; imageId < RenderTargetImageCountWithHistory; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId], GrafImageState::Current, GrafImageState::ComputeRead);
				}
				for (ur_uint imageId = 0; imageId < LightingImageCountWithHistory; ++imageId)
				{
					if (!grafDeviceDesc->RayTracing.RayTraceSupported)
					{
						ur_uint imageUsage = (imageId % LightingImageCount);
						grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ColorClear);
						grafCmdListCrnt->ClearColorImage(lightingBufferSet->images[imageId].get(), LightingBufferClearValues[imageUsage]);
					}
					grafCmdListCrnt->ImageMemoryBarrier(lightingBufferSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ComputeRead);
				}
				grafCmdListCrnt->ImageMemoryBarrier(hdrRender->GetHDRRenderTarget()->GetImage(0), GrafImageState::Current, GrafImageState::ComputeReadWrite);

				if (demoScene != ur_null)
				{
					grafCmdListCrnt->ImageMemoryBarrier(demoScene->skyImage.get(), GrafImageState::Current, GrafImageState::ComputeRead);

					demoScene->ComputeLighting(grafCmdListCrnt, renderTargetSet.get(), lightingBufferSet.get(), hdrRender->GetHDRRenderTarget());
				}
			}

			// post effects
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "PostEFfects", DebugLabelColorPass);

				// resolve HDR input directly to renderer's swap chain RT
				if (hdrRender != ur_null)
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "ResolveHDRInput", DebugLabelColorRender);
					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					hdrRender->Resolve(*grafCmdListCrnt, grafRenderer->GetCanvasRenderTarget());
				}
			}

			// foreground color render pass (drawing directly into swap chain image)
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "ForegroundPass", DebugLabelColorPass);
				grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
				grafCmdListCrnt->BeginRenderPass(grafRenderer->GetCanvasRenderPass(), grafRenderer->GetCanvasRenderTarget());

				GrafViewportDesc grafViewport = {};
				grafViewport.Width = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.x;
				grafViewport.Height = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.y;
				grafViewport.Near = 0.0f;
				grafViewport.Far = 1.0f;
				grafCmdListCrnt->SetViewport(grafViewport, true);

				// draw GUI
				static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
				static bool showGUI = true;
				showGUI = (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
				if (showGUI)
				{
					ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiCond_FirstUseEver);
					ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Once);
					ImGui::ShowMetricsWindow();

					grafRenderer->DisplayImgui();
					ImGui::Begin("Demo");
					if (ImGui::CollapsingHeader("Graphics System"))
					{
						ImGui::Text("%s", typeid(*grafRenderer->GetGrafSystem()).name());
						if (ImGui::Button("Recreate for Vulkan"))
							sceneState = SceneState::RecreateForVulkan;
						if (ImGui::Button("Recreate for DX12"))
							sceneState = SceneState::RecreateForDX12;
					}
					cameraControl.DisplayImgui();
					if (hdrRender != ur_null)
					{
						hdrRender->DisplayImgui();
					}
					if (ImGui::CollapsingHeader("Lighting"))
					{
						ImGui::Checkbox("AnimationEnabled", &lightAnimationEnabled);
						ImGui::InputFloat("CycleTime", &lightCycleTime);
						ImGui::DragFloat("CrntCycleFactor", &lightCrntCycleFactor, 0.01f, 0.0f, 1.0f);
						ImGui::ColorEdit3("Color", &lightingDesc.LightSources[0].Color.x);
						ImGui::InputFloat("Intensity", &lightingDesc.LightSources[0].Intensity);
						ImGui::InputFloat("IntensityTopAtmosphere", &lightingDesc.LightSources[0].IntensityTopAtmosphere);
						if (sphericalLight1Ptr != nullptr)
						{
							ImGui::InputFloat("SpherLightSize", &sphericalLight1Ptr->Size);
							ImGui::InputFloat("SpherLightIntensity", &sphericalLight1Ptr->Intensity);
							ImGui::ColorEdit3("SpherLightColor", &sphericalLight1Ptr->Color.x);
						}
					}
					if (ImGui::CollapsingHeader("Atmosphere"))
					{
						ur_int editableInt;
						editableInt = (ur_int)g_Settings.SkyImageSize.x;
						ImGui::InputInt("PrecomputeWidth", &editableInt);
						g_Settings.SkyImageSize.x = ur_uint(std::max(0, std::min(4096, editableInt)));
						editableInt = (ur_int)g_Settings.SkyImageSize.y;
						ImGui::InputInt("PrecomputeHeight", &editableInt);
						g_Settings.SkyImageSize.y = ur_uint(std::max(0, std::min(4096, editableInt)));
						ImGui::InputFloat("InnerRadius", &atmosphereDesc.InnerRadius);
						ImGui::InputFloat("OuterRadius", &atmosphereDesc.OuterRadius);
						ImGui::DragFloat("ScaleDepth", &atmosphereDesc.ScaleDepth, 0.01f, 0.0f, 1.0f);
						ImGui::InputFloat("Kr", &atmosphereDesc.Kr);
						ImGui::InputFloat("Km", &atmosphereDesc.Km);
						ImGui::DragFloat("G", &atmosphereDesc.G, 0.01f, -1.0f, 1.0f);
						ImGui::InputFloat("D", &atmosphereDesc.D);
					}
					if (demoScene != ur_null)
					{
						demoScene->DisplayImgui();
					}
					if (ImGui::CollapsingHeader("RayTracing"))
					{
						ur_uint lightingBufferDownscalePrev = g_Settings.RayTracing.LightingBufferDownscale;
						ur_int editableInt = (ur_int)g_Settings.RayTracing.LightingBufferDownscale;
						ImGui::InputInt("LightBufferDowncale", &editableInt);
						g_Settings.RayTracing.LightingBufferDownscale = (ur_uint)std::max(1, std::min(16, editableInt));
						canvasChanged |= (g_Settings.RayTracing.LightingBufferDownscale != lightingBufferDownscalePrev);
						ImGui::Checkbox("PerFrameJitter", &g_Settings.RayTracing.PerFrameJitter);
						if (ImGui::CollapsingHeader("Shadow"))
						{
							editableInt = (ur_int)g_Settings.RayTracing.Shadow.SamplesPerLight;
							ImGui::InputInt("SamplesPerLight", &editableInt);
							g_Settings.RayTracing.Shadow.SamplesPerLight = (ur_uint)std::max(0, std::min(1024, editableInt));
							editableInt = (ur_int)g_Settings.RayTracing.Shadow.TemporalFilter.AccumulationFrames;
							ImGui::InputInt("AccumulationFrames", &editableInt);
							g_Settings.RayTracing.Shadow.TemporalFilter.AccumulationFrames = (ur_uint)std::max(0, std::min(1024, editableInt));
							ImGui::InputFloat("TemporalTolerance", &g_Settings.RayTracing.Shadow.TemporalFilter.Tolerance);
							ImGui::InputFloat("TemporalThreshold", &g_Settings.RayTracing.Shadow.TemporalFilter.Threshold);
							editableInt = (ur_int)g_Settings.RayTracing.Shadow.SpatialFilter.PassCount;
							ImGui::InputInt("BlurPassCount", &editableInt);
							g_Settings.RayTracing.Shadow.SpatialFilter.PassCount = (ur_uint)std::max(0, std::min(ur_int(BlurPassCountPerFrame), editableInt));
							ImGui::InputFloat4("BlurEdgeParams", &g_Settings.RayTracing.Shadow.SpatialFilter.EdgeParams.x);
						}
						if (ImGui::CollapsingHeader("IndirectLight"))
						{
							editableInt = (ur_int)g_Settings.RayTracing.IndirectLight.SamplesPerFrame;
							ImGui::InputInt("SamplesPerFrame", &editableInt);
							g_Settings.RayTracing.IndirectLight.SamplesPerFrame = (ur_uint)std::max(0, std::min(1024, editableInt));
							editableInt = (ur_int)g_Settings.RayTracing.IndirectLight.TemporalFilter.AccumulationFrames;
							ImGui::InputInt("AccumulationFrames", &editableInt);
							g_Settings.RayTracing.IndirectLight.TemporalFilter.AccumulationFrames = (ur_uint)std::max(0, std::min(1024, editableInt));
							ImGui::InputFloat("TemporalTolerance", &g_Settings.RayTracing.IndirectLight.TemporalFilter.Tolerance);
							ImGui::InputFloat("TemporalThreshold", &g_Settings.RayTracing.IndirectLight.TemporalFilter.Threshold);
							editableInt = (ur_int)g_Settings.RayTracing.IndirectLight.BouncesCount;
							ImGui::InputInt("BouncesCount", &editableInt);
							g_Settings.RayTracing.IndirectLight.BouncesCount = (ur_uint)std::max(0, std::min(8, editableInt));
							editableInt = (ur_int)g_Settings.RayTracing.IndirectLight.SpatialFilter.PassCount;
							ImGui::InputInt("BlurPassCount", &editableInt);
							g_Settings.RayTracing.IndirectLight.SpatialFilter.PassCount = (ur_uint)std::max(0, std::min(ur_int(BlurPassCountPerFrame), editableInt));
							ImGui::InputFloat4("BlurEdgeParams", &g_Settings.RayTracing.IndirectLight.SpatialFilter.EdgeParams.x);
							editableInt = (ur_int)g_Settings.RayTracing.IndirectLight.SpatialPostFilter.PassCount;
							ImGui::InputInt("PostBlurPassCount", &editableInt);
							g_Settings.RayTracing.IndirectLight.SpatialPostFilter.PassCount = (ur_uint)std::max(0, std::min(ur_int(BlurPassCountPerFrame), editableInt));
						}
					}
					if (ImGui::CollapsingHeader("Canvas"))
					{
						ur_float canvasScalePrev = canvasScale;
						ImGui::InputFloat("ResolutionScale", &canvasScale);
						canvasScale = std::max(1.0f / 16.0f, std::min(4.0f, canvasScale));
						canvasChanged |= (canvasScale != canvasScalePrev);
					}
					ImGui::End();

					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "ImGui", DebugLabelColorRender);
					imguiRender->Render(*grafCmdListCrnt);
				}

				grafCmdListCrnt->EndRenderPass();
			}

			// finalize current command list
			grafCmdListCrnt->EndDebugLabel();
			grafCmdListCrnt->End();
			grafDevice->Record(grafCmdListCrnt);

		}; // end render func
		
		if (grafRenderer != ur_null)
		{
			// begin frame rendering
			grafRenderer->BeginFrame();

			// update render target(s)
			ur_uint canvasWidthNew = ur_uint(realm.GetCanvas()->GetClientBound().Width() * canvasScale);
			ur_uint canvasHeightNew = ur_uint(realm.GetCanvas()->GetClientBound().Height() * canvasScale);
			if (canvasValid && (canvasChanged || canvasWidth != canvasWidthNew || canvasHeight != canvasHeightNew))
			{
				canvasChanged = false;
				canvasWidth = canvasWidthNew;
				canvasHeight = canvasHeightNew;
				// use prev frame command list to make sure frame buffer objects are destroyed only when it is no longer used
				DestroyFrameBufferObjects(managedCommandList->GetFrameObject(grafRenderer->GetPrevRecordedFrameIdx()));
				// recreate frame buffer objects for new canvas dimensions
				InitFrameBufferObjects(canvasWidth, canvasHeight);
				// reinit HDR renderer images
				if (hdrRender != ur_null)
				{
					hdrRender->Init(canvasWidth, canvasHeight, ur_null);
				}
			}

			Job* renderFrameJob = ur_null;
			#if (RENDER_ASYNC)
			auto renderFrameJobRef = realm.GetJobSystem().Add(JobPriority::High, ur_null, RenderFrameJobFunc);
			renderFrameJob = renderFrameJobRef.get();
			#else
			Job renderFrameJobImmediate(realm.GetJobSystem(), ur_null, RenderFrameJobFunc);
			renderFrameJobImmediate.Execute();
			renderFrameJob = &renderFrameJobImmediate;
			#endif
			renderFrameJob->Wait();

			// finalize & move to next frame
			grafRenderer->EndFrameAndPresent();
		}
	
	} // end main loop

	if (grafRenderer != ur_null)
	{
		// make sure there are no resources still used on gpu before destroying
		grafRenderer->GetGrafDevice()->WaitIdle();
	}

	// destroy scene objects
	demoScene.reset();
	hdrRender.reset();

	// destroy GRAF objects
	DestroyGrafObjects();

	// destroy realm objects
	realm.RemoveComponent<ImguiRender>();
	realm.RemoveComponent<GenericRender>();
	realm.RemoveComponent<GrafRenderer>();

	return sceneState;
}