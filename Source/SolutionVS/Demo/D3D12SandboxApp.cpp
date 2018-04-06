
#include "stdafx.h"
#include "D3D12SandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Gfx/D3D12/GfxSystemD3D12.h"
#include "Gfx/D3D11/GfxSystemD3D11.h" // for test purpose
#include "Resources/Resources.h"
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
		res = gfxSwapChain->Initialize(canvasWidth, canvasHeight,
			false, GfxFormat::R8G8B8A8, true, GfxFormat::R24G8, 2);
	}

	// create graphics context
	std::unique_ptr<GfxContext> gfxContext;
	if (Succeeded(realm.GetGfxSystem()->CreateContext(gfxContext)))
	{
		res = gfxContext->Initialize();
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

	struct Vertex
	{
		ur_float3 pos;
		ur_float4 color;
		ur_float2 tex;
	};
	std::unique_ptr<GfxBuffer> gfxVB;
	if (Succeeded(realm.GetGfxSystem()->CreateBuffer(gfxVB)))
	{
		Vertex bufferData[] = {
			{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
			{ {  0.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }
		};

		GfxResourceData bufferDataDesc;
		bufferDataDesc.Ptr = bufferData;
		bufferDataDesc.RowPitch = sizeof(bufferData);
		bufferDataDesc.SlicePitch = 0;

		GfxBufferDesc bufferDesc = {};
		bufferDesc.Size = bufferDataDesc.RowPitch;
		bufferDesc.Usage = GfxUsage::Dynamic;
		bufferDesc.BindFlags = ur_uint(GfxBindFlag::VertexBuffer);

		gfxVB->Initialize(bufferDesc, &bufferDataDesc);
	}

	GfxRasterizerState gfxRasterizer = GfxRasterizerState::Default;
	gfxRasterizer.CullMode = GfxCullMode::None;

	GfxDepthStencilState gfxDepthStencil = GfxDepthStencilState::Default;
	gfxDepthStencil.DepthEnable = false;
	gfxDepthStencil.DepthWriteEnable = false;

	// declare binding matching used shader registers
	std::unique_ptr<GfxResourceBinding> gfxBinding;
	realm.GetGfxSystem()->CreateResourceBinding(gfxBinding);
	//gfxBinding->SetBuffer(0, ur_null); 
	//gfxBinding->SetTexture(0, ur_null);
	//gfxBinding->SetSampler(0, ur_null);
	gfxBinding->Initialize();

	std::unique_ptr<GfxPipelineStateObject> gfxPSO;
	realm.GetGfxSystem()->CreatePipelineStateObject(gfxPSO);
	gfxPSO->SetResourceBinding(gfxBinding.get());
	gfxPSO->SetPixelShader(gfxPS.get());
	gfxPSO->SetVertexShader(gfxVS.get());
	gfxPSO->SetInputLayout(gfxIL.get());
	gfxPSO->SetRasterizerState(gfxRasterizer);
	gfxPSO->SetDepthStencilState(gfxDepthStencil);
	gfxPSO->Initialize();

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

			static const ur_float4 s_colors[] = {
				{ 0.0f, 0.2f, 0.4f, 1.0f },
				{ 1.0f, 0.0f, 0.0f, 1.0f },
				{ 0.0f, 1.0f, 0.0f, 1.0f },
				{ 0.0f, 0.0f, 1.0f, 1.0f },
				{ 1.0f, 1.0f, 0.0f, 1.0f },
				{ 1.0f, 0.0f, 1.0f, 1.0f },
			};
			ur_uint colorIdx = 0;// static_cast<GfxSystemD3D12*>(realm.GetGfxSystem())->CurrentFrameIndex() % 6;

			gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer(), ur_null);
			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(), true, s_colors[colorIdx], false, 0.0f, false, 0);
			
			// draw test primitive
			gfxContext->SetPipelineStateObject(gfxPSO.get());
			gfxContext->SetResourceBinding(gfxBinding.get());
			gfxContext->SetVertexBuffer(gfxVB.get(), 0, sizeof(Vertex), 0);
			gfxContext->Draw(gfxVB->GetDesc().Size / sizeof(Vertex), 0, 1, 0);

			gfxContext->End();
		}

		// execute command lists
		realm.GetGfxSystem()->Render();

		// present
		gfxSwapChain->Present();

		// limit framerate
		Sleep(16);
	}

	// explicitly
	gfxSwapChain.reset(ur_null);

	return 0;
}