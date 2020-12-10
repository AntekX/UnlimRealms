
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
#define RENDER_ASYNC 1

int HybridRenderingApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"RayTracing Sandbox"));
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
	struct RenderTargetObjects
	{
		enum ImageUsage
		{
			ImageUsage_Depth = 0,
			ImageUsage_Normal,
			ImageCount
		};

		GrafRenderer* grafRenderer;
		std::unique_ptr<GrafImage> images[ImageCount];
		std::vector<std::unique_ptr<GrafRenderTarget>> targetPerFrame;
		
		Result Initialize()
		{
			Result res(Success);

			// TODO

			return res;
		}

		void Deinitialize()
		{
			targetPerFrame.clear();
		}

		RenderTargetObjects(GrafRenderer* grafRenderer) : grafRenderer(grafRenderer) {}
		~RenderTargetObjects() { Deinitialize(); }
	};
	struct GrafObjects
	{
		GrafRenderer* grafRenderer;
		std::vector<std::unique_ptr<GrafCommandList>> cmdListPerFrame;
		std::unique_ptr<RenderTargetObjects> renderTargetObjects;
		std::unique_ptr<GrafRenderPass> rasterRenderPass;

		Result Initialize()
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

			return res;
		}

		void Deinitialize()
		{
			cmdListPerFrame.clear();
		}

		GrafObjects(GrafRenderer* grafRenderer) : grafRenderer(grafRenderer) {}
		~GrafObjects() { Deinitialize(); }
	};
	GrafObjects grafObjects(grafRenderer);
	grafObjects.Initialize();
	// TODO: initialize all here

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

	// setup main camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	cameraControl.SetTargetPoint(ur_float3(0.0f));
	cameraControl.SetSpeed(4.0);
	camera.SetProjection(0.1f, 1.0e+4f, camera.GetFieldOFView(), camera.GetAspectRatio());
	camera.SetPosition(ur_float3(9.541f, 5.412f, -12.604f));
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

			ctx.progress = 1;

			// upadte scene here
			// TODO
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
			GrafDevice *grafDevice = grafRenderer->GetGrafDevice();
			GrafCanvas *grafCanvas = grafRenderer->GetGrafCanvas();
			GrafCommandList* grafCmdListCrnt = ur_null;// grafMainCmdList[grafRenderer->GetCurrentFrameId()].get();
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
					if (ImGui::CollapsingHeader("Canvas"))
					{
						int editableInt = canvasDenom;
						ImGui::InputInt("ResolutionDenominator", &editableInt);
						canvasDenom = editableInt;
					}

					imguiRender->Render(*grafCmdListCrnt);
				}

				grafCmdListCrnt->EndRenderPass();
			}

			// finalize current command list
			grafCmdListCrnt->End();
			grafDevice->Record(grafCmdListCrnt);

		}; // end render func
		
		if (grafRenderer != ur_null)
		{
			// begin frame rendering
			grafRenderer->BeginFrame();

			// resize render target(s)
			ur_uint canvasWidthNew = realm.GetCanvas()->GetClientBound().Width() / canvasDenom;
			ur_uint canvasHeightNew = realm.GetCanvas()->GetClientBound().Height() / canvasDenom;
			if (canvasValid && (canvasWidth != canvasWidthNew || canvasHeight != canvasHeightNew))
			{
				canvasWidth = canvasWidthNew;
				canvasHeight = canvasHeightNew;
				// TODO
				// use prev frame command list to make sure RT objects are destroyed only when it is no longer used
				//deinitializeGrafRenderTargetObjects(grafMainCmdList[grafRenderer->GetPrevFrameId()].get());
				// recreate RT objects for new canvas dimensions
				//initializeGrafRenderTargetObjects();
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
	// TODO

	// destroy GRAF objects
	//deinitializeGrafObjects();

	// destroy realm objects
	realm.RemoveComponent<ImguiRender>();
	realm.RemoveComponent<GenericRender>();
	realm.RemoveComponent<GrafRenderer>();

	return 0;
}