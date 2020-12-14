
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
#include "Graf/GrafRenderer.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"
#include "HDRRender/HDRRender.h"
#include "Atmosphere/Atmosphere.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

#define UPDATE_ASYNC 1
#define RENDER_ASYNC 0

int HybridRenderingApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"HybridRenderingDemo"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));
	ur_uint canvasDenom = 1;
	ur_uint canvasWidth = realm.GetCanvas()->GetClientBound().Width() / canvasDenom;
	ur_uint canvasHeight = realm.GetCanvas()->GetClientBound().Height() / canvasDenom;
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
		realm.GetLog().WriteLine("HybridRenderingApp: failed to initialize GrafRenderer", Log::Error);
	}

	// initialize GRAF objects
	
	enum RenderTargetImageUsage
	{
		RenderTargetImageUsage_Depth = 0,
		RenderTargetImageUsage_Geometry0, // xyz: baseColor; w: materialTypeID;
		RenderTargetImageUsage_Geometry1, // xy: normal; z: roughness; w: reflectance;
		RenderTargetImageCount,
		RenderTargetColorImageCount = RenderTargetImageCount - 1
	};
	
	static const GrafFormat RenderTargetImageFormat[RenderTargetImageCount] = {
		GrafFormat::D24_UNORM_S8_UINT,
		GrafFormat::R8G8B8A8_UNORM,
		GrafFormat::R8G8B8A8_UNORM,
	};

	static GrafClearValue RenderTargetClearValues[RenderTargetImageCount] = {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
	};
	
	struct RenderTargetSet
	{
		std::unique_ptr<GrafImage> images[RenderTargetImageCount];
		std::unique_ptr<GrafRenderTarget> renderTarget;
	};

	std::vector<std::unique_ptr<GrafCommandList>> cmdListPerFrame;
	std::unique_ptr<GrafRenderPass> rasterRenderPass;
	std::unique_ptr<RenderTargetSet> renderTargetSet;

	auto& DestroyRenderTargetObjects = [&](GrafCommandList* syncCmdList = ur_null)
	{
		if (ur_null == renderTargetSet)
			return;

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
	};

	auto& InitRenderTargetObjects = [&](ur_uint width, ur_uint height) -> Result
	{
		Result res(Success);
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		DestroyRenderTargetObjects(ur_null);

		renderTargetSet.reset(new RenderTargetSet());

		// images
		GrafImage* renderTargetImages[RenderTargetImageCount];
		for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
		{
			auto& image = renderTargetSet->images[imageId];
			res = grafSystem->CreateImage(image);
			if (Failed(res)) return res;
			ur_uint imageUsageFlags = ur_uint(GrafImageUsageFlag::TransferDst) | ur_uint(GrafImageUsageFlag::ShaderInput);
			if (RenderTargetImageUsage_Depth == imageId)
			{
				imageUsageFlags |= ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget);
			}
			else
			{
				imageUsageFlags |= ur_uint(GrafImageUsageFlag::ColorRenderTarget);
			}
			GrafImageDesc imageDesc = {
				GrafImageType::Tex2D,
				RenderTargetImageFormat[imageId],
				ur_uint3(width, height, 1), 1,
				imageUsageFlags,
				ur_uint(GrafDeviceMemoryFlag::GpuLocal)
			};
			res = image->Initialize(grafDevice, { imageDesc });
			if (Failed(res)) return res;
			renderTargetImages[imageId] = image.get();
		}

		// rasterization pass render target
		res = grafSystem->CreateRenderTarget(renderTargetSet->renderTarget);
		if (Failed(res)) return res;
		GrafRenderTarget::InitParams renderTargetParams = {
			rasterRenderPass.get(),
			renderTargetImages,
			ur_array_size(renderTargetImages)
		};
		res = renderTargetSet->renderTarget->Initialize(grafDevice, renderTargetParams);
		if (Failed(res)) return res;

		return res;
	};

	auto& DestroyGrafObjects = [&]()
	{
		// here we expect that device is idle
		DestroyRenderTargetObjects();
		rasterRenderPass.reset();
		cmdListPerFrame.clear();
	};

	auto& InitGrafObjects = [&]() -> Result
	{
		Result res(Success);
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// command lists (per frame)
		ur_uint frameCount = grafRenderer->GetRecordedFrameCount();
		cmdListPerFrame.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateCommandList(cmdListPerFrame[iframe]);
			if (Failed(res)) break;
			res = cmdListPerFrame[iframe]->Initialize(grafDevice);
			if (Failed(res)) break;
		}
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
		res = InitRenderTargetObjects(canvasWidth, canvasHeight);
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

		struct SceneConstants
		{
			ur_float4x4 viewProj;
		};

		enum MeshId
		{
			MeshId_Plane = 0,
			MeshId_Sphere,
			MeshId_Wuson,
			MeshId_MedievalBuilding,
			MeshCount
		};

		class Mesh : public GrafEntity
		{
		public:

			struct Vertex
			{
				ur_float3 pos;
				ur_float3 norm;
			};

			typedef ur_uint32 Index;

			std::unique_ptr<GrafBuffer> vertexBuffer;
			std::unique_ptr<GrafBuffer> indexBuffer;
			ur_uint verticesCount;
			ur_uint indicesCount;

			Mesh(GrafSystem &grafSystem) : GrafEntity(grafSystem) {}
			~Mesh() {}

			void Initialize(GrafRenderer* grafRenderer,
				const Vertex* vertices, ur_uint verticesCount,
				const Index* indices, ur_uint indicesCount)
			{
				GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
				GrafDevice* grafDevice = grafRenderer->GetGrafDevice();

				this->verticesCount = verticesCount;
				this->indicesCount = indicesCount;

				// vertex attributes buffer

				GrafBuffer::InitParams VBParams;
				VBParams.BufferDesc.Usage =
					ur_uint(GrafBufferUsageFlag::VertexBuffer) |
					ur_uint(GrafBufferUsageFlag::TransferDst) |
					ur_uint(GrafBufferUsageFlag::StorageBuffer) |
					ur_uint(GrafBufferUsageFlag::RayTracing) |
					ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress);
				VBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
				VBParams.BufferDesc.SizeInBytes = verticesCount * sizeof(Vertex);

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
				IBParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
				IBParams.BufferDesc.SizeInBytes = indicesCount * sizeof(Index);

				grafSystem->CreateBuffer(this->indexBuffer);
				this->indexBuffer->Initialize(grafDevice, IBParams);

				grafRenderer->Upload((ur_byte*)indices, this->indexBuffer.get(), IBParams.BufferDesc.SizeInBytes);
			}
		};

		GrafRenderer* grafRenderer;
		std::vector<std::unique_ptr<Mesh>> meshes;
		std::unique_ptr<GrafDescriptorTableLayout> rasterDescTableLayout;
		std::vector<std::unique_ptr<GrafDescriptorTable>> rasterDescTablePerFrame;
		std::unique_ptr<GrafPipeline> rasterPipelineState;
		std::unique_ptr<GrafDescriptorTableLayout> lightingDescTableLayout;
		std::vector<std::unique_ptr<GrafDescriptorTable>> lightingDescTablePerFrame;
		std::unique_ptr<GrafComputePipeline> lightingPipelineState;
		std::unique_ptr<GrafShaderLib> shaderLib;
		std::unique_ptr<GrafShader> shaderVertex;
		std::unique_ptr<GrafShader> shaderPixel;
		SceneConstants sceneConstants;
		Allocation sceneCBCrntFrameAlloc;

		DemoScene(Realm& realm) : RealmEntity(realm), grafRenderer(ur_null)
		{
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

			// load mesh(es)

			GrafUtils::MeshVertexElementFlags vertexMask = (ur_uint(GrafUtils::MeshVertexElementFlag::Position) | ur_uint(GrafUtils::MeshVertexElementFlag::Normal)); // load only subset of attributes compatible with Mesh::Vertex
			std::string modelResName[] = {
				"../Res/Models/Plane.obj",
				"../Res/Models/Sphere.obj",
				"../Res/Models/Wuson.obj",
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
						mesh->Initialize(this->grafRenderer,
							(Mesh::Vertex*)meshData.Vertices.data(), ur_uint(meshData.Vertices.size() / sizeof(Mesh::Vertex)),
							(Mesh::Index*)meshData.Indices.data(), ur_uint(meshData.Indices.size() / sizeof(Mesh::Index)));
						this->meshes.emplace_back(std::move(mesh));
					}
				}
			}

			// load shaders

			enum ShaderLibId
			{
				ShaderLibId_ComputeLighting,
				ShaderLibId_Dummy,
			};
			GrafShaderLib::EntryPoint shaderLibEntries[] = {
				{ "ComputeLighting", GrafShaderType::Compute },
				{ "dummyShaderToMakeFXCHappy", GrafShaderType::Compute },
			};
			GrafUtils::CreateShaderLibFromFile(*grafDevice, "HybridRendering_lib.spv", shaderLibEntries, ur_array_size(shaderLibEntries), this->shaderLib);
			GrafUtils::CreateShaderFromFile(*grafDevice, "HybridRendering_vs.spv", GrafShaderType::Vertex, this->shaderVertex);
			GrafUtils::CreateShaderFromFile(*grafDevice, "HybridRendering_ps.spv", GrafShaderType::Pixel, this->shaderPixel);

			// rasterization descriptor table

			GrafDescriptorRangeDesc rasterDescTableLayoutRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
			};
			GrafDescriptorTableLayoutDesc rasterDescTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Vertex) |
				ur_uint(GrafShaderStageFlag::Pixel),
				rasterDescTableLayoutRanges, ur_array_size(rasterDescTableLayoutRanges)
			};
			grafSystem->CreateDescriptorTableLayout(this->rasterDescTableLayout);
			this->rasterDescTableLayout->Initialize(grafDevice, { rasterDescTableLayoutDesc });
			this->rasterDescTablePerFrame.resize(grafRenderer->GetRecordedFrameCount());
			for (auto& descTable : this->rasterDescTablePerFrame)
			{
				grafSystem->CreateDescriptorTable(descTable);
				descTable->Initialize(grafDevice, { this->rasterDescTableLayout.get() });
			}

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
					{ GrafFormat::R32G32B32_SFLOAT, 0 },
					{ GrafFormat::R32G32B32_SFLOAT, 12 }
				};
				GrafVertexInputDesc vertexInputs[] = { {
					GrafVertexInputType::PerVertex, 0, sizeof(Mesh::Vertex),
					vertexElements, ur_array_size(vertexElements)
				} };
				GrafColorBlendOpDesc colorTargetBlendOpDesc[RenderTargetColorImageCount] = {
					GrafColorBlendOpDesc::Default,
					GrafColorBlendOpDesc::Default
				};
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
				pipelineParams.CullMode = GrafCullMode::Back;
				pipelineParams.DepthTestEnable = true;
				pipelineParams.DepthWriteEnable = true;
				pipelineParams.DepthCompareOp = GrafCompareOp::LessOrEqual;
				pipelineParams.ColorBlendOpDesc = colorTargetBlendOpDesc;
				pipelineParams.ColorBlendOpDescCount = ur_array_size(colorTargetBlendOpDesc);
				this->rasterPipelineState->Initialize(grafDevice, pipelineParams);
			}

			// lighting descriptor table

			GrafDescriptorRangeDesc lightingDescTableLayoutRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
				{ GrafDescriptorType::Texture, 0, 3 },
				{ GrafDescriptorType::RWTexture, 0, 1 },
			};
			GrafDescriptorTableLayoutDesc lightingDescTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute) |
				ur_uint(GrafShaderStageFlag::AllRayTracing),
				lightingDescTableLayoutRanges, ur_array_size(lightingDescTableLayoutRanges)
			};
			grafSystem->CreateDescriptorTableLayout(this->lightingDescTableLayout);
			this->lightingDescTableLayout->Initialize(grafDevice, { lightingDescTableLayoutDesc });
			this->lightingDescTablePerFrame.resize(grafRenderer->GetRecordedFrameCount());
			for (auto& descTable : this->lightingDescTablePerFrame)
			{
				grafSystem->CreateDescriptorTable(descTable);
				descTable->Initialize(grafDevice, { this->lightingDescTableLayout.get() });
			}

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
		}

		void Update(ur_float elapsedSeconds)
		{
			// do some animation here
		}

		void Render(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, Camera& camera)
		{
			// update & upload frame constants

			sceneConstants.viewProj = camera.GetViewProj();

			GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
			this->sceneCBCrntFrameAlloc = this->grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SceneConstants));
			dynamicCB->Write((ur_byte*)&sceneConstants, sizeof(sceneConstants), 0, this->sceneCBCrntFrameAlloc.Offset);

			// update descriptor table

			GrafDescriptorTable* descriptorTable = this->rasterDescTablePerFrame[this->grafRenderer->GetCurrentFrameId()].get();
			descriptorTable->SetConstantBuffer(0, dynamicCB, this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);

			// rasterize meshes

			grafCmdList->BindPipeline(this->rasterPipelineState.get());
			grafCmdList->BindDescriptorTable(descriptorTable, this->rasterPipelineState.get());
			for (auto& mesh : this->meshes)
			{
				ur_uint instanceCount = 1; // TODO
				grafCmdList->BindVertexBuffer(mesh->vertexBuffer.get(), 0);
				grafCmdList->BindIndexBuffer(mesh->indexBuffer.get(), (sizeof(Mesh::Index) == 2 ? GrafIndexType::UINT16 : GrafIndexType::UINT32));
				grafCmdList->DrawIndexed(mesh->indicesCount, instanceCount, 0, 0, 0);
			}
		}

		void RayTrace(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, Camera& camera)
		{
			// TODO
		}

		void ComputeLighting(GrafCommandList* grafCmdList, RenderTargetSet* renderTargetSet, GrafRenderTarget* lightingTarget)
		{
			// update descriptor table
			// common frame constant buffer is expected to be uploaded during rasterization pass

			GrafDescriptorTable* descriptorTable = this->lightingDescTablePerFrame[this->grafRenderer->GetCurrentFrameId()].get();
			descriptorTable->SetConstantBuffer(0, this->grafRenderer->GetDynamicConstantBuffer(), this->sceneCBCrntFrameAlloc.Offset, this->sceneCBCrntFrameAlloc.Size);
			for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
			{
				descriptorTable->SetImage(imageId, renderTargetSet->images[imageId].get());
			}
			descriptorTable->SetRWImage(0, lightingTarget->GetImage(0));

			// compute

			grafCmdList->BindComputePipeline(this->lightingPipelineState.get());
			grafCmdList->BindComputeDescriptorTable(descriptorTable, this->lightingPipelineState.get());

			static const ur_uint3 groupSize = { 8, 8, 1 };
			const ur_uint3& targetSize = lightingTarget->GetImage(0)->GetDesc().Size;
			grafCmdList->Dispatch((targetSize.x - 1) / groupSize.x + 1, (targetSize.y - 1) / groupSize.y + 1, 1);
		}

		void ShowImgui()
		{
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::CollapsingHeader("DemoScene"))
			{
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
		hdrParams.BloomThreshold = 4.0f;
		hdrRender->SetParams(hdrParams);
		res = hdrRender->Init(canvasWidth, canvasHeight, ur_null);
		if (Failed(res))
		{
			realm.GetLog().WriteLine("VoxelPlanetApp: failed to initialize HDRRender", Log::Error);
			hdrRender.reset();
		}
	}

	// setup main camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	cameraControl.SetTargetPoint(ur_float3(0.0f));
	cameraControl.SetSpeed(4.0);
	camera.SetProjection(0.1f, 1.0e+4f, camera.GetFieldOFView(), camera.GetAspectRatio());
	camera.SetPosition(ur_float3(0.0f, 0.0f, -8.0f));
	camera.SetLookAt(cameraControl.GetTargetPoint(), cameraControl.GetWorldUp());

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

		auto& UpdateFrameJobFunc = [&](Job::Context& ctx) -> void
		{
			// reset update progress fence
			ctx.progress = 0;

			// move timer
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f;  // to seconds

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

		auto& RenderFrameJobFunc = [&](Job::Context& ctx) -> void
		{
			static const ur_float4 DebugLabelColorMain = { 1.0f, 0.8f, 0.7f, 1.0f };
			static const ur_float4 DebugLabelColorPass = { 0.6f, 1.0f, 0.6f, 1.0f };
			static const ur_float4 DebugLabelColorRender = { 0.8f, 0.8f, 1.0f, 1.0f };

			GrafSystem* grafSystem = grafRenderer->GetGrafSystem();
			GrafDevice *grafDevice = grafRenderer->GetGrafDevice();
			GrafCanvas *grafCanvas = grafRenderer->GetGrafCanvas();
			const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystem->GetPhysicalDeviceDesc(grafDevice->GetDeviceId());
			
			GrafCommandList* grafCmdListCrnt = cmdListPerFrame[grafRenderer->GetCurrentFrameId()].get();
			grafCmdListCrnt->Begin();
			grafCmdListCrnt->BeginDebugLabel("DrawFrame", DebugLabelColorMain);

			GrafViewportDesc grafViewport = {};
			grafViewport.Width = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.x;
			grafViewport.Height = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.y;
			grafViewport.Near = 0.0f;
			grafViewport.Far = 1.0f;
			grafCmdListCrnt->SetViewport(grafViewport, true);

			GrafClearValue rtClearValue = { 0.8f, 0.9f, 1.0f, 0.0f };
			grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::TransferDst);
			grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), rtClearValue);

			updateFrameJob->Wait(); // make sure async update is done

			// rasterization pass
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "RasterizePass", DebugLabelColorPass);
				for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId].get(), GrafImageState::Current, GrafImageState::TransferDst);
					if (RenderTargetImageUsage_Depth == imageId)
					{
						grafCmdListCrnt->ClearDepthStencilImage(renderTargetSet->images[imageId].get(), RenderTargetClearValues[imageId]);
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId].get(), GrafImageState::Current, GrafImageState::DepthStencilWrite);
					}
					else
					{
						grafCmdListCrnt->ClearColorImage(renderTargetSet->images[imageId].get(), RenderTargetClearValues[imageId]);
						grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ColorWrite);
					}
				}

				grafCmdListCrnt->BeginRenderPass(rasterRenderPass.get(), renderTargetSet->renderTarget.get());

				if (demoScene != ur_null)
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "DemoScene", DebugLabelColorRender);
					demoScene->Render(grafCmdListCrnt, renderTargetSet.get(), camera);
				}

				grafCmdListCrnt->EndRenderPass();
			}

			// ray tracing pass
			if (grafDeviceDesc->RayTracing.RayTraceSupported)
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "RayTracePass", DebugLabelColorPass);
				for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId].get(), GrafImageState::Current, GrafImageState::RayTracingRead);
				}

				if (demoScene != ur_null)
				{
					GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "DemoScene", DebugLabelColorRender);
					demoScene->RayTrace(grafCmdListCrnt, renderTargetSet.get(), camera);
				}
			}

			// lighting pass
			if (hdrRender != ur_null)
			{
				GrafUtils::ScopedDebugLabel label(grafCmdListCrnt, "LightingPass", DebugLabelColorPass);
				for (ur_uint imageId = 0; imageId < RenderTargetImageCount; ++imageId)
				{
					grafCmdListCrnt->ImageMemoryBarrier(renderTargetSet->images[imageId].get(), GrafImageState::Current, GrafImageState::ComputeRead);
				}
				grafCmdListCrnt->ImageMemoryBarrier(hdrRender->GetHDRRenderTarget()->GetImage(0), GrafImageState::Current, GrafImageState::ComputeReadWrite);
				
				demoScene->ComputeLighting(grafCmdListCrnt, renderTargetSet.get(), hdrRender->GetHDRRenderTarget());
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
					if (demoScene != ur_null)
					{
						demoScene->ShowImgui();
					}
					if (ImGui::CollapsingHeader("Canvas"))
					{
						int editableInt = canvasDenom;
						ImGui::InputInt("ResolutionDenominator", &editableInt);
						canvasDenom = editableInt;
					}

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
			ur_uint canvasWidthNew = realm.GetCanvas()->GetClientBound().Width() / canvasDenom;
			ur_uint canvasHeightNew = realm.GetCanvas()->GetClientBound().Height() / canvasDenom;
			if (canvasValid && (canvasWidth != canvasWidthNew || canvasHeight != canvasHeightNew))
			{
				canvasWidth = canvasWidthNew;
				canvasHeight = canvasHeightNew;
				// use prev frame command list to make sure RT objects are destroyed only when it is no longer used
				DestroyRenderTargetObjects(cmdListPerFrame[grafRenderer->GetPrevFrameId()].get());
				// recreate RT objects for new canvas dimensions
				InitRenderTargetObjects(canvasWidth, canvasHeight);
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

	return 0;
}