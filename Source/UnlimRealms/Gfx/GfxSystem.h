///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxTypes.h"

namespace UnlimRealms
{

	// forward declarations
	class GfxSystem;
	class GfxEntity;
	class GfxContext;
	class GfxTexture;
	class GfxRenderTarget;
	class GfxSwapChain;
	class GfxBuffer;
	class GfxShader;
	class GfxVertexShader;
	class GfxPixelShader;
	class GfxInputLayout;
	class GfxPipelineState;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSystem : public RealmEntity
	{
	public:

		GfxSystem(Realm &realm);

		virtual ~GfxSystem();

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

		inline const ur_uint GetAdaptersCount() const;

		inline const GfxAdapterDesc& GetAdapterDesc(ur_uint idx) const;

		inline const GfxAdapterDesc& GetActiveAdapterDesc() const;

		inline ur_uint GetActiveAdapterIdx() const;

	protected:

		std::vector<GfxAdapterDesc> gfxAdapters;
		ur_uint gfxAdapterIdx;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics system entity
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxEntity : public RealmEntity
	{
	public:

		GfxEntity(GfxSystem &gfxSystem);

		~GfxEntity();
		
		inline GfxSystem& GetGfxSystem();

	private:

		GfxSystem &gfxSystem;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics context
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxContext : public GfxEntity
	{
	public:

		GfxContext(GfxSystem &gfxSystem);

		virtual ~GfxContext();

		virtual Result Initialize();

		virtual Result Begin();

		virtual Result End();

		virtual Result SetRenderTarget(GfxRenderTarget *rt);

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

		virtual Result UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, const GfxResourceData *data, ur_uint offset, ur_uint size);

		typedef std::function<Result (GfxResourceData *data)> UpdateBufferCallback;
		virtual Result UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback);
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics texture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxTexture : public GfxEntity
	{
	public:

		GfxTexture(GfxSystem &gfxSystem);

		virtual ~GfxTexture();

		Result Initialize(const GfxTextureDesc &desc, const GfxResourceData *data);

		Result Initialize(
			ur_uint width,
			ur_uint height,
			ur_uint levels = 1,
			GfxFormat format = GfxFormat::R8G8B8A8,
			GfxUsage usage = GfxUsage::Default,
			ur_uint bindFlags = ur_uint(GfxBindFlag::ShaderResource),
			ur_uint accessFlags = ur_uint(0));

		inline const GfxTextureDesc& GetDesc() const;
	
	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		GfxTextureDesc desc;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics render target
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxRenderTarget : public GfxEntity
	{
	public:

		GfxRenderTarget(GfxSystem &gfxSystem);

		virtual ~GfxRenderTarget();

		Result Initialize(const GfxTextureDesc &desc, bool hasDepthStencil, GfxFormat dsFormat);

		Result Initialize(
			ur_uint width = 0,
			ur_uint height = 0,
			ur_uint levels = 1,
			GfxFormat format = GfxFormat::R8G8B8A8,
			bool hasDepthStencil = true, GfxFormat dsFormat = GfxFormat::R24G8);

		inline GfxTexture* GetTargetBuffer() const;

		inline GfxTexture* GetDepthStencilBuffer() const;

	protected:

		virtual Result InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc);

		virtual Result InitializeDepthStencil(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc);

		virtual Result OnInitialize();

	private:

		std::unique_ptr<GfxTexture> targetBuffer;
		std::unique_ptr<GfxTexture> depthStencilBuffer;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics render targets swap chain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSwapChain : public GfxEntity
	{
	public:

		GfxSwapChain(GfxSystem &gfxSystem);

		virtual ~GfxSwapChain();

		virtual Result Initialize(const GfxPresentParams &params);

		Result Initialize(
			ur_uint bufferWidth,
			ur_uint bufferHeight,
			ur_bool fullscreen = false,
			GfxFormat bufferFormat = GfxFormat::R8G8B8A8,
			ur_bool depthStencilEnabled = true,
			GfxFormat depthStencilFormat = GfxFormat::R24G8,
			ur_uint bufferCount = 2,
			ur_uint fullscreenRefreshRate = 59,
			ur_uint mutisampleCount = 1,
			ur_uint mutisampleQuality = 0);

		virtual Result Present();

		virtual GfxRenderTarget* GetTargetBuffer();
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics buffer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxBuffer : public GfxEntity
	{
	public:

		GfxBuffer(GfxSystem &gfxSystem);

		virtual ~GfxBuffer();

		Result Initialize(const GfxBufferDesc &desc, const GfxResourceData *data);

		Result Initialize(
			ur_uint sizeInBytes,
			GfxUsage usage = GfxUsage::Default,
			ur_uint bindFlags = ur_uint(0),
			ur_uint accessFlags = ur_uint(0),
			const GfxResourceData *data = ur_null);

		inline const GfxBufferDesc& GetDesc() const;

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		GfxBufferDesc desc;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics shader programm
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxShader : public GfxEntity
	{
	public:

		GfxShader(GfxSystem &gfxSystem);
		
		virtual ~GfxShader();

		Result Initialize(std::unique_ptr<ur_byte[]> &byteCode, ur_size sizeInBytes);

		inline const ur_byte* GetByteCode() const;

		inline ur_size GetSizeInBytes() const;

	protected:

		virtual Result OnInitialize();

	private:

		std::unique_ptr<ur_byte[]> byteCode;
		ur_size sizeInBytes;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics vertex shader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxVertexShader : public GfxShader
	{
	public:

		GfxVertexShader(GfxSystem &gfxSystem);

		virtual ~GfxVertexShader();

	protected:

		virtual Result OnInitialize();
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base graphics pixel shader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPixelShader : public GfxShader
	{
	public:

		GfxPixelShader(GfxSystem &gfxSystem);

		virtual ~GfxPixelShader();

	protected:

		virtual Result OnInitialize();
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx input layout
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GfxInputLayout : public GfxEntity
	{
	public:

		GfxInputLayout(GfxSystem &gfxSystem);

		virtual ~GfxInputLayout();

		Result Initialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count);

		inline const std::vector<GfxInputElement> GetElements() const;

	protected:

		virtual Result OnInitialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count);

	private:

		std::vector<GfxInputElement> elements;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx pipeline state full description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPipelineState : public GfxEntity
	{	
	public:

		GfxInputLayout *InputLayout;
		GfxVertexShader *VertexShader;
		GfxPixelShader *PixelShader;

	public:

		GfxPipelineState(GfxSystem &gfxSystem);

		virtual ~GfxPipelineState();

		Result SetRenderState(const GfxRenderState &renderState);

		inline const GfxRenderState& GetRenderState();

	protected:

		virtual Result OnSetRenderState(const GfxRenderState &renderState);

	private:

		GfxRenderState renderState;
	};

} // end namespace UnlimRealms

#include "Gfx/GfxSystem.inline.h"