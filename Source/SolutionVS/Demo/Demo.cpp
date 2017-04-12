// Demo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Demo.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Gfx/D3D11/GfxSystemD3D11.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"
#include "Isosurface/Isosurface.h"
#include "Atmosphere/Atmosphere.h"
#include "Multiverse/Multiverse.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	ur_uint screenWidth = (ur_uint)GetSystemMetrics(SM_CXSCREEN);
	ur_uint screenHeight = (ur_uint)GetSystemMetrics(SM_CYSCREEN);
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm));
	canvas->Initialize( RectI(0, 0, screenWidth, screenHeight) );
	realm.SetCanvas( std::move(canvas) );

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput( std::move(input) );

	// create gfx system
	std::unique_ptr<GfxSystemD3D11> gfx(new GfxSystemD3D11(realm));
	Result res = gfx->Initialize( realm.GetCanvas() );
	realm.SetGfxSystem( std::move(gfx) );

	// create swap chain
	std::unique_ptr<GfxSwapChain> gfxSwapChain;
	if (Succeeded(realm.GetGfxSystem()->CreateSwapChain(gfxSwapChain)))
	{
		res = gfxSwapChain->Initialize(screenWidth, screenHeight);
	}

	// create gfx context
	std::unique_ptr<GfxContext> gfxContext;
	if (Succeeded(realm.GetGfxSystem()->CreateContext(gfxContext)))
	{
		res = gfxContext->Initialize();
	}

	// initialize ImguiRender
	ImguiRender *imguiRender = ur_null;
	if (realm.AddComponent<ImguiRender>(realm))
	{
		imguiRender = realm.GetComponent<ImguiRender>();
		res = imguiRender->Init();
	}

	// initialize GenericRender
	GenericRender *genericRender = ur_null;
	if (realm.AddComponent<GenericRender>(realm))
	{
		genericRender = realm.GetComponent<GenericRender>();
		genericRender->Init();
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
		/*generateParams.octaves.assign({
			{ 0.875f, 7.5f, -1.0f, 0.5f },
			{ 0.345f, 30.0f, -0.5f, 0.1f },
			{ 0.035f, 120.0f, -1.0f, 0.2f },
		});*/
		// canyons
		generateParams.octaves.assign({
			{ 1.100f, 8.0f, -1.00f, 0.4f },
			{ 0.345f, 32.0f, -0.5f, 0.1f },
			{ 0.035f, 128.0f, -1.0f, 0.2f },
		});

		std::unique_ptr<Isosurface::ProceduralGenerator> dataVolume(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::SimplexNoise, generateParams));
#endif

		Isosurface::HybridCubes::Desc desc;
		desc.CellSize = 2.0f;
		desc.LatticeResolution = 10;
		desc.DetailLevelDistance = desc.CellSize * desc.LatticeResolution.x * 1.0f;
		std::unique_ptr<Isosurface::HybridCubes> presentation(new Isosurface::HybridCubes(*isosurface.get(), desc));

		isosurface->Init(std::move(dataVolume), std::move(presentation));
	}

	// demo atmosphere
	std::unique_ptr<Atmosphere> atmosphere(new Atmosphere(realm));
	{
		Atmosphere::Desc desc = Atmosphere::Desc::Default;
		desc.InnerRadius = surfaceRadiusMin;
		desc.OuterRadius = surfaceRadiusMin + (surfaceRadiusMax - surfaceRadiusMin) * 3.0f;
		atmosphere->Init(desc);
	}

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	camera.SetPosition(ur_float3(0.0f, 0.0f, -surfaceRadiusMax * 3.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

	// multiverse
	Multiverse *multiverse = ur_null;
	if (realm.AddComponent<Multiverse>(realm))
	{
		multiverse = realm.GetComponent<Multiverse>();
		// TODO: add a space here
	}

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

		// update sub systems

		realm.GetInput()->Update();
		imguiRender->NewFrame();
		cameraControl.Update();
		camera.SetAspectRatio((float)realm.GetCanvas()->GetBound().Width() / realm.GetCanvas()->GetBound().Height());

		// update isosurface

		isosurface->GetPresentation()->Update(camera.GetPosition(), camera.GetViewProj());
		isosurface->Update();
		
		// update camera control speed depending on the distance to isosurface
		ur_float surfDist = (camera.GetPosition() - isosurface->GetData()->GetBound().Center()).Length();
		cameraControl.SetSpeed(std::max(0.1f, (surfDist - surfaceRadiusMin) * 0.5f));

		{ // use context to draw
			gfxContext->Begin();

			gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer());
			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(),
				true, /*{0.1f, 0.1f, 0.15f, 1.0f}*/{0.0f, 0.0f, 0.0f, 1.0f},
				true, 1.0f,
				true, 0);

			// draw isosurface
			isosurface->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition(), atmosphere.get());
			atmosphere->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition());

			// draw generic primitives
			if (genericRender != ur_null)
			{
				// render some demo primitives
				genericRender->DrawLine(ur_float3(0.0f, 0.0f, 0.0f), ur_float3(1.0f, 0.0f, 0.0f), ur_float4(1.0f, 0.0f, 0.0f, 1.0f));
				genericRender->DrawLine(ur_float3(0.0f, 0.0f, 0.0f), ur_float3(0.0f, 1.0f, 0.0f), ur_float4(0.0f, 1.0f, 0.0f, 1.0f));
				genericRender->DrawLine(ur_float3(0.0f, 0.0f, 0.0f), ur_float3(0.0f, 0.0f, 1.0f), ur_float4(0.0f, 0.0f, 1.0f, 1.0f));

				genericRender->Render(*gfxContext, camera.GetViewProj());
			}

			// expose demo gui
			ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
			ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
			ImGui::Begin("DEMO", ur_null, 0);
			ImGui::Text("Gfx Adapter: %S", gfxContext->GetGfxSystem().GetActiveAdapterDesc().description.c_str());
			cameraControl.ShowImgui();
			isosurface->ShowImgui();
			ImGui::End();

			// render some demo UI
			ImGui::ShowMetricsWindow();
			imguiRender->Render(*gfxContext);

			gfxContext->End();
		}

		realm.GetGfxSystem()->Render();

		gfxSwapChain->Present();
    }
	realm.GetLog().WriteLine("Left main message loop");

	return 0;//(int)msg.wParam;
}