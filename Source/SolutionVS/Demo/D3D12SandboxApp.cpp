
#include "stdafx.h"
#include "D3D12SandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Core/Math.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

#if defined(UR_GRAF)

#define UR_GRAF_SYSTEM GrafSystemDX12

#include "Graf/DX12/GrafSystemDX12.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/GrafRenderer.h"

int D3D12SandboxApp::Run()
{

	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"GRAF Sandbox"));
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
		std::unique_ptr<GrafSystem> grafSystem(new UR_GRAF_SYSTEM(realm));
		res = grafSystem->Initialize(realm.GetCanvas());
		if (Failed(res)) break;

		// initialize renderer
		GrafRenderer::InitParams grafRendererParams = GrafRenderer::InitParams::Default;
		grafRendererParams.DeviceId = grafSystem->GetRecommendedDeviceId();
		grafRendererParams.CanvasParams = GrafCanvas::InitParams::Default;
		res = grafRenderer->Initialize(std::move(grafSystem), grafRendererParams);
		if (Failed(res)) break;

	} while (false);
	if (Failed(res))
	{
		realm.RemoveComponent<GrafRenderer>();
		grafRenderer = ur_null;
		realm.GetLog().WriteLine("DX12SandboxApp: failed to initialize GrafRenderer", Log::Error);
	}

	// sample mesh data
	struct VertexSample
	{
		ur_float3 pos;
		ur_float3 color;
	};
	VertexSample verticesSample[] = {
		{ {-0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f,-0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {-0.5f,-0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {-0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.5f,-0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } }
	};

	// sample vertex shader CB
	struct SampleCBData
	{
		ur_float4x4 Transform;
	};
	
	// initialize sample GRAF objects
	std::unique_ptr<GrafRenderPass> grafColorDepthPass;
	std::unique_ptr<GrafRenderTarget> grafColorDepthTarget;
	std::unique_ptr<GrafImage> grafRTImageColor;
	std::unique_ptr<GrafImage> grafRTImageDepth;
	std::unique_ptr<GrafSampler> grafDefaultSampler;
	std::unique_ptr<GrafShader> grafShaderSampleVS;
	std::unique_ptr<GrafShader> grafShaderSamplePS;
	std::unique_ptr<GrafBuffer> grafVBSample;
	std::unique_ptr<GrafImage> grafImageSample;
	std::unique_ptr<GrafDescriptorTableLayout> grafBindingLayoutSample;
	std::unique_ptr<GrafPipeline> grafPipelineSample;
	std::vector<std::unique_ptr<GrafCommandList>> grafMainCmdList;
	std::vector<std::unique_ptr<GrafDescriptorTable>> grafBindingSample;
	auto deinitializeGrafRenderTargetObjects = [&](GrafCommandList* deferredDestroyCmdList = ur_null) -> void {
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
	auto deinitializeGrafFrameObjects = [&]() -> void {
		grafBindingSample.clear();
		grafMainCmdList.clear();
	};
	auto deinitializeGrafObjects = [&]() -> void {
		// order matters!
		deinitializeGrafRenderTargetObjects();
		deinitializeGrafFrameObjects();
		grafColorDepthPass.reset();
		grafPipelineSample.reset();
		grafVBSample.reset();
		grafImageSample.reset();
		grafBindingLayoutSample.reset();
		grafShaderSampleVS.reset();
		grafShaderSamplePS.reset();
		grafDefaultSampler.reset();
	};
	Result grafRes = NotInitialized;
	auto initializeGrafObjects = [&]() -> void
	{
		if (ur_null == grafRenderer) return;
		GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
		GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

		// default sampler
		grafRes = grafRenderer->GetGrafSystem()->CreateSampler(grafDefaultSampler);
		if (Failed(grafRes)) return;
		grafRes = grafDefaultSampler->Initialize(grafDevice, { GrafSamplerDesc::Default });
		if (Failed(grafRes)) return;

		// vertex shader sample
		grafRes = GrafUtils::CreateShaderFromFile(*grafDevice, "sample_vs", GrafShaderType::Vertex, grafShaderSampleVS);
		if (Failed(grafRes)) return;

		// pixel shader sample
		grafRes = GrafUtils::CreateShaderFromFile(*grafDevice, "sample_ps", GrafShaderType::Pixel, grafShaderSamplePS);
		if (Failed(grafRes)) return;

		// shader bindings layout sample
		grafRes = grafSystem->CreateDescriptorTableLayout(grafBindingLayoutSample);
		if (Failed(grafRes)) return;
		GrafDescriptorRangeDesc grafBindingLayoutSampleRanges[] = {
			{ GrafDescriptorType::ConstantBuffer, 0, 1 },
			{ GrafDescriptorType::Sampler, 0, 1 },
			{ GrafDescriptorType::Texture, 0, 1 },
		};
		GrafDescriptorTableLayoutDesc grafBindingLayoutSampleDesc = {
			GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Vertex | (ur_uint)GrafShaderStageFlag::Pixel),
			grafBindingLayoutSampleRanges, ur_array_size(grafBindingLayoutSampleRanges)
		};
		grafRes = grafBindingLayoutSample->Initialize(grafDevice, { grafBindingLayoutSampleDesc });
		if (Failed(grafRes)) return;

		// vertex buffer sample
		grafRes = grafSystem->CreateBuffer(grafVBSample);
		if (Failed(grafRes)) return;
		GrafBuffer::InitParams grafVBSampleParams;
		grafVBSampleParams.BufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::VertexBuffer;
		grafVBSampleParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
		grafVBSampleParams.BufferDesc.SizeInBytes = sizeof(verticesSample);
		grafVBSampleParams.BufferDesc.ElementSize = sizeof(VertexSample);
		grafRes = grafVBSample->Initialize(grafDevice, grafVBSampleParams);
		if (Failed(grafRes)) return;
		grafRes = grafVBSample->Write((ur_byte*)verticesSample);
		if (Failed(grafRes)) return;

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

		// graphics pipeline for sample rendering
		// note: initialized with GrafRenderer's canvas render pass to directly render into swap chain render target(s)
		grafRes = grafSystem->CreatePipeline(grafPipelineSample);
		if (Failed(grafRes)) return;
		GrafShader* sampleShaderStages[] = {
			grafShaderSampleVS.get(),
			grafShaderSamplePS.get()
		};
		GrafDescriptorTableLayout* sampleBindingLayouts[] = {
			grafBindingLayoutSample.get(),
		};
		GrafVertexElementDesc sampleVertexElements[] = {
			{ GrafFormat::R32G32B32_SFLOAT, 0, "POSITION" },
			{ GrafFormat::R32G32B32_SFLOAT, 12, "COLOR" }
		};
		GrafVertexInputDesc sampleVertexInputs[] = {{
			GrafVertexInputType::PerVertex, 0, sizeof(VertexSample),
			sampleVertexElements, ur_array_size(sampleVertexElements)
		}};
		GrafPipeline::InitParams samplePipelineParams = GrafPipeline::InitParams::Default;
		samplePipelineParams.RenderPass = grafRenderer->GetCanvasRenderPass();
		samplePipelineParams.ShaderStages = sampleShaderStages;
		samplePipelineParams.ShaderStageCount = ur_array_size(sampleShaderStages);
		samplePipelineParams.DescriptorTableLayouts = sampleBindingLayouts;
		samplePipelineParams.DescriptorTableLayoutCount = ur_array_size(sampleBindingLayouts);
		samplePipelineParams.VertexInputDesc = sampleVertexInputs;
		samplePipelineParams.VertexInputCount = ur_array_size(sampleVertexInputs);
		grafRes = grafPipelineSample->Initialize(grafDevice, samplePipelineParams);
		if (Failed(grafRes)) return;
	};
	auto initializeGrafFrameObjects = [&]() -> void
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

		// sample shader decriptor tables (per frame & per draw call)
		// tables share the same layout but store unique per frame/call data
		grafBindingSample.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			grafRes = grafSystem->CreateDescriptorTable(grafBindingSample[iframe]);
			if (Failed(grafRes)) break;
			grafRes = grafBindingSample[iframe]->Initialize(grafDevice, { grafBindingLayoutSample.get() });
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;
	};
	auto initializeGrafRenderTargetObjects = [&]() -> void
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
			ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::TransferSrc),
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
		realm.GetLog().WriteLine("DX12SandboxApp: failed to initialize one or more graphics objects", Log::Error);
	}

	// prepare sample image
	if (grafRenderer != ur_null)
	{
		do
		{
			GrafSystem *grafSystem = grafRenderer->GetGrafSystem();
			GrafDevice *grafDevice = grafRenderer->GetGrafDevice();

			// load from file to cpu visible buffer(s)
			struct ImageInitData
			{
				GrafUtils::ImageData imageData;
				std::unique_ptr<GrafCommandList> uploadCmdList;
			};
			std::unique_ptr<ImageInitData> imageInitData(new ImageInitData());
			grafRes = GrafUtils::LoadImageFromFile(*grafDevice, "../Res/testimage.dds", imageInitData->imageData);
			if (Failed(grafRes)) break;
			
			// create image in gpu local memory
			grafRes = grafSystem->CreateImage(grafImageSample);
			if (Failed(grafRes)) break;
			GrafImageDesc grafImageDesc = imageInitData->imageData.Desc;
			grafImageDesc.MipLevels = 1; // TODO: fill all mips
			grafRes = grafImageSample->Initialize(grafDevice, { grafImageDesc });
			if (Failed(grafRes)) break;

			// upload to gpu image
			grafRes = grafSystem->CreateCommandList(imageInitData->uploadCmdList);
			if (Failed(grafRes)) break;
			grafRes = imageInitData->uploadCmdList->Initialize(grafDevice);
			if (Failed(grafRes)) break;

			// record commands performing required image memory transitions and cpu->gpu memory copies
			imageInitData->uploadCmdList->Begin();
			imageInitData->uploadCmdList->ImageMemoryBarrier(grafImageSample.get(), GrafImageState::Current, GrafImageState::TransferDst);
			imageInitData->uploadCmdList->Copy(imageInitData->imageData.MipBuffers[0].get(), grafImageSample.get());
			imageInitData->uploadCmdList->ImageMemoryBarrier(grafImageSample.get(), GrafImageState::Current, GrafImageState::ShaderRead);
			imageInitData->uploadCmdList->End();
			grafDevice->Record(imageInitData->uploadCmdList.get());

			// destroy temporary data & upload command list when finished
			GrafCommandList* uploadCmdListPtr = imageInitData->uploadCmdList.get();
			grafRenderer->AddCommandListCallback(uploadCmdListPtr, { imageInitData.release() }, [](GrafCallbackContext& ctx) -> Result
			{
				ImageInitData* imageInitData = reinterpret_cast<ImageInitData*>(ctx.DataPtr);
				delete imageInitData;
				return Result(Success);
			});
		}
		while (false);
		if (Failed(grafRes))
		{
			grafImageSample.reset();
		}
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
			realm.GetLog().WriteLine("DX12Sandbox: failed to initialize ImguiRender", Log::Error);
		}
	}

	// initialize generic primitives renderer
	GenericRender* genericRender = ur_null;
	if (grafRenderer != ur_null)
	{
		genericRender = realm.AddComponent<GenericRender>(realm);
		Result res = genericRender->Init(grafColorDepthPass.get());
		if (Failed(res))
		{
			realm.RemoveComponent<GenericRender>();
			genericRender = ur_null;
			realm.GetLog().WriteLine("DX12Sandbox: failed to initialize GenericRender", Log::Error);
		}
	}

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	camera.SetPosition(ur_float3(0.0f, 0.0f,-10.0f));
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

	// sample animation
	static const ur_float sampleAnimDelay = 1.0f;
	static const ur_float sampleAnimRadius = 0.1f;
	ur_float sampleAnimElapsedTime = 0;
	ur_float2 sampleAnimPos[4];

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
		if (imguiRender != ur_null && canvasValid)
		{
			imguiRender->NewFrame();
		}
		cameraControl.Update();
		camera.SetAspectRatio((float)realm.GetCanvas()->GetClientBound().Width() / realm.GetCanvas()->GetClientBound().Height());

		// update frame
		auto updateFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {
			
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

			// sample aniamtion
			sampleAnimElapsedTime += elapsedTime;
			for (ur_uint instId = 0; instId < 4; ++instId)
			{
				ur_float animAngle = sampleAnimElapsedTime / (sampleAnimDelay * (instId + 1)) * MathConst<ur_float>::Pi * 2.0f + instId * MathConst<ur_float>::Pi / 8;
				sampleAnimPos[instId].x = cosf(animAngle) * sampleAnimRadius;
				sampleAnimPos[instId].y = sinf(animAngle) * sampleAnimRadius;
			}
			ctx.progress = 2;
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

			auto drawFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {

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
					grafCmdListCrnt->ImageMemoryBarrier(grafRTImageColor.get(), GrafImageState::Current, GrafImageState::ColorWrite);
					grafCmdListCrnt->BeginRenderPass(grafColorDepthPass.get(), grafColorDepthTarget.get(), rtClearValues);

					// immediate mode generic primitives rendering

					const ur_float bbR = 1.0f;
					const ur_float4 testColor = ur_float4(ur_float3(1.0f) - ur_float3(crntClearColor), 1.0f);
					const ur_float3 testPtimitiveVertices[] = { {-bbR,-bbR, 0.0f }, { bbR,-bbR, 0.0f }, { bbR, bbR, 0.0f }, {-bbR, bbR, 0.0f } };
					genericRender->DrawWireBox({ -bbR,-bbR,-bbR }, { bbR, bbR, bbR }, ur_float4(testColor.z, testColor.y, testColor.x, 1.0f));
					genericRender->DrawConvexPolygon(4, testPtimitiveVertices, testColor);
					genericRender->Render(*grafCmdListCrnt, camera.GetViewProj());

					grafCmdListCrnt->EndRenderPass();
					
					// copy RT color result to swap chain image

					grafCmdListCrnt->ImageMemoryBarrier(grafRTImageColor.get(), GrafImageState::Current, GrafImageState::TransferSrc);
					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::TransferDst);
					grafCmdListCrnt->Copy(grafRTImageColor.get(), grafCanvas->GetCurrentImage());
					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::Common);
				}

				{ // foreground color render pass (drawing directly into swap chain image)

					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					grafCmdListCrnt->BeginRenderPass(grafRenderer->GetCanvasRenderPass(), grafRenderer->GetCanvasRenderTarget());

					// draw sample primitives

					updateFrameJob->WaitProgress(2); // wait till animation params are up to date
					SampleCBData sampleCBData;
					sampleCBData.Transform = ur_float4x4::Identity;
					for (ur_uint instId = 0; instId < 4; ++instId)
					{
						sampleCBData.Transform.r[instId].x = sampleAnimPos[instId].x;
						sampleCBData.Transform.r[instId].y = sampleAnimPos[instId].y;
					}
					sampleCBData.Transform.Transpose();
					GrafBuffer* dynamicCB = grafRenderer->GetDynamicConstantBuffer(); // sample CB data changes every frame, use GrafRenderer's dynamic CB
					Allocation dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(SampleCBData));
					dynamicCB->Write((ur_byte*)&sampleCBData, sizeof(sampleCBData), 0, dynamicCBAlloc.Offset);
					grafBindingSample[frameIdx]->SetConstantBuffer(0, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
					grafBindingSample[frameIdx]->SetSampledImage(0, grafImageSample.get(), grafDefaultSampler.get());

					grafCmdListCrnt->BindPipeline(grafPipelineSample.get());
					grafCmdListCrnt->BindDescriptorTable(grafBindingSample[frameIdx].get(), grafPipelineSample.get());
					grafCmdListCrnt->BindVertexBuffer(grafVBSample.get(), 0);
					grafCmdListCrnt->Draw(6, 4, 0, 0);

					// draw GUI
					
					static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
					static bool showGUI = true;
					showGUI = (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
					if (showGUI)
					{
						ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiCond_FirstUseEver);
						ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Once);
						ImGui::ShowMetricsWindow();

						grafRenderer->ShowImgui();

						imguiRender->Render(*grafCmdListCrnt);
					}

					grafCmdListCrnt->EndRenderPass();
					grafCmdListCrnt->ImageMemoryBarrier(grafCanvas->GetCurrentImage(), GrafImageState::Current, GrafImageState::Common);
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

	// destroy sample GRAF objects
	deinitializeGrafObjects();

	// destroy Imgui renderer
	realm.RemoveComponent<ImguiRender>();

	// destroy generic primitives renderer
	realm.RemoveComponent<GenericRender>();

	// destroy GRAF renderer
	realm.RemoveComponent<GrafRenderer>();

	return 0;
}

