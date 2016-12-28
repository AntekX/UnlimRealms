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

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	camera.SetPosition(ur_float3(0.0f, 0.0f, -10.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

	// test isosurface
	std::unique_ptr<Isosurface> isosurface(new Isosurface(realm));
	{
		Isosurface::AdaptiveVolume::Desc desc;
		desc.Bound = BoundingBox(ur_float3(-4.0f, -4.0f, -4.0f), ur_float3(4.0f, 4.0f, 4.0f));
		desc.BlockSize = 0.125f;
		desc.BlockResolution = 4;// 16;
		desc.DetailLevelDistance = desc.BlockSize.x * 2.0f;
		desc.RefinementProgression = 2.0f;

		/*Isosurface::ProceduralGenerator::FillParams generateParams;
		generateParams.internalValue = +1.0f;
		generateParams.externalValue = -1.0f;
		std::unique_ptr<Isosurface::ProceduralGenerator> loader(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::Fill, generateParams));*/
		Isosurface::ProceduralGenerator::SphericalDistanceFieldParams generateParams;
		generateParams.center = desc.Bound.Center();
		generateParams.radius = desc.Bound.SizeMin() * 0.5f;
		std::unique_ptr<Isosurface::ProceduralGenerator> loader(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::SphericalDistanceField, generateParams));
		/*Isosurface::ProceduralGenerator::SimplexNoiseParams generateParams;
		std::unique_ptr<Isosurface::ProceduralGenerator> loader(new Isosurface::ProceduralGenerator(*isosurface.get(),
			Isosurface::ProceduralGenerator::Algorithm::SimplexNoise, generateParams));*/

		//std::unique_ptr<Isosurface::SurfaceNet> presentation(new Isosurface::SurfaceNet(*isosurface.get()));
		std::unique_ptr<Isosurface::HybridTetrahedra> presentation(new Isosurface::HybridTetrahedra(*isosurface.get()));

		isosurface->Init(desc, std::move(loader), std::move(presentation));
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

		// temp
		static bool freezeRefinement = false;
		if (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F))
			freezeRefinement = !freezeRefinement;
		if (!freezeRefinement)
			isosurface->GetVolume()->RefinementByDistance(camera.GetPosition());
		isosurface->Update();

		// expose demo gui

		ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
		ImGui::Begin("DEMO", ur_null, 0);
		cameraControl.ShowImgui();
		isosurface->ShowImgui();
		ImGui::End();

		{ // use context to draw
			gfxContext->Begin();

			gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer());
			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(),
				true, {0.1f, 0.1f, 0.15f, 1.0f},
				true, 1.0f,
				true, 0);

			// draw isosurface
			isosurface->Render(*gfxContext, camera.GetViewProj());

			// draw generic primitives
			if (genericRender != ur_null)
			{
				// render some demo primitives
				genericRender->DrawLine(ur_float3(0.0f, 0.0f, 0.0f), ur_float3(1.0f, 0.0f, 0.0f), ur_float4(1.0f, 0.0f, 0.0f, 1.0f));
				genericRender->DrawLine(ur_float3(0.0f, 0.0f, 0.0f), ur_float3(0.0f, 1.0f, 0.0f), ur_float4(0.0f, 1.0f, 0.0f, 1.0f));
				genericRender->DrawLine(ur_float3(0.0f, 0.0f, 0.0f), ur_float3(0.0f, 0.0f, 1.0f), ur_float4(0.0f, 0.0f, 1.0f, 1.0f));

				genericRender->Render(*gfxContext, camera.GetViewProj());
			}

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