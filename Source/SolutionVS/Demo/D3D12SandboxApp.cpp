
#include "stdafx.h"
#include "D3D12SandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Gfx/D3D12/GfxSystemD3D12.h"
#include "Gfx/D3D11/GfxSystemD3D11.h" // for test purpose
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

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
		res = gfxSwapChain->Initialize(canvasWidth, canvasHeight);
	}

	// create gfx context
	std::unique_ptr<GfxContext> gfxContext;
	if (Succeeded(realm.GetGfxSystem()->CreateContext(gfxContext)))
	{
		res = gfxContext->Initialize();
	}

	// Main message loop:
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

		// update swap chain resolution to match the canvas size
		if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() ||
			canvasHeight != realm.GetCanvas()->GetClientBound().Height())
		{
			canvasWidth = realm.GetCanvas()->GetClientBound().Width();
			canvasHeight = realm.GetCanvas()->GetClientBound().Height();
			gfxSwapChain->Initialize(canvasWidth, canvasHeight);
		}

		// update sub systems
		realm.GetInput()->Update();

		{ // use context to draw
			gfxContext->Begin();

			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(), true, ur_float4(0.0f, 0.0f, 1.0f, 1.0f), false, 0.0f, false, 0);

			gfxContext->End();
		}

		// execute command lists
		realm.GetGfxSystem()->Render();

		// present
		gfxSwapChain->Present();
	}

	// explicitly
	gfxSwapChain.reset(ur_null);

	return 0;
}