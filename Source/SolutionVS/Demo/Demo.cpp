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
#include "Resources/Resources.h"
#include "Camera/CameraControl.h"
#include "HDRRender/HDRRender.h"
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
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"Voxel Planet Demo"));
	canvas->Initialize( RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)) );
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

	// HDR rendering
	HDRRender* hdrRender = ur_null;
	if (realm.AddComponent<HDRRender>(realm))
	{
		hdrRender = realm.GetComponent<HDRRender>();
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.BloomThreshold = 4.0f;
		hdrRender->SetParams(hdrParams);
		hdrRender->Init(canvasWidth, canvasHeight);
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
			{ 1.100f, 8.0f, -0.25f, 0.4f },
			{ 0.345f, 32.0f, -0.25f, 0.1f },
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

		//dataVolume->Save("test.isd", desc.CellSize, desc.LatticeResolution);

		isosurface->Init(std::move(dataVolume), std::move(presentation));
	}

	// demo atmosphere
	std::unique_ptr<Atmosphere> atmosphere(new Atmosphere(realm));
	{
		Atmosphere::Desc desc = Atmosphere::Desc::Default;
		desc.InnerRadius = lerp(surfaceRadiusMin, surfaceRadiusMax, 0.75f);
		desc.OuterRadius = surfaceRadiusMin * 1.2f;
		desc.Kr = 0.00005f;
		desc.Km = 0.00005f;
		desc.ScaleDepth = 0.16f;
		atmosphere->Init(desc);
	}

	// demo moon
	ur_float moonRadiusMin = 200.0f;
	ur_float moonRadiusMax = 220.0f;
	std::unique_ptr<Isosurface> moon(new Isosurface(realm));
	{
		ur_float r = moonRadiusMax;
		ur_float3 p = ur_float3(+1000, 1500.0f, 1500.0f);
		BoundingBox volumeBound(ur_float3(-r, -r, -r) + p, ur_float3(r, r, r) + p);
		Isosurface::ProceduralGenerator::SimplexNoiseParams generateParams;
		generateParams.bound = volumeBound;
		generateParams.radiusMin = moonRadiusMin;
		generateParams.radiusMax = moonRadiusMax;
		generateParams.octaves.assign({
			{ 1.000f, 4.0f, -0.0f, 1.0f },
			{ 0.400f, 16.0f, -1.0f, 0.2f },
			{ 0.035f, 128.0f, -1.0f, 0.2f },
		});
		std::unique_ptr<Isosurface::ProceduralGenerator> dataVolume(new Isosurface::ProceduralGenerator(*moon.get(),
			Isosurface::ProceduralGenerator::Algorithm::SimplexNoise, generateParams));

		Isosurface::HybridCubes::Desc desc;
		desc.CellSize = 2.0f;
		desc.LatticeResolution = 10;
		desc.DetailLevelDistance = desc.CellSize * desc.LatticeResolution.x * 1.0f;
		std::unique_ptr<Isosurface::HybridCubes> presentation(new Isosurface::HybridCubes(*moon.get(), desc));

		moon->Init(std::move(dataVolume), std::move(presentation));
	}

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::AroundPoint);
	camera.SetPosition(ur_float3(0.0f, 0.0f, -surfaceRadiusMax * 3.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

	// multiverse
	//Multiverse *multiverse = ur_null;
	//Space *space = ur_null;
	//if (realm.AddComponent<Multiverse>(realm))
	//{
	//	multiverse = realm.GetComponent<Multiverse>();
	//	// TODO: add a space here
	//}

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

		// update canvas
		if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() ||
			canvasHeight != realm.GetCanvas()->GetClientBound().Height())
		{
			canvasWidth = realm.GetCanvas()->GetClientBound().Width();
			canvasHeight = realm.GetCanvas()->GetClientBound().Height();
			gfxSwapChain->Initialize(canvasWidth, canvasHeight);
			hdrRender->Init(canvasWidth, canvasHeight);
		}

		// update sub systems
		realm.GetInput()->Update();
		imguiRender->NewFrame();
		cameraControl.Update();
		camera.SetAspectRatio((float)canvasWidth / canvasHeight);

		// update isosurface
		isosurface->GetPresentation()->Update(camera.GetPosition(), camera.GetViewProj());
		isosurface->Update();
		moon->GetPresentation()->Update(camera.GetPosition(), camera.GetViewProj());
		
		// update camera control speed depending on the distance to isosurface
		ur_float surfDist = std::min(
			(camera.GetPosition() - isosurface->GetData()->GetBound().Center()).Length() - surfaceRadiusMin,
			(camera.GetPosition() - moon->GetData()->GetBound().Center()).Length() - moonRadiusMin);
		cameraControl.SetSpeed(std::max(0.1f, surfDist * 0.5f));

		{ // use context to draw
			gfxContext->Begin();

			// begin HDR rendering
			hdrRender->BeginRender(*gfxContext);

			// draw isosurface
			moon->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition(), ur_null);
			isosurface->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition(), atmosphere.get());
			atmosphere->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition());

			// end HDR rendering
			hdrRender->EndRender(*gfxContext);

			// resolve HDR image to back buffer
			gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer());
			hdrRender->Resolve(*gfxContext, camera.GetViewProj());

			// render batched generic primitives
			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(),
				false, 0.0f, true, 1.0f, true, 0);
			genericRender->Render(*gfxContext, camera.GetViewProj());

			// expose demo gui
			static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
			ImGui::SetNextWindowSize(imguiDemoWndSize, ImGuiSetCond_Once);
			ImGui::SetNextWindowPos({ canvasWidth - imguiDemoWndSize.x, 0.0f }, ImGuiSetCond_Once);
			ImGui::Begin("Control Panel");
			ImGui::Text("Gfx Adapter: %S", gfxContext->GetGfxSystem().GetActiveAdapterDesc().Description.c_str());
			cameraControl.ShowImgui();
			isosurface->ShowImgui();
			atmosphere->ShowImgui();
			hdrRender->ShowImgui();
			ImGui::End();

			// Imgui metrics
			ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
			ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
			ImGui::ShowMetricsWindow();
			imguiRender->Render(*gfxContext);

			gfxContext->End();
		}

		realm.GetGfxSystem()->Render();

		gfxSwapChain->Present();
    }
	realm.GetLog().WriteLine("Left main message loop");

	// deinitialize explicitly before realm instance destroyed
	isosurface.reset(ur_null);
	moon.reset(ur_null);

	return 0;//(int)msg.wParam;
}