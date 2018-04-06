//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "D3D12HelloTriangle.h"

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

D3D12HelloTriangle::D3D12HelloTriangle(Realm& realm, HWND hwnd, UINT width, UINT height, std::wstring name) :
	DXSample(hwnd, width, height, name),
	m_realm(realm),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
#if (USE_GFX_LIB)
	
#else
	, m_frameIndex(0)
	, m_rtvDescriptorSize(0)
#endif
{
}

void D3D12HelloTriangle::OnInit(HWND hwnd)
{
	LoadPipeline(hwnd);
	LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12HelloTriangle::LoadPipeline(HWND hwnd)
{
#if (USE_GFX_LIB)

	// create gfx system
	std::unique_ptr<GfxSystemD3D12> gfx(new GfxSystemD3D12(m_realm));
	Result res = gfx->Initialize(m_realm.GetCanvas());
	m_realm.SetGfxSystem(std::move(gfx));

	// create swap chain
	ur_uint canvasWidth = m_realm.GetCanvas()->GetClientBound().Width();
	ur_uint canvasHeight = m_realm.GetCanvas()->GetClientBound().Height();
	std::unique_ptr<GfxSwapChain> gfxSwapChain;
	if (Succeeded(m_realm.GetGfxSystem()->CreateSwapChain(gfxSwapChain)))
	{
		res = gfxSwapChain->Initialize(canvasWidth, canvasHeight,
			false, GfxFormat::R8G8B8A8, true, GfxFormat::R24G8, FrameCount);
		m_gfxSwapChain.reset(static_cast<GfxSwapChainD3D12*>(gfxSwapChain.release()));
	}

	// create gfx context
	std::unique_ptr<GfxContext> gfxContext;
	if (Succeeded(m_realm.GetGfxSystem()->CreateContext(gfxContext)))
	{
		res = gfxContext->Initialize();
		m_gfxContext.reset(static_cast<GfxContextD3D12*>(gfxContext.release()));
	}

#else

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
			));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
			));
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
		));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
#endif
}

// Load the sample assets.
void D3D12HelloTriangle::LoadAssets()
{
	ID3D12Device* device = ur_null;
	ID3D12CommandAllocator* commandAllocator = ur_null;
#if (USE_GFX_LIB)
	GfxSystemD3D12* gfxSysytem = static_cast<GfxSystemD3D12*>(m_realm.GetGfxSystem());
	device = gfxSysytem->GetD3DDevice();
	commandAllocator = gfxSysytem->GetD3DCommandAllocator();
#else
	device = m_device.Get();
	commandAllocator = m_commandAllocator.Get();
#endif
	// Create an empty root signature.
	ID3D12RootSignature* rootSignature = nullptr;
#if (USE_GFX_LIB)
	gfxSysytem->CreateResourceBinding(m_gfxBinding);
	m_gfxBinding->Initialize();
	rootSignature = static_cast<GfxResourceBindingD3D12*>(m_gfxBinding.get())->GetD3DRootSignature();
#else
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		rootSignature = m_rootSignature.Get();
	}
#endif

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		// Describe and create the graphics pipeline state object (PSO).
#if (USE_GFX_LIB)
		gfxSysytem->CreateVertexShader(m_gfxVertexShader);
		CreateVertexShaderFromFile(m_realm, m_gfxVertexShader, "sample_vs.cso");
		
		gfxSysytem->CreatePixelShader(m_gfxPixelShader);
		CreatePixelShaderFromFile(m_realm, m_gfxPixelShader, "sample_ps.cso");

		gfxSysytem->CreateInputLayout(m_gfxInputLayout);
		GfxInputElement gfxInputElements[] = {
			{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 },
			{ GfxSemantic::Color, 0, 0, GfxFormat::R32G32B32A32, GfxFormatView::Float, 0 },
			{ GfxSemantic::TexCoord, 0, 0, GfxFormat::R32G32, GfxFormatView::Float, 0 }
		};		
		m_gfxInputLayout->Initialize(*m_gfxVertexShader.get(), gfxInputElements, _countof(gfxInputElements));

		GfxRasterizerState gfxRasterizer = GfxRasterizerState::Default;
		gfxRasterizer.CullMode = GfxCullMode::None;

		GfxDepthStencilState gfxDepthStencil = GfxDepthStencilState::Default;
		gfxDepthStencil.DepthEnable = false;
		gfxDepthStencil.StencilEnable = false;
		
		gfxSysytem->CreatePipelineStateObject(m_gfxPipelineState);
		m_gfxPipelineState->SetResourceBinding(m_gfxBinding.get());
		m_gfxPipelineState->SetInputLayout(m_gfxInputLayout.get());
		m_gfxPipelineState->SetVertexShader(m_gfxVertexShader.get());
		m_gfxPipelineState->SetPixelShader(m_gfxPixelShader.get());
		m_gfxPipelineState->SetRasterizerState(gfxRasterizer);
		m_gfxPipelineState->SetBlendState(GfxBlendState::Default);
		m_gfxPipelineState->SetDepthStencilState(gfxDepthStencil);
		m_gfxPipelineState->SetPrimitiveTopology(GfxPrimitiveTopology::TriangleList);
		m_gfxPipelineState->Initialize();
