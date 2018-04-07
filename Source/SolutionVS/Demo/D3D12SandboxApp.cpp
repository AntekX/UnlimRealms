
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
#include "Core/Math.h"
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
			false, GfxFormat::R8G8B8A8, true, GfxFormat::R24G8, 3);
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

	std::unique_ptr<GfxBuffer> gfxVB;
	if (Succeeded(realm.GetGfxSystem()->CreateBuffer(gfxVB)))
	{
		struct Vertex
		{
			ur_float3 pos;
			ur_float4 color;
			ur_float2 tex;
		} bufferData[] = {
			{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 2.0f } },
			{ {  0.00f,  0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 2.0f, 2.0f } }
		};
		GfxResourceData bufferDataDesc;
		bufferDataDesc.Ptr = bufferData;
		bufferDataDesc.RowPitch = sizeof(bufferData);
		bufferDataDesc.SlicePitch = 0;

		gfxVB->Initialize(sizeof(bufferData), sizeof(Vertex), GfxUsage::Dynamic, ur_uint(GfxBindFlag::VertexBuffer), 0, &bufferDataDesc);
	}

	const ur_uint InstanceCount = 16;
	struct Constants
	{
		ur_float4x4 Transform[InstanceCount];
	};
	Constants cbData = { ur_float4x4::Identity };
	GfxResourceData cbResData = { &cbData, sizeof(Constants) , 0};
	std::unique_ptr<GfxBuffer> gfxCB;
	if (Succeeded(realm.GetGfxSystem()->CreateBuffer(gfxCB)))
	{
		gfxCB->Initialize(sizeof(Constants), 0, GfxUsage::Dynamic, ur_uint(GfxBindFlag::ConstantBuffer), ur_uint(GfxAccessFlag::Write), &cbResData);
	}

	std::unique_ptr<GfxTexture> gfxTexture;
	realm.GetGfxSystem()->CreateTexture(gfxTexture);
	//CreateTextureFromFile(realm, gfxTexture, "Res/Rock_desert_03_FWD.dds");
	CreateTextureFromFile(realm, gfxTexture, "Res/testimage.dds");

	GfxSamplerState gfxSampler = GfxSamplerState::Default;
	gfxSampler.AddressU = GfxTextureAddressMode::Wrap;
	gfxSampler.AddressV = GfxTextureAddressMode::Wrap;

	GfxRasterizerState gfxRasterizer = GfxRasterizerState::Default;
	gfxRasterizer.CullMode = GfxCullMode::None;

	GfxDepthStencilState gfxDepthStencil = GfxDepthStencilState::Default;
	gfxDepthStencil.DepthEnable = false;
	gfxDepthStencil.DepthWriteEnable = false;

	// declare binding matching used shader registers
	std::unique_ptr<GfxResourceBinding> gfxBinding;
	realm.GetGfxSystem()->CreateResourceBinding(gfxBinding);
	gfxBinding->SetBuffer(0, gfxCB.get());
	gfxBinding->SetTexture(0, gfxTexture.get());
	gfxBinding->SetSampler(0, &gfxSampler);
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

	// animation
	ClockTime timer = Clock::now();
	const ur_float moveSpeed = 0.75f;
	ur_float2 movePos[InstanceCount];
	ur_float2 moveDir[InstanceCount];
	for (ur_uint i = 0; i < InstanceCount; ++i)
	{
		movePos[i] = { 0.0f, 0.0f };
		moveDir[i].x = (ur_float)rand() / RAND_MAX * 2.0f - 1.0f;
		moveDir[i].y = (ur_float)rand() / RAND_MAX * 2.0f - 1.0f;
		moveDir[i].Normalize();
	}

	// Main message loop:
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
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
			break;
		}
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

		{ // use context to draw
			gfxContext->Begin();

			// prepare RT
			gfxContext->SetRenderTarget(gfxSwapChain->GetTargetBuffer(), ur_null);
			gfxContext->ClearTarget(gfxSwapChain->GetTargetBuffer(), true, { 0.0f, 0.2f, 0.4f, 1.0f }, false, 0.0f, false, 0);

			// animate primitives
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
				cbData.Transform[i] = cbData.Transform[i].Multiply(ur_float4x4::Scaling(1.0f, ur_float(canvasWidth) / canvasHeight, 1.0f));
				cbData.Transform[i] = cbData.Transform[i].Multiply(ur_float4x4::Translation(movePos[i].x, movePos[i].y, 0.0f));
			}
			gfxContext->UpdateBuffer(gfxCB.get(), GfxGPUAccess::Write, false, &cbResData, 0, 0);
			
			// draw primitives
			gfxContext->SetPipelineStateObject(gfxPSO.get());
			gfxContext->SetResourceBinding(gfxBinding.get());
			gfxContext->SetVertexBuffer(gfxVB.get(), 0);
			gfxContext->Draw(gfxVB->GetDesc().Size / gfxVB->GetDesc().ElementSize, 0, InstanceCount, 0);

			gfxContext->End();
		}

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