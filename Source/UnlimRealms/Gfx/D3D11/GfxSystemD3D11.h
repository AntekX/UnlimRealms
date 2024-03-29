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

#include <dxgi.h>
#include <d3d11.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

namespace UnlimRealms
{

	// referenced classes
	class WinCanvas;
	class GfxSystemD3D11;
	class GfxContextD3D11;
	class GfxPipelineStateObjectD3D11;
	class GfxResourceBindingD3D11;
	class GfxTextureD3D11;
	class GfxRenderTargetD3D11;
	class GfxSwapChainD3D11;
	class GfxBufferD3D11;
	class GfxVertexShaderD3D11;
	class GfxPixelShaderD3D11;
	class GfxInputLayoutD3D11;
	class GfxSamplerD3D11;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSystemD3D11 : public GfxSystem
	{
	public:

		GfxSystemD3D11(Realm &realm);

		virtual ~GfxSystemD3D11();

		virtual Result Initialize(Canvas *canvas);

		virtual Result Render();

		virtual Result CreateContext(std::unique_ptr<GfxContext> &gfxContext);

		virtual Result CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture);

		virtual Result CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT);

		virtual Result CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain);

		virtual Result CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer);

		virtual Result CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader);

		virtual Result CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader);

		virtual Result CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout); // TODO: deprecated, to be removed

		virtual Result CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState); // TODO: deprecated, to be removed

		virtual Result CreateSampler(std::unique_ptr<GfxSampler> &gfxSampler);

		virtual Result CreateResourceBinding(std::unique_ptr<GfxResourceBinding> &gfxBinding);

		virtual Result CreatePipelineStateObject(std::unique_ptr<GfxPipelineStateObject> &gfxPipelineState);

		inline WinCanvas* GetWinCanvas() const;

		inline IDXGIFactory1* GetDXGIFactory() const;

		inline ID3D11Device* GetDevice() const;

		inline ID3D11DeviceContext* GetDeviceContext() const;

		inline void AddCommandList(shared_ref<ID3D11CommandList> &d3dCommandList);

	private:

		Result InitializeDXGI();

		Result InitializeDevice(IDXGIAdapter *pAdapter,
			const D3D_FEATURE_LEVEL *featureLevels,
			UINT levelsCount);

		void ReleaseDXGIObjects();

		void ReleaseDeviceObjects();

		WinCanvas *winCanvas;
		shared_ref<IDXGIFactory1> dxgiFactory;
		std::vector<shared_ref<IDXGIAdapter1>> dxgiAdapters;
		D3D_FEATURE_LEVEL d3dFeatureLevel;
		IDXGIAdapter1 *d3dDeviceAdapter;
		shared_ref<ID3D11Device> d3dDevice;
		shared_ref<ID3D11DeviceContext> d3dImmediateContext;
		std::vector<shared_ref<ID3D11CommandList>> d3dCommandLists[2];
		ur_uint commandListId;
		std::mutex commandListMutex;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics context
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxContextD3D11 : public GfxContext
	{
	public:

		GfxContextD3D11(GfxSystem &gfxSystem);

		virtual ~GfxContextD3D11();

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

		virtual Result SetPipelineStateObject(GfxPipelineStateObject *state);

		virtual Result SetResourceBinding(GfxResourceBinding *binding);

		// TODO: deprected, to be removed
		#if 1
		virtual Result SetPipelineState(GfxPipelineState *state);

		virtual Result SetTexture(GfxTexture *texture, ur_uint slot);

		virtual Result SetConstantBuffer(GfxBuffer *buffer, ur_uint slot);

		virtual Result SetVertexShader(GfxVertexShader *shader);

		virtual Result SetPixelShader(GfxPixelShader *shader);
		#endif

		virtual Result SetVertexBuffer(GfxBuffer *buffer, ur_uint slot);

		virtual Result SetIndexBuffer(GfxBuffer *buffer);

		virtual Result Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, UpdateBufferCallback callback);

	private:

		Result SetupPipeline();

		enum StateFlag
		{
			PipelineStateFlag		= 0x1,
			ResourceBindingFlag		= 0x2
		};
		typedef ur_uint StateFlags;

		shared_ref<ID3D11DeviceContext> d3dContext;
		GfxPipelineStateObjectD3D11* gfxPipelineState;
		GfxResourceBindingD3D11* gfxResourceBinding;
		StateFlags stateFlags;
		ur_uint gfxPipelineStateVersion;
		ur_bool newGAPIMode; // TEMP: switches on/off new GAPI code branch depending on the used function(s)
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics texture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxTextureD3D11 : public GfxTexture
	{
	public:

		GfxTextureD3D11(GfxSystem &gfxSystem);

		virtual ~GfxTextureD3D11();

		Result Initialize(const GfxTextureDesc &desc, shared_ref<ID3D11Texture2D> &d3dTexture);

		inline ID3D11Texture2D* GetD3DTexture() const;

		inline ID3D11ShaderResourceView* GetSRV() const;

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		bool initializedFromD3DTex;
		shared_ref<ID3D11Texture2D> d3dTexture;
		shared_ref<ID3D11ShaderResourceView> d3dSRV;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics render target
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxRenderTargetD3D11 : public GfxRenderTarget
	{
	public:

		GfxRenderTargetD3D11(GfxSystem &gfxSystem);

		virtual ~GfxRenderTargetD3D11();

		inline ID3D11RenderTargetView* GetRTView() const;

		inline ID3D11DepthStencilView* GetDSView() const;

	protected:

		virtual Result OnInitialize();

	private:

		shared_ref<ID3D11RenderTargetView> d3dRTView;
		shared_ref<ID3D11DepthStencilView> d3dDSView;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Render target swap chain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSwapChainD3D11 : public GfxSwapChain
	{
	public:

		GfxSwapChainD3D11(GfxSystem &gfxSystem);

		virtual ~GfxSwapChainD3D11();

		virtual Result Initialize(const GfxPresentParams &params);

		virtual Result Present();

		virtual GfxRenderTarget* GetTargetBuffer();

	protected:

		class UR_DECL RenderTarget : public GfxRenderTargetD3D11
		{
		public:

			RenderTarget(GfxSystem &gfxSystem, shared_ref<ID3D11Texture2D> &dxgiSwapChainBuffer);

			virtual ~RenderTarget();

		protected:

			virtual Result InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc);

		private:
			
			shared_ref<ID3D11Texture2D> dxgiSwapChainBuffer;
		};

	private:

		std::unique_ptr<RenderTarget> targetBuffer;
		DXGI_SWAP_CHAIN_DESC dxgiChainDesc;
		shared_ref<IDXGISwapChain> dxgiSwapChain;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics buffer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxBufferD3D11 : public GfxBuffer
	{
	public:

		GfxBufferD3D11(GfxSystem &gfxSystem);

		virtual ~GfxBufferD3D11();

		inline ID3D11Buffer* GetD3DBuffer() const;

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		shared_ref<ID3D11Buffer> d3dBuffer;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics vertex shader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxVertexShaderD3D11 : public GfxVertexShader
	{
	public:

		GfxVertexShaderD3D11(GfxSystem &gfxSystem);

		virtual ~GfxVertexShaderD3D11();

		inline ID3D11VertexShader* GetD3DShader() const;

	protected:

		virtual Result OnInitialize();

	private:

		shared_ref<ID3D11VertexShader> d3dVertexShader;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics pixel shader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPixelShaderD3D11 : public GfxPixelShader
	{
	public:

		GfxPixelShaderD3D11(GfxSystem &gfxSystem);

		virtual ~GfxPixelShaderD3D11();

		inline ID3D11PixelShader* GetD3DShader() const;

	protected:

		virtual Result OnInitialize();

	private:

		shared_ref<ID3D11PixelShader> d3dPixelShader;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics input layout
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxInputLayoutD3D11 : public GfxInputLayout
	{
	public:

		GfxInputLayoutD3D11(GfxSystem &gfxSystem);

		virtual ~GfxInputLayoutD3D11();

		inline ID3D11InputLayout* GetD3DInputLayout() const;

	protected:

		virtual Result OnInitialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count);

	private:

		shared_ref<ID3D11InputLayout> d3dInputLayout;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics pipeline state
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPipelineStateD3D11 : public GfxPipelineState
	{
	public:

		GfxPipelineStateD3D11(GfxSystem &gfxSystem);

		virtual ~GfxPipelineStateD3D11();

		inline ID3D11SamplerState* GetD3DSamplerState(ur_uint idx) const;
		
		inline ID3D11RasterizerState* GetD3DRasterizerState() const;
		
		inline ID3D11DepthStencilState* GetD3DDepthStencilState() const;
		
		inline ID3D11BlendState* GetD3DBlendState() const;

	protected:

		virtual Result OnSetRenderState(const GfxRenderState &renderState);

	private:

		shared_ref<ID3D11SamplerState> d3dSamplerState[GfxRenderState::MaxSamplerStates];
		shared_ref<ID3D11RasterizerState> d3dRasterizerState;
		shared_ref<ID3D11DepthStencilState> d3dDepthStencilState;
		shared_ref<ID3D11BlendState> d3dBlendState;
		ur_size hashSamplerState[GfxRenderState::MaxSamplerStates];
		ur_size hashRasterizerState;
		ur_size hashDepthStencilState;
		ur_size hashBlendState;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 sampler
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSamplerD3D11 : public GfxSampler
	{
	public:

		GfxSamplerD3D11(GfxSystem &gfxSystem);

		virtual ~GfxSamplerD3D11();

		inline ID3D11SamplerState* GetD3DSamplerState() const;

	protected:

		virtual Result OnInitialize(const GfxSamplerState& state);

	private:

		shared_ref<ID3D11SamplerState> d3dSamplerState;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 graphics pipeline state object
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPipelineStateObjectD3D11 : public GfxPipelineStateObject
	{
	public:

		GfxPipelineStateObjectD3D11(GfxSystem &gfxSystem);

		virtual ~GfxPipelineStateObjectD3D11();

		inline ID3D11InputLayout* GetD3DInputLayout() const;
		
		inline ID3D11RasterizerState* GetD3DRasterizerState() const;
		
		inline ID3D11DepthStencilState* GetD3DDepthStencilState() const;
		
		inline ID3D11BlendState* GetD3DBlendState() const;

		inline ur_uint GetVersionID() const;

	protected:

		virtual Result OnInitialize(const StateFlags& changedStates);

	private:

		shared_ref<ID3D11InputLayout> d3dInputLayout;
		shared_ref<ID3D11RasterizerState> d3dRasterizerState;
		shared_ref<ID3D11DepthStencilState> d3dDepthStencilState;
		shared_ref<ID3D11BlendState> d3dBlendState;
		ur_uint versionID;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D11 resource binding
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxResourceBindingD3D11 : public GfxResourceBinding
	{
	public:

		GfxResourceBindingD3D11(GfxSystem &gfxSystem);

		virtual ~GfxResourceBindingD3D11();

		Result SetD3DContextResources(ID3D11DeviceContext* d3dContext);

	protected:

		virtual Result OnInitialize();
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	extern UR_DECL D3D11_USAGE GfxUsageToD3D11(GfxUsage usage);

	extern UR_DECL D3D11_BIND_FLAG GfxBindFlagToD3D11(GfxBindFlag flag);

	extern UR_DECL UINT GfxBindFlagsToD3D11(ur_uint flags);

	extern UR_DECL D3D11_CPU_ACCESS_FLAG GfxAccessFlagToD3D11_CPUAccessFlag(GfxAccessFlag flag);

	extern UR_DECL UINT GfxAccessFlagsToD3D11_CPUAccessFlags(ur_uint flags);

	extern UR_DECL D3D11_MAP GfxGPUAccessFlagToD3D11(GfxGPUAccess gpuAccess);

	extern UR_DECL D3D11_VIEWPORT GfxViewPortToD3D11(const GfxViewPort &viewPort);

	extern UR_DECL GfxViewPort GfxViewPortFromD3D11(const D3D11_VIEWPORT &viewPort);

	extern UR_DECL D3D11_RECT RectIToD3D11(const RectI &rect);

	extern UR_DECL D3D11_TEXTURE2D_DESC GfxTextureDescToD3D11(const GfxTextureDesc &desc);

	extern UR_DECL D3D11_BUFFER_DESC GfxBufferDescToD3D11(const GfxBufferDesc &desc);

	extern UR_DECL LPCSTR GfxSemanticToD3D11(GfxSemantic semantic);

	extern UR_DECL D3D11_INPUT_ELEMENT_DESC GfxInputElementToD3D11(const GfxInputElement &element);

	extern UR_DECL D3D11_PRIMITIVE_TOPOLOGY GfxPrimitiveTopologyToD3D11(GfxPrimitiveTopology topology);

	extern UR_DECL D3D11_RENDER_TARGET_BLEND_DESC GfxBlendStateToD3D11(const GfxBlendState &state);

	extern UR_DECL D3D11_BLEND GfxBlendFactorToD3D11(GfxBlendFactor blendFactor);

	extern UR_DECL D3D11_BLEND_OP GfxBlendOpToD3D11(GfxBlendOp blendOp);

	extern UR_DECL D3D11_COMPARISON_FUNC GfxCmpFuncToD3D11(GfxCmpFunc func);

	extern UR_DECL D3D11_FILTER GfxFilterToD3D11(GfxFilter minFilter, GfxFilter magFilter, GfxFilter mipFilter);
	
	extern UR_DECL D3D11_TEXTURE_ADDRESS_MODE GfxTextureAddressModeToD3D11(GfxTextureAddressMode mode);

	extern UR_DECL D3D11_SAMPLER_DESC GfxSamplerStateToD3D11(const GfxSamplerState &state);

	extern UR_DECL D3D11_FILL_MODE GfxFillModeToD3D11(GfxFillMode mode);

	extern UR_DECL D3D11_CULL_MODE GfxCullModeToD3D11(GfxCullMode mode);

	extern UR_DECL D3D11_RASTERIZER_DESC GfxRasterizerStateToD3D11(const GfxRasterizerState& state);

	extern UR_DECL D3D11_STENCIL_OP GfxStencilOpToD3D11(GfxStencilOp stencilOp);

	extern UR_DECL D3D11_DEPTH_STENCILOP_DESC GfxDepthStencilOpDescToD3D11(const GfxDepthStencilOpDesc& desc);

	extern UR_DECL D3D11_DEPTH_STENCIL_DESC GfxDepthStencilStateToD3D11(const GfxDepthStencilState &state);

} // end namespace UnlimRealms

#include "Gfx/D3D11/GfxSystemD3D11.inline.h"