#else
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };		
		psoDesc.pRootSignature = rootSignature;
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
#endif
	}

#if (!USE_GFX_LIB)
	// Create the command list.
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(m_commandList->Close());
#endif

	// Create the vertex buffer.
	{		
#if (USE_GFX_LIB)

		m_realm.GetGfxSystem()->CreateBuffer(m_gfxVertexBuffer);

		// Define the geometry for a triangle.
		GfxVertex triangleVertices[] =
		{
			{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0, 0.0 } },
			{ {  0.0f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.5, 1.0 } },
			{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0, 0.0 } }
		};
		
		GfxResourceData bufferDataDesc;
		bufferDataDesc.Ptr = triangleVertices;
		bufferDataDesc.RowPitch = sizeof(triangleVertices);
		bufferDataDesc.SlicePitch = 0;

		GfxBufferDesc bufferDesc = {};
		bufferDesc.Size = bufferDataDesc.RowPitch;
		bufferDesc.Usage = GfxUsage::Default;
		bufferDesc.BindFlags = ur_uint(GfxBindFlag::VertexBuffer);

		m_gfxVertexBuffer->Initialize(bufferDesc, &bufferDataDesc);
#else
		// Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f * m_aspectRatio, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
		};
		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
#endif
	}

#if (!USE_GFX_LIB)
	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForPreviousFrame();
	}
#endif
}

// Update frame-based values.
void D3D12HelloTriangle::OnUpdate()
{
}

// Render the scene.
void D3D12HelloTriangle::OnRender()
{
	ID3D12CommandQueue* commandQueue = nullptr;
	IDXGISwapChain3* swapChain = nullptr;
#if (USE_GFX_LIB)
	GfxSystemD3D12* gfxSysytem = static_cast<GfxSystemD3D12*>(m_realm.GetGfxSystem());
	swapChain = static_cast<GfxSwapChainD3D12*>(m_gfxSwapChain.get())->GetDXGISwapChain();
	commandQueue = gfxSysytem->GetD3DCommandQueue();
#else
	commandQueue = m_commandQueue.Get();
	swapChain = m_swapChain.Get();
#endif

	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
#if (USE_GFX_LIB)
	m_realm.GetGfxSystem()->Render();
#else
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
#endif

	// Present the frame.
#if (USE_GFX_LIB)
	m_gfxSwapChain->Present();
#else
	ThrowIfFailed(swapChain->Present(1, 0));
#endif

	WaitForPreviousFrame();
}

void D3D12HelloTriangle::OnDestroy()
{
#if (USE_GFX_LIB)
	m_gfxContext.reset(nullptr);
	m_gfxSwapChain.reset(nullptr);
#else
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
#endif
}

void D3D12HelloTriangle::PopulateCommandList()
{
#if (USE_GFX_LIB)
	GfxSystemD3D12* gfxSystem = static_cast<GfxSystemD3D12*>(m_realm.GetGfxSystem());

	m_gfxContext->Begin();

	// Set necessary state.
	m_gfxContext->SetPipelineStateObject(m_gfxPipelineState.get());
	m_gfxContext->SetResourceBinding(m_gfxBinding.get());

	// Set RT
	m_gfxContext->SetRenderTarget(m_gfxSwapChain->GetTargetBuffer());

	// Record commands.
	const ur_float4 clearColor = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_gfxContext->ClearTarget(m_gfxSwapChain->GetTargetBuffer(), true, clearColor, false, 1.0f, false, 0);
	m_gfxContext->SetVertexBuffer(m_gfxVertexBuffer.get(), 0, sizeof(GfxVertex), 0);
	m_gfxContext->Draw(3, 0, 1, 0);

	m_gfxContext->End();

#else

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());
#endif
}

void D3D12HelloTriangle::WaitForPreviousFrame()
{
#if (!USE_GFX_LIB)
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
#endif
}

int D3D12HelloTriangleApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindow, L"D3D12 Sandbox"));
	canvas->Initialize(RectI(0, 0, 1280, 720));
	realm.SetCanvas(std::move(canvas));
	WinCanvas* winCanvas = static_cast<WinCanvas*>(realm.GetCanvas());

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// create sample
	D3D12HelloTriangle sample(realm, winCanvas->GetHwnd(), (UINT)winCanvas->GetClientBound().Width(), (UINT)winCanvas->GetClientBound().Height(), L"D3D12HelloTriangle");
	sample.OnInit(winCanvas->GetHwnd());

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

		sample.OnUpdate();
		sample.OnRender();
	}

	sample.OnDestroy();

	return 0;
}