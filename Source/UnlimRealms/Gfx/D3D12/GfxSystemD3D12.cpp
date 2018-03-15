///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Gfx/D3D12/GfxSystemD3D12.h"
#include "Gfx/DXGIUtils/DXGIUtils.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSystemD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSystemD3D12::GfxSystemD3D12(Realm &realm) :
		GfxSystem(realm)
	{
		this->winCanvas = ur_null;
	}

	GfxSystemD3D12::~GfxSystemD3D12()
	{
	}

	Result GfxSystemD3D12::InitializeDXGI()
	{
		this->ReleaseDXGIObjects();

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

		HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(&this->dxgiFactory));
		if (FAILED(hr))
			return ResultError(Failure, "GfxSystemD3D12: failed to create DXGI factory");

		UINT adapterId = 0;
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		do
		{
			shared_ref<IDXGIAdapter1> dxgiAdapter;
			hr = this->dxgiFactory->EnumAdapters1(adapterId++, dxgiAdapter);
			if (SUCCEEDED(hr))
			{
				dxgiAdapter->GetDesc1(&dxgiAdapterDesc);
				this->dxgiAdapters.push_back(dxgiAdapter);
				this->gfxAdapters.push_back(DXGIAdapterDescToGfx(dxgiAdapterDesc));
			}
		} while (SUCCEEDED(hr));

		return (!this->dxgiAdapters.empty() ?
			Result(Success) :
			ResultError(Failure, "GfxSystemD3D12: failed to enumerate adapaters"));
	}

	Result GfxSystemD3D12::InitializeDevice(IDXGIAdapter *pAdapter, const D3D_FEATURE_LEVEL minimalFeatureLevel)
	{
		this->ReleaseDeviceObjects();

		if (ur_null == pAdapter)
		{
			this->gfxAdapterIdx = 0;
			// try to find high performance GPU
			// let's consider that HP GPU is the one with the highest amount of dedicated VRAM on board
			DXGI_ADAPTER_DESC1 adapterDesc;
			ur_size dedicatedMemoryMax = 0;
			for (ur_size idx = 0; idx < this->dxgiAdapters.size(); ++idx)
			{
				this->dxgiAdapters[idx]->GetDesc1(&adapterDesc);
				ur_size dedicatedMemoryMb = adapterDesc.DedicatedVideoMemory / (1 << 20);
				if (dedicatedMemoryMb > dedicatedMemoryMax)
				{
					dedicatedMemoryMax = dedicatedMemoryMb;
					this->gfxAdapterIdx = ur_uint(idx);
					break;
				}
			}
			pAdapter = *(this->dxgiAdapters.begin() + this->gfxAdapterIdx);
		}

		HRESULT hr = D3D12CreateDevice(pAdapter, minimalFeatureLevel, __uuidof(ID3D12Device), this->d3dDevice);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSystemD3D12: failed to initialize Direct3D device");

		#if defined(_PROFILE)
		this->d3dDevice->SetStablePowerState(true);
		#endif

		return Result(Success);
	}

	void GfxSystemD3D12::ReleaseDXGIObjects()
	{
		this->gfxAdapters.clear();
		this->dxgiAdapters.clear();
		this->dxgiFactory.reset(ur_null);
	}

	void GfxSystemD3D12::ReleaseDeviceObjects()
	{
		this->d3dDevice.reset(ur_null);
	}

	Result GfxSystemD3D12::Initialize(Canvas *canvas)
	{
		if (ur_null == canvas)
			return ResultError(InvalidArgs, "GfxSystemD3D12: failed to initialize, target canvas is null");

		if (typeid(*canvas).hash_code() != typeid(WinCanvas).hash_code())
			return ResultError(InvalidArgs, "GfxSystemD3D12: failed to initialize, target canvas type is unsupported");

		this->winCanvas = static_cast<WinCanvas*>(canvas);

		Result res = this->InitializeDXGI();
		if (Succeeded(res))
		{
			res = this->InitializeDevice(ur_null, D3D_FEATURE_LEVEL_11_0);
		}

		return (Succeeded(res) ?
			ResultNote(Success, "GfxSystemD3D12: initialized") :
			ResultError(res.Code, "GfxSystemD3D12: failed to initialize"));
	}

	Result GfxSystemD3D12::Render()
	{
		return NotImplemented;
	}

	Result GfxSystemD3D12::CreateContext(std::unique_ptr<GfxContext> &gfxContext)
	{
		gfxContext.reset(new GfxContextD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture)
	{
		gfxTexture.reset(new GfxTextureD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT)
	{
		gfxRT.reset(new GfxRenderTargetD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain)
	{
		gfxSwapChain.reset(new GfxSwapChainD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer)
	{
		gfxBuffer.reset(new GfxBufferD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader)
	{
		return NotImplemented;
	}

	Result GfxSystemD3D12::CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader)
	{
		return NotImplemented;
	}

	Result GfxSystemD3D12::CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout)
	{
		return NotImplemented;
	}

	Result GfxSystemD3D12::CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxContextD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxContextD3D12::GfxContextD3D12(GfxSystem &gfxSystem) :
		GfxContext(gfxSystem)
	{
	}

	GfxContextD3D12::~GfxContextD3D12()
	{
	}

	Result GfxContextD3D12::Initialize()
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::Begin()
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::End()
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetRenderTarget(GfxRenderTarget *rt)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetRenderTarget(GfxRenderTarget *rt, GfxRenderTarget *ds)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetViewPort(const GfxViewPort *viewPort)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::GetViewPort(GfxViewPort &viewPort)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetScissorRect(const RectI *rect)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::ClearTarget(GfxRenderTarget *rt,
		bool clearColor, const ur_float4 &color,
		bool clearDepth, const ur_float &depth,
		bool clearStencil, const ur_uint &stencil)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetPipelineState(GfxPipelineState *state)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetTexture(GfxTexture *texture, ur_uint slot)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetConstantBuffer(GfxBuffer *buffer, ur_uint slot)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetVertexBuffer(GfxBuffer *buffer, ur_uint slot, ur_uint stride, ur_uint offset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetIndexBuffer(GfxBuffer *buffer, ur_uint bitsPerIndex, ur_uint offset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetVertexShader(GfxVertexShader *shader)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetPixelShader(GfxPixelShader *shader)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics texture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxTextureD3D12::GfxTextureD3D12(GfxSystem &gfxSystem) :
		GfxTexture(gfxSystem)
	{
	}

	GfxTextureD3D12::~GfxTextureD3D12()
	{
	}

	Result GfxTextureD3D12::Initialize(const GfxTextureDesc &desc, shared_ref<ID3D12Resource> &d3dTexture)
	{
		return NotImplemented;
	}

	Result GfxTextureD3D12::OnInitialize(const GfxResourceData *data)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics render target
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxRenderTargetD3D12::GfxRenderTargetD3D12(GfxSystem &gfxSystem) :
		GfxRenderTarget(gfxSystem)
	{
	}

	GfxRenderTargetD3D12::~GfxRenderTargetD3D12()
	{
	}

	Result GfxRenderTargetD3D12::OnInitialize()
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 render targets swap chain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSwapChainD3D12::GfxSwapChainD3D12(GfxSystem &gfxSystem) :
		GfxSwapChain(gfxSystem)
	{
		memset(&this->dxgiChainDesc, 0, sizeof(this->dxgiChainDesc));
		this->backBufferIndex = 0;
	}

	GfxSwapChainD3D12::~GfxSwapChainD3D12()
	{
	}

	Result GfxSwapChainD3D12::Initialize(const GfxPresentParams &params)
	{
		this->backBuffers.clear();

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		IDXGIFactory4 *dxgiFactory = d3dSystem.GetDXGIFactory();
		ID3D12Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dSystem.GetWinCanvas())
			return ResultError(NotInitialized, "GfxSwapChainD3D12::Initialize: failed, canvas not initialized");

		this->dxgiChainDesc.Width = params.BufferWidth;
		this->dxgiChainDesc.Height = params.BufferHeight;
		this->dxgiChainDesc.Format = GfxFormatToDXGI(params.BufferFormat, GfxFormatView::Unorm);
		this->dxgiChainDesc.Stereo = false;
		this->dxgiChainDesc.SampleDesc.Count = params.MutisampleCount;
		this->dxgiChainDesc.SampleDesc.Quality = params.MutisampleQuality;
		this->dxgiChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		this->dxgiChainDesc.BufferCount = params.BufferCount;
		this->dxgiChainDesc.Scaling = DXGI_SCALING_STRETCH;
		this->dxgiChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		this->dxgiChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		this->dxgiChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		shared_ref<IDXGISwapChain1> dxgiSwapChain1;
		HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(d3dDevice, d3dSystem.GetWinCanvas()->GetHwnd(),
			&this->dxgiChainDesc, nullptr, nullptr, dxgiSwapChain1);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSwapChainD3D12::Initialize: failed to create DXGI swap chain");

		GfxTextureDesc desc;
		desc.Width = params.BufferWidth;
		desc.Height = params.BufferHeight;
		desc.Levels = 1;
		desc.Format = params.BufferFormat;
		desc.FormatView = GfxFormatView::Unorm;
		desc.Usage = GfxUsage::Default;
		desc.BindFlags = ur_uint(GfxBindFlag::RenderTarget);
		desc.AccessFlags = ur_uint(0);

		std::unique_ptr<BackBuffer> dummy;
		this->backBuffers.reserve(params.BufferCount);
		for (ur_uint ibuffer = 0; ibuffer < params.BufferCount; ++ibuffer)
		{
			shared_ref<ID3D12Resource> d3dResource;
			hr = this->dxgiSwapChain->GetBuffer(ibuffer, __uuidof(ID3D12Resource), d3dResource);
			if (FAILED(hr))
				return ResultError(Failure, "GfxSwapChainD3D12::Initialize: failed to retrieve back buffer resource");

			std::unique_ptr<GfxSwapChainD3D12::BackBuffer> backBuffer(new BackBuffer(d3dSystem, d3dResource));
			Result res = backBuffer->Initialize(desc, params.DepthStencilEnabled, params.DepthStencilFormat);
			if (Failed(res))
				return ResultError(res.Code, "GfxSwapChainD3D12::Initialize: failed to initialize back buffer");

			this->backBuffers.push_back(std::move(backBuffer));
		}

		dxgiSwapChain1->QueryInterface(__uuidof(IDXGISwapChain3), this->dxgiSwapChain);
		this->backBufferIndex = (ur_uint)this->dxgiSwapChain->GetCurrentBackBufferIndex();

		return Result(Success);
	}

	Result GfxSwapChainD3D12::Present()
	{
		if (this->dxgiSwapChain.empty())
			return ResultError(NotInitialized, "GfxSwapChainD3D12::Present: DXGI swap chain not initialized");

		if (FAILED(this->dxgiSwapChain->Present(0, 0)))
			return ResultError(Failure, "GfxSwapChainD3D12::Present: failed");

		return Result(Success);
	}

	GfxRenderTarget* GfxSwapChainD3D12::GetTargetBuffer()
	{
		if (this->backBufferIndex > (ur_uint)this->backBuffers.size())
			return ur_null;

		return this->backBuffers[this->backBufferIndex].get();
	}

	GfxSwapChainD3D12::BackBuffer::BackBuffer(GfxSystem &gfxSystem, shared_ref<ID3D12Resource> &dxgiSwapChainBuffer) :
		GfxRenderTargetD3D12(gfxSystem),
		dxgiSwapChainBuffer(dxgiSwapChainBuffer)
	{
	}

	GfxSwapChainD3D12::BackBuffer::~BackBuffer()
	{
	}

	Result GfxSwapChainD3D12::BackBuffer::InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc)
	{
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());

		std::unique_ptr<GfxTextureD3D12> newTargetBuffer(new GfxTextureD3D12(d3dSystem));
		Result res = newTargetBuffer->Initialize(desc, this->dxgiSwapChainBuffer);
		if (Failed(res))
			return ResultError(res.Code, "GfxSwapChainD3D12::BackBuffer::InitializeTargetBuffer: failed");

		resultBuffer = std::move(newTargetBuffer);

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 render targets swap chain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxBufferD3D12::GfxBufferD3D12(GfxSystem &gfxSystem) :
		GfxBuffer(gfxSystem)
	{
	}

	GfxBufferD3D12::~GfxBufferD3D12()
	{
	}

	Result GfxBufferD3D12::OnInitialize(const GfxResourceData *data)
	{
		return NotImplemented;
	}

} // end namespace UnlimRealms