#else

#include "Gfx/D3D12/GfxSystemD3D12.h"
#include "Gfx/D3D11/GfxSystemD3D11.h" // for test purpose
#include "Resources/Resources.h"

int D3D12SandboxApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"D3D12 Sandbox"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// create gfx system
	//std::unique_ptr<GfxSystemD3D11> gfx(new GfxSystemD3D11(realm)); // for test purpose
	std::unique_ptr<GfxSystemD3D12> gfx(new GfxSystemD3D12(realm));
	Result res = gfx->Initialize(realm.GetCanvas());
	realm.SetGfxSystem(std::move(gfx));

	// create swap chain
	ur_uint canvasWidth = realm.GetCanvas()->GetClientBound().Width();
	ur_uint canvasHeight = realm.GetCanvas()->GetClientBound().Height();
	std::unique_ptr<GfxSwapChain> gfxSwapChain;
	if (Succeeded(realm.GetGfxSystem()->CreateSwapChain(gfxSwapChain)))
	{
		res = gfxSwapChain->Initialize(canvasWidth, canvasHeight,
			false, GfxFormat::R8G8B8A8, true, GfxFormat::R24G8, 3);
	}

	// create graphics context
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

	// initialize test rendering primitive
	std::unique_ptr<GfxVertexShader> gfxVS;
	std::unique_ptr<GfxPixelShader> gfxPS;
	CreateVertexShaderFromFile(realm, gfxVS, "sample_vs.cso");
	CreatePixelShaderFromFile(realm, gfxPS, "sample_ps.cso");

	std::unique_ptr<GfxInputLayout> gfxIL;
	if (Succeeded(realm.GetGfxSystem()->CreateInputLayout(gfxIL)))
	{
		GfxInputElement elements[] = {
			{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 },
			{ GfxSemantic::Color, 0, 0, GfxFormat::R32G32B32A32, GfxFormatView::Float, 0 },
			{ GfxSemantic::TexCoord, 0, 0, GfxFormat::R32G32, GfxFormatView::Float, 0 },
		};
		res = gfxIL->Initialize(*gfxVS.get(), elements, ur_array_size(elements));
	}

	std::unique_ptr<GfxBuffer> gfxVB;
	if (Succeeded(realm.GetGfxSystem()->CreateBuffer(gfxVB)))
	{
		struct Vertex
		{
			ur_float3 pos;
			ur_float4 color;
			ur_float2 tex;
		} bufferData[] = {
			{ { -1.0f, -1.0f,  0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { -1.0f,  1.0f,  0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
			{ {  1.0f, -1.0f,  0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ {  1.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }
		};
		GfxResourceData bufferDataDesc;
		bufferDataDesc.Ptr = bufferData;
		bufferDataDesc.RowPitch = sizeof(bufferData);
		bufferDataDesc.SlicePitch = 0;

		gfxVB->Initialize(sizeof(bufferData), sizeof(Vertex), GfxUsage::Immutable, ur_uint(GfxBindFlag::VertexBuffer), 0, &bufferDataDesc);
	}

	const ur_uint InstanceCount = 24;
	const ur_uint DrawCallCount = 3;
	const ur_uint InstancePerDrawCall = InstanceCount / DrawCallCount;
	struct Constants
	{
		ur_float4x4 Transform[InstanceCount];
		ur_float4 Desc;
	};
	Constants cbData = { ur_float4x4::Identity, { 0.0f, 0.0f, 0.0f, 0.0f } };
	GfxResourceData cbResData = { &cbData, sizeof(Constants) , 0};
	std::unique_ptr<GfxBuffer> gfxCB;
	if (Succeeded(realm.GetGfxSystem()->CreateBuffer(gfxCB)))
	{
		gfxCB->Initialize(sizeof(Constants), 0, GfxUsage::Dynamic, ur_uint(GfxBindFlag::ConstantBuffer), ur_uint(GfxAccessFlag::Write), &cbResData);
	}

	std::string texturesToLoad[DrawCallCount] = {
		"../Res/testimage3.dds",
		"../Res/testimage.dds",
		"../Res/testimage2.dds",
	};
	std::vector<std::unique_ptr<GfxTexture>> gfxTextures;
	for (auto& texturePath : texturesToLoad)
	{
		std::unique_ptr<GfxTexture> texture;
		if (Succeeded(CreateTextureFromFile(realm, texture, texturePath)))
		{
			gfxTextures.emplace_back(std::move(texture));
		}
	}

	std::unique_ptr<GfxSampler> gfxSampler;
	if (Succeeded(realm.GetGfxSystem()->CreateSampler(gfxSampler)))
	{
		GfxSamplerState gfxSamplerState = GfxSamplerState::Default;
		gfxSamplerState.AddressU = GfxTextureAddressMode::Wrap;
		gfxSamplerState.AddressV = GfxTextureAddressMode::Wrap;
		gfxSampler->Initialize(gfxSamplerState);
	}

	GfxRasterizerState gfxRasterizer = GfxRasterizerState::Default;
	gfxRasterizer.CullMode = GfxCullMode::None;

	GfxDepthStencilState gfxDepthStencil = GfxDepthStencilState::Default;
	gfxDepthStencil.DepthEnable = false;
	gfxDepthStencil.DepthWriteEnable = false;

	// declare binding matching used shader registers
	GfxResourceBinding::Layout gfxBindingLayout = {
		{ GfxShaderRegister::ConstantBuffer, 0 },
		{ GfxShaderRegister::ReadBuffer, 0 },
		{ GfxShaderRegister::Sampler, 0 }
	};
	std::unique_ptr<GfxResourceBinding> gfxBinding;
	realm.GetGfxSystem()->CreateResourceBinding(gfxBinding);
	gfxBinding->Initialize(gfxBindingLayout);
	gfxBinding->SetConstantBuffer(0, gfxCB.get());
	gfxBinding->SetTexture(0, gfxTextures.empty() ? ur_null : gfxTextures.begin()->get());
	gfxBinding->SetSampler(0, gfxSampler.get());

	// pipeline state
	std::unique_ptr<GfxPipelineStateObject> gfxPSO;
	realm.GetGfxSystem()->CreatePipelineStateObject(gfxPSO);
	gfxPSO->SetPixelShader(gfxPS.get());
	gfxPSO->SetVertexShader(gfxVS.get());
	gfxPSO->SetResourceBinding(gfxBinding.get());
	gfxPSO->SetInputLayout(gfxIL.get());
	gfxPSO->SetRasterizerState(gfxRasterizer);
	gfxPSO->SetDepthStencilState(gfxDepthStencil);
	gfxPSO->SetPrimitiveTopology(GfxPrimitiveTopology::TriangleStrip);
	gfxPSO->Initialize();

	// animation
	ClockTime timer = Clock::now();
	const ur_float spriteScale = 0.15f;
	const ur_float moveSpeed = 0.5f;
	ur_float2 movePos[InstanceCount];
	ur_float2 moveDir[InstanceCount];
	for (ur_uint i = 0; i < InstanceCount; ++i)
	{
		movePos[i] = { 0.0f, 0.0f };
		moveDir[i].x = (ur_float)rand() / RAND_MAX * 2.0f - 1.0f;
		moveDir[i].y = (ur_float)rand() / RAND_MAX * 2.0f - 1.0f;
		moveDir[i].Normalize();
	}

	// TEST: explicitly synchronized draw calls
#define SYNC_DRAW_CALLS 0
#if (SYNC_DRAW_CALLS)
	auto gfxD3D12 = static_cast<GfxSystemD3D12*>(realm.GetGfxSystem());
	auto d3dDevice = gfxD3D12->GetD3DDevice();
	auto d3dCommandQueue = gfxD3D12->GetD3DCommandQueue();
	shared_ref<ID3D12Fence> d3dDrawCallFence;
	ur_uint drawCallFenceValue = 1;
	d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), d3dDrawCallFence);
	std::unique_ptr<GfxContext> gfxDrawCallContext;
	realm.GetGfxSystem()->CreateContext(gfxDrawCallContext);
	gfxDrawCallContext->Initialize();
#endif

	// Main message loop:
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
			WinInput *winInput = static_cast<WinInput*>(realm.GetInput());
			winInput->ProcessMsg(msg);
		}
		if (msg.message == WM_QUIT)
			break;
		auto timeDelta = Clock::now() - timeBefore;
		timer += timeDelta; // skip input delay

		// update swap chain resolution to match the canvas size
		if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() ||
			canvasHeight != realm.GetCanvas()->GetClientBound().Height())
		{
			canvasWidth = realm.GetCanvas()->GetClientBound().Width();
			canvasHeight = realm.GetCanvas()->GetClientBound().Height();
			gfxSwapChain->Initialize(canvasWidth, canvasHeight,
				false, GfxFormat::R8G8B8A8, true, GfxFormat::R24G8, 3);
		}

		// update sub systems
		realm.GetInput()->Update();
		imguiRender->NewFrame();

		// animate primitives
		auto updateFrameJob = realm.GetJobSystem().Add(ur_null,
			[&](Job::Context& ctx) -> void
		{
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f; // to seconds
			for (ur_uint i = 0; i < InstanceCount; ++i)
			{
				movePos[i] = movePos[i] + moveDir[i] * moveSpeed * elapsedTime;
				if (movePos[i].x > 1.0f || movePos[i].x < -1.0f)
				{
					movePos[i].x = movePos[i].x / abs(movePos[i].x);
					moveDir[i].x *= -1.0f;
					moveDir[i].y = (ur_float)rand() / RAND_MAX * 2.0f - 1.0f + moveDir[i].y;
					moveDir[i].Normalize();
				}
				if (movePos[i].y > 1.0f || movePos[i].y < -1.0f)
				{
					movePos[i].y = movePos[i].y / abs(movePos[i].y);
					moveDir[i].y *= -1.0f;
					moveDir[i].x = (ur_float)rand() / RAND_MAX * 2.0f - 1.0f + moveDir[i].x;
					moveDir[i].Normalize();
				}
				cbData.Transform[i] = ur_float4x4::Identity;
				cbData.Transform[i].Multiply(ur_float4x4::Scaling(spriteScale * (moveDir[i].x < 0.0f ? 1.0f : -1.0f), spriteScale * ur_float(canvasWidth) / canvasHeight, 1.0f));
				cbData.Transform[i].Multiply(ur_float4x4::Translation(movePos[i].x, movePos[i].y, 0.0f));
				ctx.progress = i + 1;
			}
		});

		// draw primitives
		auto drawFrameJob = realm.GetJobSystem().Add(ur_null,
			[&](Job::Context& ctx) -> void
		{
			gfxContext->Begin();

			// prepare RT
			gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer(), ur_null);
			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(), true, { 0.0f, 0.2f, 0.4f, 1.0f }, false, 0.0f, false, 0);
			
			#if (!SYNC_DRAW_CALLS)
			// draw primitives
			gfxContext->SetPipelineStateObject(gfxPSO.get());
			gfxContext->SetResourceBinding(gfxBinding.get());
			gfxContext->SetVertexBuffer(gfxVB.get(), 0);
			for (ur_uint drawCallIdx = 0; drawCallIdx < DrawCallCount; ++drawCallIdx)
			{
				gfxBinding->SetTexture(0, gfxTextures.empty() ? ur_null : gfxTextures[drawCallIdx % gfxTextures.size()].get());
				updateFrameJob->WaitProgress((drawCallIdx + 1) * InstancePerDrawCall); // wait till portion of data required for this draw call is fully updated
				cbData.Desc.x = ur_float(drawCallIdx * InstancePerDrawCall);
				gfxContext->UpdateBuffer(gfxCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, 0);
				gfxContext->Draw(gfxVB->GetDesc().Size / gfxVB->GetDesc().ElementSize, 0, InstancePerDrawCall, 0);
			}
			#endif

			// expose demo gui
			static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
			static bool showGUI = true;
			showGUI = (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
			if (showGUI)
			{
				ImGui::SetNextWindowSize(imguiDemoWndSize, ImGuiSetCond_Once);
				ImGui::SetNextWindowPos({ canvasWidth - imguiDemoWndSize.x, 0.0f }, ImGuiSetCond_Once);
				ImGui::Begin("Control Panel");
				ImGui::Text("Graphics Device:");
				ImGui::Text("	Description: %s", gfxContext->GetGfxSystem().GetActiveAdapterDesc().Description.c_str());
				ImGui::Text("	Dedicated Memory (Mb): %u", gfxContext->GetGfxSystem().GetActiveAdapterDesc().DedicatedVideoMemory / (1 << 20));
				ImGui::Text("	Shared Memory (Mb): %u", gfxContext->GetGfxSystem().GetActiveAdapterDesc().SharedSystemMemory / (1 << 20));
				ImGui::End();

				// Imgui metrics
				ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
				ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
				ImGui::ShowMetricsWindow();
				imguiRender->Render(*gfxContext);
			}

			gfxContext->End();

			#if (SYNC_DRAW_CALLS)
			// TEST: explicitly do synchronized draw calls
			auto WaitDrawCallComplete = [&]() -> void {
				d3dCommandQueue->Signal(d3dDrawCallFence, drawCallFenceValue);
				while (d3dDrawCallFence->GetCompletedValue() < drawCallFenceValue) Sleep(0);
				++drawCallFenceValue;
			};
			for (ur_uint drawCallIdx = 0; drawCallIdx < DrawCallCount; ++drawCallIdx)
			{
				gfxBinding->SetTexture(0, gfxTextures.empty() ? ur_null : gfxTextures[drawCallIdx % gfxTextures.size()].get());
				updateFrameJob->WaitProgress((drawCallIdx + 1) * InstancePerDrawCall); // wait till portion of data required for this draw call is fully updated
				cbData.Desc.x = ur_float(drawCallIdx * InstancePerDrawCall);

				gfxDrawCallContext->Begin();
				gfxDrawCallContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer(), ur_null);
				gfxDrawCallContext->UpdateBuffer(gfxCB.get(), GfxGPUAccess::Write, &cbResData, 0, 0);
				gfxDrawCallContext->SetPipelineStateObject(gfxPSO.get());
				gfxDrawCallContext->SetResourceBinding(gfxBinding.get());
				gfxDrawCallContext->SetVertexBuffer(gfxVB.get(), 0);
				gfxDrawCallContext->Draw(gfxVB->GetDesc().Size / gfxVB->GetDesc().ElementSize, 0, InstancePerDrawCall, 0);
				gfxDrawCallContext->End();
				realm.GetGfxSystem()->Render();
				
				WaitDrawCallComplete();
			}
			#endif
		});
		drawFrameJob->Wait();

		// execute command lists
		realm.GetGfxSystem()->Render();

		// present
		gfxSwapChain->Present();
	}

	// explicitly wait for GPU for now
	GfxSystemD3D12 *gfxSystemD3D12 = dynamic_cast<GfxSystemD3D12*>(realm.GetGfxSystem());
	if (gfxSystemD3D12 != ur_null)
	{
		gfxSystemD3D12->WaitGPU();
	}

	// explicitly release resources
	gfxPSO.reset(ur_null);
	gfxBinding.reset(ur_null);
	gfxVB.reset(ur_null);
	gfxCB.reset(ur_null);
	gfxContext.reset(ur_null);
	gfxSwapChain.reset(ur_null);

	return 0;
}

#endif