// DemoGPUGrass.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "DemoGPUGrass.h"

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
#include "Terrain/Terrain.h"
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
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"GPU Grass Demo"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// create gfx system
	std::unique_ptr<GfxSystemD3D11> gfx(new GfxSystemD3D11(realm));
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

	// initialize ImguiRender
	ImguiRender *imguiRender = realm.AddComponent<ImguiRender>(realm);
	if (imguiRender != ur_null)
	{
		imguiRender = realm.GetComponent<ImguiRender>();
		res = imguiRender->Init();
	}

	// initialize GenericRender
	GenericRender *genericRender = realm.AddComponent<GenericRender>(realm);
	if (genericRender != ur_null)
	{
		genericRender = realm.GetComponent<GenericRender>();
		genericRender->Init();
	}

	// HDR rendering
	HDRRender* hdrRender = realm.AddComponent<HDRRender>(realm);
	if (hdrRender != ur_null)
	{
		hdrRender = realm.GetComponent<HDRRender>();
		HDRRender::Params hdrParams = HDRRender::Params::Default;
		hdrParams.BloomThreshold = 4.0f;
		hdrRender->SetParams(hdrParams);
		hdrRender->Init(canvasWidth, canvasHeight);
	}

	// atmosphere
	ur_float surfaceRadiusMin = 63710.0f;
	std::unique_ptr<Atmosphere> atmosphere(new Atmosphere(realm));
	{
		Atmosphere::Desc desc = Atmosphere::Desc::Default;
		desc.InnerRadius = surfaceRadiusMin;
		desc.OuterRadius = surfaceRadiusMin + 600.0f;
		desc.Kr = 0.000001f;
		desc.Km = 0.000001f;
		desc.ScaleDepth = 0.25f;
		atmosphere->Init(desc);
	}

	// terrain
	//std::unique_ptr<Terrain> terrain(new Terrain(realm));
	//Terrain::InstanceHandle terrainHandle;
	//{
	//	terrain->Init();
	//	terrain->RegisterSubSystem<Terrain::ProceduralData>();
	//	terrain->RegisterSubSystem<Terrain::SimpleGrid>();
	//	
	//	// demo instance
	//	ur_float3 size(1000.0f, 200.0f, 1000.0f);
	//	Terrain::ProceduralData::InstanceDesc dataDesc;
	//	dataDesc.Width = 1024;
	//	dataDesc.Height = 1024;
	//	dataDesc.Seed = 0;
	//	Terrain::SimpleGrid::InstanceDesc presentationDesc;
	//	presentationDesc.Bound.Min = ur_float3(-size.x * 0.5f, surfaceRadiusMin, -size.z);
 //       presentationDesc.Bound.Max = ur_float3(+size.x * 0.5f, surfaceRadiusMin + size.y, +size.z);
	//	terrain->Create<Terrain::ProceduralData, Terrain::SimpleGrid>(terrainHandle, dataDesc, presentationDesc);
	//}

	// demo camera
	Camera camera(realm);
	CameraControl cameraControl(realm, &camera, CameraControl::Mode::Free);
	camera.SetPosition(ur_float3(0.0f, surfaceRadiusMin + 2.0f, 0.0f));
	cameraControl.SetTargetPoint(ur_float3(0.0f));

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

		// update camera control speed depending on the distance to isosurface
		ur_float surfDist = 0;
		cameraControl.SetSpeed(std::max(10.0f, surfDist * 0.5f));

		{ // use context to draw
			gfxContext->Begin();

			// begin HDR rendering
			if (Succeeded(hdrRender->BeginRender(*gfxContext)))
			{
				// draw atmosphere
				atmosphere->Render(*gfxContext, camera.GetViewProj(), camera.GetPosition());

				// end HDR rendering
				hdrRender->EndRender(*gfxContext);

				// atmospheric post effects
				atmosphere->RenderPostEffects(*gfxContext, *hdrRender->GetHDRTarget(), camera.GetViewProj(), camera.GetPosition());

				// resolve HDR image to back buffer
				gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer(), hdrRender->GetHDRTarget());
				hdrRender->Resolve(*gfxContext);

				// render batched generic primitives
				genericRender->Render(*gfxContext, camera.GetViewProj());

				// test surface grid
				{
					static const ur_float3 points[] = {
						{ -1.0f, 0.0f, -1.0f },{ 1.0f, 0.0f, -1.0f },
						{ 1.0f, 0.0f,  1.0f },{ -1.0f, 0.0f,  1.0f },
						{ -1.0f, 0.0f, -1.0f }
					};
					genericRender->DrawPolyline(ur_array_size(points), points, ur_float4(1.0f, 0.0f, 0.0f, 1.0f));
					ur_float4x4 wvp = ur_float4x4::Scaling(100.0f, 1.0f, 100.0f);
					wvp.Multiply(ur_float4x4::Translation(camera.GetPosition().x, surfaceRadiusMin, camera.GetPosition().z));
					wvp.Multiply(camera.GetViewProj());
					genericRender->Render(*gfxContext, wvp);
				}
			}

			// expose demo gui
			static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
			ImGui::SetNextWindowSize(imguiDemoWndSize, ImGuiSetCond_Once);
			ImGui::SetNextWindowPos({ canvasWidth - imguiDemoWndSize.x, 0.0f }, ImGuiSetCond_Once);
			ImGui::Begin("Control Panel");
			ImGui::Text("Gfx Adapter: %S", gfxContext->GetGfxSystem().GetActiveAdapterDesc().Description.c_str());
			cameraControl.ShowImgui();
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
	// ...

	return 0;//(int)msg.wParam;
}