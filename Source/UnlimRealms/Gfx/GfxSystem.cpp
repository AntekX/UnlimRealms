///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Gfx/GfxSystem.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSystem
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSystem::GfxSystem(Realm &realm) :
		RealmEntity(realm)
	{
		this->gfxAdapterIdx = 0;
		LogNote("GfxSystem: created");
	}

	GfxSystem::~GfxSystem()
	{
		LogNote("GfxSystem: destroyed");
	}

	Result GfxSystem::Initialize(Canvas *canvas)
	{
		return Result(NotImplemented);
	}

	Result GfxSystem::Render()
	{
		return Result(NotImplemented);
	}

	Result GfxSystem::CreateContext(std::unique_ptr<GfxContext> &gfxContext)
	{
		gfxContext.reset(new GfxContext(*this));
		return Result(Success);
	}

	Result GfxSystem::CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture)
	{
		gfxTexture.reset(new GfxTexture(*this));
		return Result(Success);
	}

	Result GfxSystem::CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT)
	{
		gfxRT.reset(new GfxRenderTarget(*this));
		return Result(Success);
	}

	Result GfxSystem::CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain)
	{
		gfxSwapChain.reset(new GfxSwapChain(*this));
		return Result(Success);
	}

	Result GfxSystem::CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer)
	{
		gfxBuffer.reset(new GfxBuffer(*this));
		return Result(Success);
	}

	Result GfxSystem::CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader)
	{
		gfxVertexShader.reset(new GfxVertexShader(*this));
		return Result(Success);
	}

	Result GfxSystem::CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader)
	{
		gfxPixelShader.reset(new GfxPixelShader(*this));
		return Result(Success);
	}

	Result GfxSystem::CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout)
	{
		gfxInputLayout.reset(new GfxInputLayout(*this));
		return Result(Success);
	}

	Result GfxSystem::CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState)
	{
		gfxPipelineState.reset(new GfxPipelineState(*this));
		return Result(Success);
	}

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxEntity
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxEntity::GfxEntity(GfxSystem &gfxSystem) :
		RealmEntity(gfxSystem.GetRealm()),
		gfxSystem(gfxSystem)
	{
	}

	GfxEntity::~GfxEntity()
	{
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxContext
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxContext::GfxContext(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{

	}

	GfxContext::~GfxContext()
	{

	}

	Result GfxContext::Initialize()
	{
		return Result(NotImplemented);
	}

	Result GfxContext::Begin()
	{
		return Result(NotImplemented);
	}

	Result GfxContext::End()
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetRenderTarget(GfxRenderTarget *rt)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetViewPort(const GfxViewPort *viewPort)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::GetViewPort(GfxViewPort &viewPort)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetScissorRect(const RectI *rect)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::ClearTarget(GfxRenderTarget *rt,
		bool clearColor, const ur_float4 &color,
		bool clearDepth, const ur_float &depth,
		bool clearStencil, const ur_uint &stencil)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetPipelineState(GfxPipelineState *state)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetTexture(GfxTexture *texture, ur_uint slot)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetConstantBuffer(GfxBuffer *buffer, ur_uint slot)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetVertexBuffer(GfxBuffer *buffer, ur_uint slot, ur_uint stride, ur_uint offset)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetIndexBuffer(GfxBuffer *buffer, ur_uint bitsPerIndex, ur_uint offset)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetVertexShader(GfxVertexShader *shader)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::SetPixelShader(GfxPixelShader *shader)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return Result(NotImplemented);
	}

	Result GfxContext::UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, const GfxResourceData *srcData, ur_uint offset, ur_uint size)
	{
		std::function<Result(GfxResourceData*)> copyFunc = [&](GfxResourceData *dstData) -> Result {
			if (ur_null == buffer || ur_null == dstData || ur_null == srcData)
				return ResultError(InvalidArgs, "GfxContext::UpdateBuffer: failed, invalid args");
			
			ur_uint copySize = (size > 0 ? size : buffer->GetDesc().Size);
			if (offset + copySize > buffer->GetDesc().Size)
				return ResultError(InvalidArgs, "GfxContext::UpdateBuffer: failed to copy data, invalid offset/size");
			
			memcpy((ur_byte*)dstData->Ptr + offset, srcData->Ptr, copySize);
			
			return Result(Success);
		};

		return this->UpdateBuffer(buffer, gpuAccess, doNotWait, copyFunc);
	}

	Result GfxContext::UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback)
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxTexture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	
	GfxTexture::GfxTexture(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
		memset(&this->desc, 0, sizeof(this->desc));
	}

	GfxTexture::~GfxTexture()
	{

	}

	Result GfxTexture::Initialize(const GfxTextureDesc &desc, const GfxResourceData *data)
	{
		this->desc = desc;

		return this->OnInitialize(data);
	}

	Result GfxTexture::Initialize(
		ur_uint width,
		ur_uint height,
		ur_uint levels,
		GfxFormat format,
		GfxUsage usage,
		ur_uint bindFlags,
		ur_uint accessFlags)
	{
		GfxTextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.Levels = levels;
		desc.Format = format;
		desc.FormatView = GfxFormatView::Unorm;
		desc.Usage = usage;
		desc.BindFlags = bindFlags;
		desc.AccessFlags = accessFlags;

		return this->Initialize(desc, ur_null);
	}

	Result GfxTexture::OnInitialize(const GfxResourceData *data)
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxRenderTarget
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxRenderTarget::GfxRenderTarget(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
	}

	GfxRenderTarget::~GfxRenderTarget()
	{
	}

	Result GfxRenderTarget::Initialize(const GfxTextureDesc &desc, bool hasDepthStencil, GfxFormat dsFormat)
	{
		std::unique_ptr<GfxTexture> newTargetBuffer;
		std::unique_ptr<GfxTexture> newDepthStencilBuffer;

		Result res = this->InitializeTargetBuffer(newTargetBuffer, desc);
		if (Failed(res))
			return ResultError(res.Code, "GfxRenderTarget: failed to initialize render target buffer");

		if (hasDepthStencil)
		{
			GfxTextureDesc dsDesc = desc;
			dsDesc.BindFlags = ur_uint(GfxBindFlag::DepthStencil) | ur_uint(GfxBindFlag::ShaderResource);
			dsDesc.Format = dsFormat;
			dsDesc.FormatView = GfxFormatView::Unorm;
			
			res = this->InitializeDepthStencil(newDepthStencilBuffer, dsDesc);
			if (Failed(res))
				return ResultError(res.Code, "GfxRenderTarget: failed to initialize depth stencil buffer");
		}

		this->targetBuffer = std::move(newTargetBuffer);
		this->depthStencilBuffer = std::move(newDepthStencilBuffer);

		return this->OnInitialize();
	}

	Result GfxRenderTarget::Initialize(
		ur_uint width,
		ur_uint height,
		ur_uint levels,
		GfxFormat format,
		bool hasDepthStencil, GfxFormat dsFormat)
	{
		GfxTextureDesc desc;
		desc.Width = width;
		desc.Height = height;
		desc.Levels = levels;
		desc.Format = format;
		desc.FormatView = GfxFormatView::Unorm;
		desc.Usage = GfxUsage::Default;
		desc.BindFlags = ur_uint(GfxBindFlag::RenderTarget) | ur_uint(GfxBindFlag::ShaderResource);
		desc.AccessFlags = ur_uint(0);

		return this->Initialize(desc, hasDepthStencil, dsFormat);
	}

	Result GfxRenderTarget::InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc)
	{
		Result res = this->GetGfxSystem().CreateTexture(resultBuffer);
		if (Succeeded(res))
		{
			res = resultBuffer->Initialize(desc, ur_null);
		}
		return res;
	}

	Result GfxRenderTarget::InitializeDepthStencil(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc)
	{
		Result res = this->GetGfxSystem().CreateTexture(resultBuffer);
		if (Succeeded(res))
		{
			res = resultBuffer->Initialize(desc, ur_null);
		}
		return res;
	}

	Result GfxRenderTarget::OnInitialize()
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSwapChain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSwapChain::GfxSwapChain(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
	}

	GfxSwapChain::~GfxSwapChain()
	{
	}

	Result GfxSwapChain::Initialize(const GfxPresentParams &params)
	{
		return Result(NotImplemented);
	}

	Result GfxSwapChain::Initialize(
		ur_uint bufferWidth,
		ur_uint bufferHeight,
		ur_bool fullscreen,
		GfxFormat bufferFormat,
		ur_bool depthStencilEnabled,
		GfxFormat depthStencilFormat,
		ur_uint bufferCount,
		ur_uint fullscreenRefreshRate,
		ur_uint mutisampleCount,
		ur_uint mutisampleQuality)
	{
		GfxPresentParams params;
		params.BufferWidth = bufferWidth;
		params.BufferHeight = bufferHeight;
		params.BufferCount = bufferCount;
		params.BufferFormat = bufferFormat;
		params.Fullscreen = fullscreen;
		params.FullscreenRefreshRate = fullscreenRefreshRate;
		params.DepthStencilEnabled = depthStencilEnabled;
		params.DepthStencilFormat = depthStencilFormat;
		params.MutisampleCount = mutisampleCount;
		params.MutisampleQuality = mutisampleQuality;

		return this->Initialize(params);
	}

	Result GfxSwapChain::Present()
	{
		return Result(NotImplemented);
	}

	GfxRenderTarget* GfxSwapChain::GetTargetBuffer()
	{
		return ur_null;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxBuffer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxBuffer::GfxBuffer(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
	}

	GfxBuffer::~GfxBuffer()
	{
	}

	Result GfxBuffer::Initialize(const GfxBufferDesc &desc, const GfxResourceData *data)
	{
		this->desc = desc;

		return this->OnInitialize(data);
	}

	Result GfxBuffer::Initialize(
		ur_uint sizeInBytes,
		GfxUsage usage,
		ur_uint bindFlags,
		ur_uint accessFlags,
		const GfxResourceData *data)
	{
		GfxBufferDesc desc;
		desc.Size = sizeInBytes;
		desc.Usage = usage;
		desc.BindFlags = bindFlags;
		desc.AccessFlags = accessFlags;
		desc.StructureStride = 0;

		return this->Initialize(desc, data);
	}

	Result GfxBuffer::OnInitialize(const GfxResourceData *data)
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxShader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxShader::GfxShader(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
		this->sizeInBytes = 0;
	}

	GfxShader::~GfxShader()
	{
	}

	Result GfxShader::Initialize(std::unique_ptr<ur_byte[]> &byteCode, ur_size sizeInBytes)
	{
		this->byteCode = std::move(byteCode);
		this->sizeInBytes = sizeInBytes;

		if (ur_null == this->byteCode || 0 == this->sizeInBytes)
			return Result(InvalidArgs);

		return this->OnInitialize();
	}

	Result GfxShader::OnInitialize()
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxVertexShader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxVertexShader::GfxVertexShader(GfxSystem &gfxSystem) :
		GfxShader(gfxSystem)
	{
	}

	GfxVertexShader::~GfxVertexShader()
	{
	}

	Result GfxVertexShader::OnInitialize()
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPixelShader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxPixelShader::GfxPixelShader(GfxSystem &gfxSystem) :
		GfxShader(gfxSystem)
	{
	}

	GfxPixelShader::~GfxPixelShader()
	{
	}

	Result GfxPixelShader::OnInitialize()
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxInputLayout
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxInputLayout::GfxInputLayout(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
	}

	GfxInputLayout::~GfxInputLayout()
	{
	}

	Result GfxInputLayout::Initialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count)
	{
		this->elements.resize(count);

		ur_size size = sizeof(GfxInputElement) * count;
		if (ur_null == elements || 0 == size)
			return Result(InvalidArgs);

		memcpy(this->elements.data(), elements, size);

		return this->OnInitialize(shader, elements, count);
	}

	Result GfxInputLayout::OnInitialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count)
	{
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPipelineState
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxPipelineState::GfxPipelineState(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem),
		PrimitiveTopology(GfxPrimitiveTopology::TriangleList),
		StencilRef(0x0),
		InputLayout(ur_null),
		VertexShader(ur_null),
		PixelShader(ur_null)
	{
	}

	GfxPipelineState::~GfxPipelineState()
	{
	}

	Result GfxPipelineState::SetRenderState(const GfxRenderState &renderState)
	{
		this->renderState = renderState;
		return this->OnSetRenderState(renderState);
	}

	Result GfxPipelineState::OnSetRenderState(const GfxRenderState &renderState)
	{
		return Result(Success);
	}

} // end namespace UnlimRealms