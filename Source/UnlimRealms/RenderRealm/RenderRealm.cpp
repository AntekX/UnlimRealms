///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "RenderRealm.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Graf/GrafRenderer.h"
#include "Graf/DX12/GrafSystemDX12.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"

namespace UnlimRealms
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafClearValue DefaultClearColor = { 0.08f, 0.08f, 0.16f, 0.0f };

	static const ur_float4 DebugLabelColorForeground = { 0.8f, 0.8f, 1.0f, 1.0f };
	static const ur_float4 DebugLabelColorImGui = { 0.8f, 0.8f, 1.0f, 1.0f };

	const RenderRealm::InitParams RenderRealm::InitParams::Default = {
		L"UnlimRealmsRenderRealm",
		Canvas::Style::OverlappedWindowMaximized,
		GrafSystemType::DX12,
		true, // ImguiEnabled
		GrafRenderer::InitParams::Default,
	};

	RenderRealm::RenderRealm() :
		state(State::Initialize),
		grafRenderer(ur_null)
	{
	}

	RenderRealm::~RenderRealm()
	{
	}

	Result RenderRealm::Initialize(const InitParams& initParams)
	{
		// initialize base realm components
		Result res = Realm::Initialize();
		if (Failed(res))
		{
			LogError("RenderRealm::Initialize: failed to initialize realm");
			return res;
		}

		// create system canvas
		std::unique_ptr<Canvas> canvas;
		res = this->CreateSystemCanvas(canvas, initParams.CanvasStyle, initParams.Title);
		if (Succeeded(res))
		{
			res = canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
		}
		if (Failed(res))
		{
			this->Deinitialize();
			LogError("RenderRealm::Initialize: failed to initlize system canvas");
			return res;
		}
		this->SetCanvas(std::move(canvas));

		// create input system
		std::unique_ptr<Input> input;
		res = this->CreateSystemInput(input);
		if (Succeeded(res))
		{
			res = input->Initialize();
		}
		if (Failed(res))
		{
			this->Deinitialize();
			LogError("RenderRealm::Initialize: failed to initlize system input");
			return res;
		}
		this->SetInput(std::move(input));

		// initialize graphics
		this->InitializeGrafRenderer(initParams.GrafSystemType, initParams.RendererParams);
		if (Failed(res))
		{
			this->Deinitialize();
			LogError("RenderRealm::Initialize: failed to initlize graphics");
			return res;
		}

		// initialize ImGui renderer
		if (initParams.ImguiEnabled)
		{
			this->imguiRender = this->AddComponent<ImguiRender>(*this);
			Result res = this->imguiRender->Init();
			if (Failed(res))
			{
				this->Deinitialize();
				LogError("RenderRealm::Initialize: failed to initlize ImguiRender");
				return res;
			}
		}

		return Result(Success);
	}

	Result RenderRealm::Initialize()
	{
		return Initialize(RenderRealm::InitParams::Default);
	}

	Result RenderRealm::Deinitialize()
	{
		// release graphic objects
		DeintializeGrafRenderer();
		
		return Realm::Deinitialize();
	}

	Result RenderRealm::InitializeGrafRenderer(GrafSystemType grafSystemType, const GrafRenderer::InitParams& rendererParams)
	{
		this->DeintializeGrafRenderer();

		// create GRAF system
		std::unique_ptr<GrafSystem> grafSystem;
		switch (grafSystemType)
		{
		case GrafSystemType::DX12:
			grafSystem.reset(new GrafSystemDX12(*this));
			break;
		case GrafSystemType::Vulkan:
			grafSystem.reset(new GrafSystemVulkan(*this));
			break;
		default:
			return Result(NotImplemented);
		};
		Result res = grafSystem->Initialize(this->GetCanvas());
		if (Failed(res))
		{
			LogError("RenderRealm::InitializeGrafRenderer: failed to initlize GRAF system");
			return res;
		}

		// create renderer
		this->grafRenderer = this->AddComponent<GrafRenderer>(*this);
		res = this->grafRenderer->Initialize(std::move(grafSystem), rendererParams);
		if (Failed(res))
		{
			this->RemoveComponent<GrafRenderer>();
			this->grafRenderer = ur_null;
			LogError("RenderRealm::InitializeGrafRenderer: failed to initlize GRAF renderer");
			return res;
		}

		// init graphic objects
		this->InitializeGraphicObjects();

		// init canvas graphics objects
		this->InitializeCanvasObjects();

		return Result(Success);
	}

	Result RenderRealm::DeintializeGrafRenderer()
	{
		// release graphic objects
		this->DeinitializeGraphicObjects();

		// destroy Imgui renderer
		if (this->imguiRender)
		{
			this->RemoveComponent<ImguiRender>();
			this->imguiRender = ur_null;
		}

		// destroy renderer
		if (this->grafRenderer)
		{
			this->grafRenderer->Deinitialize();
			this->RemoveComponent<GrafRenderer>();
			this->grafRenderer = ur_null;
		}

		return Result(Success);
	}

	Result RenderRealm::Run()
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::Update(const UpdateContext& updateContext)
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::Render(const RenderContext& renderContext)
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::DisplayImgui()
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::CreateSystemCanvas(std::unique_ptr<Canvas>& canvas, Canvas::Style style, const wchar_t* title)
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::CreateSystemInput(std::unique_ptr<Input>& input)
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::InitializeGraphicObjects()
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::DeinitializeGraphicObjects()
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::InitializeCanvasObjects()
	{
		return Result(NotImplemented);
	}

	Result RenderRealm::SafeDeleteCanvasObjects(GrafCommandList* commandList)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	WinRenderRealm::WinRenderRealm()
	{
	}

	WinRenderRealm::~WinRenderRealm()
	{
	}

	Result WinRenderRealm::CreateSystemCanvas(std::unique_ptr<Canvas>& canvas, Canvas::Style style, const wchar_t* title)
	{
		canvas.reset(new WinCanvas(*this, style, title));
		return Result(Success);
	}

	Result WinRenderRealm::CreateSystemInput(std::unique_ptr<Input>& input)
	{
		input.reset(new WinInput(*this));
		return Result(Success);
	}

	Result WinRenderRealm::Run()
	{
		ur_uint canvasWidth = this->GetCanvasWidth();
		ur_uint canvasHeight = this->GetCanvasHeight();
		ur_bool canvasValid = false;
		ClockTime runStartTime = Clock::now();
		ClockTime lastUpdateTime = runStartTime;
		UpdateContext updateContext = {};
		RenderContext renderContext = {};

		// Main loop
		this->SetState(State::Run);
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (State::Run == this->GetState())
		{
			// process messages & input

			ClockTime timeBefore = Clock::now();
			while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) && msg.message != WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				// forward msg to WinInput system
				WinInput* winInput = static_cast<WinInput*>(this->GetInput());
				winInput->ProcessMsg(msg);
			}
			if (msg.message == WM_QUIT || this->GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::Escape))
			{
				this->SetState(State::Finish);
				break;
			}
			auto inputDelay = Clock::now() - timeBefore;
			lastUpdateTime += inputDelay; // skip input delay

			canvasValid = (this->GetCanvas()->GetClientBound().Area() > 0);
			if (!canvasValid)
				Sleep(60); // lower update frequency while minimized

			// update

			// input
			this->GetInput()->Update();

			// update timer
			ClockTime timeNow = Clock::now();
			updateContext.CurrentRunTime = ClockDeltaAs<std::chrono::milliseconds>(timeNow - runStartTime).count();
			updateContext.ElapsedTime = ClockDeltaAs<std::chrono::milliseconds>(timeNow - lastUpdateTime).count();
			lastUpdateTime = timeNow;

			this->Update(updateContext);

			// render

			if (this->GetGrafRenderer() != ur_null)
			{
				// begin new frame
				this->GetGrafRenderer()->BeginFrame();

				// perfrom rendering
				renderContext.CommandList = this->GetGrafRenderer()->GetTransientCommandList();
				renderContext.CommandList->Begin();

				// update canvas render target(s)
				ur_uint canvasWidthActual = this->GetCanvasWidth();
				ur_uint canvasHeightActual = this->GetCanvasHeight();
				if (canvasValid && (canvasWidth != canvasWidthActual || canvasHeight != canvasHeightActual))
				{
					canvasWidth = canvasWidthActual;
					canvasHeight = canvasHeightActual;
					SafeDeleteCanvasObjects(renderContext.CommandList);
					InitializeCanvasObjects();
				}

				// clear swap chain current image
				GrafClearValue rtClearValue = DefaultClearColor;
				renderContext.CommandList->ImageMemoryBarrier(this->GetGrafRenderer()->GetGrafCanvas()->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorClear);
				renderContext.CommandList->ClearColorImage(this->GetGrafRenderer()->GetGrafCanvas()->GetCurrentImage(), rtClearValue);

				// main render pass
				{
					GrafViewportDesc grafViewport = {};
					grafViewport.Width = (ur_float)canvasWidth;
					grafViewport.Height = (ur_float)canvasHeight;
					grafViewport.Near = 0.0f;
					grafViewport.Far = 1.0f;
					renderContext.CommandList->SetViewport(grafViewport, true);

					this->Render(renderContext);
				}

				// foreground pass (drawing directly into renderer's swap chain image)
				{
					GrafUtils::ScopedDebugLabel label(renderContext.CommandList, "Foreground", DebugLabelColorForeground);
					renderContext.CommandList->ImageMemoryBarrier(this->GetGrafRenderer()->GetGrafCanvas()->GetCurrentImage(), GrafImageState::Current, GrafImageState::ColorWrite);
					renderContext.CommandList->BeginRenderPass(this->GetGrafRenderer()->GetCanvasRenderPass(), this->GetGrafRenderer()->GetCanvasRenderTarget());

					GrafViewportDesc grafViewport = {};
					grafViewport.Width = (ur_float)canvasWidth;
					grafViewport.Height = (ur_float)canvasHeight;
					grafViewport.Near = 0.0f;
					grafViewport.Far = 1.0f;
					renderContext.CommandList->SetViewport(grafViewport, true);

					// ImGui
					static bool showGUI = true;
					showGUI = (this->GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
					if (this->GetImguiRenderer() && showGUI)
					{
						GrafUtils::ScopedDebugLabel label(renderContext.CommandList, "ImGui", DebugLabelColorImGui);
						this->GetImguiRenderer()->NewFrame();

						ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiCond_FirstUseEver);
						ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Once);
						ImGui::ShowMetricsWindow();

						this->GetGrafRenderer()->DisplayImgui();
						this->DisplayImgui();

						this->GetImguiRenderer()->Render(*renderContext.CommandList);
					}

					renderContext.CommandList->EndRenderPass();
				}

				renderContext.CommandList->End();
				this->GetGrafRenderer()->GetGrafDevice()->Record(renderContext.CommandList);

				// finalize & move to next frame
				this->GetGrafRenderer()->EndFrameAndPresent();
			}
		}

		return Result(Success);
	}

} // end namespace UnlimRealms