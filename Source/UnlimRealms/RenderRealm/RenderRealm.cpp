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
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"

namespace UnlimRealms
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const RenderRealm::InitParams RenderRealm::InitParams::Default = {
		L"UnlimRealmsRenderRealm",
		Canvas::Style::OverlappedWindowMaximized,
		GrafSystemType::DX12,
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
			LogError("RenderRealm::Initialize: failed to initlize ghraphics");
			return res;
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

		return Result(Success);
	}

	Result RenderRealm::DeintializeGrafRenderer()
	{
		// release graphic objects
		this->DeinitializeGraphicObjects();

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
		ur_float canvasResolutionScale = 1.0f;
		ur_uint canvasWidth = ur_uint(this->GetCanvas()->GetClientBound().Width() * canvasResolutionScale);
		ur_uint canvasHeight = ur_uint(this->GetCanvas()->GetClientBound().Height() * canvasResolutionScale);
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

				// update render target(s)
				ur_uint canvasWidthActual = ur_uint(this->GetCanvas()->GetClientBound().Width() * canvasResolutionScale);
				ur_uint canvasHeightActual = ur_uint(this->GetCanvas()->GetClientBound().Height() * canvasResolutionScale);
				if (canvasValid && (canvasWidth != canvasWidthActual || canvasHeight != canvasHeightActual))
				{
					canvasWidth = canvasWidthActual;
					canvasHeight = canvasHeightActual;
					// use prev frame command list to make sure frame buffer objects are destroyed only when it is no longer used
					//DestroyFrameBufferObjects(managedCommandList->GetFrameObject(this->GetGrafRenderer()->GetPrevFrameId()));
					// recreate frame buffer objects for new canvas dimensions
					//InitFrameBufferObjects(canvasWidth, canvasHeight);
				}

				// perfrom rendering
				renderContext.CommandList = this->GetGrafRenderer()->GetTransientCommandList();
				renderContext.CommandList->Begin();
				this->Render(renderContext);
				renderContext.CommandList->End();
				this->GetGrafRenderer()->GetGrafDevice()->Record(renderContext.CommandList);

				// finalize & move to next frame
				this->GetGrafRenderer()->EndFrameAndPresent();
			}
		}

		return Result(Success);
	}

} // end namespace UnlimRealms