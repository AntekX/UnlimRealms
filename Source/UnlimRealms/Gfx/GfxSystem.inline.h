///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{
	
	inline const ur_uint GfxSystem::GetAdaptersCount() const
	{
		return (ur_uint)this->gfxAdapters.size();
	}

	inline const GfxAdapterDesc& GfxSystem::GetAdapterDesc(ur_uint idx) const
	{
		return this->gfxAdapters[idx];
	}

	inline const GfxAdapterDesc& GfxSystem::GetActiveAdapterDesc() const
	{
		return this->gfxAdapters[this->gfxAdapterIdx];
	}

	inline ur_uint GfxSystem::GetActiveAdapterIdx() const
	{
		return this->gfxAdapterIdx;
	}

	inline GfxSystem& GfxEntity::GetGfxSystem()
	{
		return this->gfxSystem;
	}

	inline const GfxTextureDesc& GfxTexture::GetDesc() const
	{
		return this->desc;
	}

	inline GfxTexture* GfxRenderTarget::GetTragetBuffer() const
	{
		return this->targetBuffer.get();
	}

	inline GfxTexture* GfxRenderTarget::GetDepthStencilBuffer() const
	{
		return this->depthStencilBuffer.get();
	}

	inline const GfxBufferDesc& GfxBuffer::GetDesc() const
	{
		return this->desc;
	}

	inline const ur_byte* GfxShader::GetByteCode() const
	{
		return this->byteCode.get();
	}

	inline ur_size GfxShader::GetSizeInBytes() const
	{
		return this->sizeInBytes;
	}

	inline const std::vector<GfxInputElement> GfxInputLayout::GetElements() const
	{
		return this->elements;
	}

	inline const GfxRenderState& GfxPipelineState::GetRenderState()
	{
		return this->renderState;
	}

} // end namespace UnlimRealms