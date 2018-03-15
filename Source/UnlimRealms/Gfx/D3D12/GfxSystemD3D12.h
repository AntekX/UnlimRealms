///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Gfx/GfxSystem.h"
#include "Sys/Windows/WinUtils.h"

// DirectX

#include <dxgi1_4.h>
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace UnlimRealms
{
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSystemD3D12 : public GfxSystem
	{
	public:

		GfxSystemD3D12(Realm &realm);

		virtual ~GfxSystemD3D12();

		virtual Result Initialize(Canvas *canvas);

		virtual Result Render();

		virtual Result CreateContext(std::unique_ptr<GfxContext> &gfxContext);

		virtual Result CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture);

		virtual Result CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT);

		virtual Result CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain);

		virtual Result CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer);

		virtual Result CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader);

		virtual Result CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader);

		virtual Result CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout);

		virtual Result CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState);

		inline WinCanvas* GetWinCanvas() const;

		inline IDXGIFactory4* GetDXGIFactory() const;

		inline ID3D12Device* GetDevice() const;

	private:

		Result InitializeDXGI();

		Result InitializeDevice(IDXGIAdapter *pAdapter, const D3D_FEATURE_LEVEL minimalFeatureLevel);

		void ReleaseDXGIObjects();

		void ReleaseDeviceObjects();

		WinCanvas *winCanvas;
		shared_ref<IDXGIFactory4> dxgiFactory;
		std::vector<shared_ref<IDXGIAdapter1>> dxgiAdapters;
		shared_ref<ID3D12Device> d3dDevice;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics context
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxContextD3D12 : public GfxContext
	{
	public:

		GfxContextD3D12(GfxSystem &gfxSystem);

		virtual ~GfxContextD3D12();

		virtual Result Initialize();

		virtual Result Begin();

		virtual Result End();

		virtual Result SetRenderTarget(GfxRenderTarget *rt);

		virtual Result SetRenderTarget(GfxRenderTarget *rt, GfxRenderTarget *ds);

		virtual Result SetViewPort(const GfxViewPort *viewPort);

		virtual Result GetViewPort(GfxViewPort &viewPort);

		virtual Result SetScissorRect(const RectI *rect);

		virtual Result ClearTarget(GfxRenderTarget *rt,
			bool clearColor, const ur_float4 &color,
			bool clearDepth, const ur_float &depth,
			bool clearStencil, const ur_uint &stencil);

		virtual Result SetPipelineState(GfxPipelineState *state);

		virtual Result SetTexture(GfxTexture *texture, ur_uint slot);

		virtual Result SetConstantBuffer(GfxBuffer *buffer, ur_uint slot);

		virtual Result SetVertexBuffer(GfxBuffer *buffer, ur_uint slot, ur_uint stride, ur_uint offset);

		virtual Result SetIndexBuffer(GfxBuffer *buffer, ur_uint bitsPerIndex, ur_uint offset);

		virtual Result SetVertexShader(GfxVertexShader *shader);

		virtual Result SetPixelShader(GfxPixelShader *shader);

		virtual Result Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback);

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics texture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxTextureD3D12 : public GfxTexture
	{
	public:

		GfxTextureD3D12(GfxSystem &gfxSystem);

		virtual ~GfxTextureD3D12();

		Result Initialize(const GfxTextureDesc &desc, shared_ref<ID3D12Resource> &d3dTexture);

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics render target
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxRenderTargetD3D12 : public GfxRenderTarget
	{
	public:

		GfxRenderTargetD3D12(GfxSystem &gfxSystem);

		virtual ~GfxRenderTargetD3D12();

	protected:

		virtual Result OnInitialize();

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 render targets swap chain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSwapChainD3D12 : public GfxSwapChain
	{
	public:

		GfxSwapChainD3D12(GfxSystem &gfxSystem);

		virtual ~GfxSwapChainD3D12();

		virtual Result Initialize(const GfxPresentParams &params);

		virtual Result Present();

		virtual GfxRenderTarget* GetTargetBuffer();

	protected:

		class UR_DECL BackBuffer : public GfxRenderTargetD3D12
		{
		public:

			BackBuffer(GfxSystem &gfxSystem, shared_ref<ID3D12Resource> &dxgiSwapChainBuffer);

			virtual ~BackBuffer();

		protected:

			virtual Result InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc);

		private:

			shared_ref<ID3D12Resource> dxgiSwapChainBuffer;
		};

	private:

		DXGI_SWAP_CHAIN_DESC1 dxgiChainDesc;
		shared_ref<IDXGISwapChain3> dxgiSwapChain;
		ur_uint backBufferIndex;
		std::vector<std::unique_ptr<BackBuffer>> backBuffers;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics buffer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxBufferD3D12 : public GfxBuffer
	{
	public:

		GfxBufferD3D12(GfxSystem &gfxSystem);

		virtual ~GfxBufferD3D12();

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);
	};

} // end namespace UnlimRealms

#include "Gfx/D3D12/GfxSystemD3D12.inline.h"