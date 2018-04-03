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

	inline GfxTexture* GfxRenderTarget::GetTargetBuffer() const
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

	inline const GfxResourceBinding* GfxPipelineStateObject::GetBinding() const
	{
		return this->binding;
	}

	inline const GfxBlendState& GfxPipelineStateObject::GetBlendState(ur_uint rtIndex) const
	{
		return this->blendState[rtIndex];
	}

	inline const GfxRasterizerState& GfxPipelineStateObject::GetRasterizerState() const
	{
		return this->rasterizerState;
	}

	inline const GfxDepthStencilState& GfxPipelineStateObject::GetDepthStencilState() const
	{
		return this->depthStencilState;
	}

	inline const GfxPrimitiveTopology& GfxPipelineStateObject::GetPrimitiveTopology() const
	{
		return this->primitiveTopology;
	}

	inline const ur_uint GfxPipelineStateObject::GetRenderTargetsNumber() const
	{
		return this->numRenderTargets;
	}

	inline const GfxFormat GfxPipelineStateObject::GetRenderTargetFormat(ur_uint rtIndex) const
	{
		return this->renderTargetFormats[rtIndex];
	}

	inline const GfxFormat GfxPipelineStateObject::GetDepthStencilFormat() const
	{
		return this->depthStencilFormat;
	}

	inline const ur_uint GfxPipelineStateObject::GetStencilRef() const
	{
		return this->stencilRef;
	}

	inline const GfxInputLayout* GfxPipelineStateObject::GetInputLayout() const
	{
		return this->inputLayout;
	}

	inline const GfxVertexShader* GfxPipelineStateObject::GetVertexShader() const
	{
		return this->vertexShader;
	}

	inline const GfxPixelShader* GfxPipelineStateObject::GetPixelShader() const
	{
		return this->pixelShader;
	}

	inline const std::vector<std::pair<ur_uint, GfxBuffer*>>& GfxResourceBinding::GetBuffers() const
	{
		return this->buffers;
	}

	inline const std::vector<std::pair<ur_uint, GfxTexture*>>& GfxResourceBinding::GetTextures() const
	{
		return this->textures;
	}

	inline const std::vector<std::pair<ur_uint, GfxSamplerState*>>& GfxResourceBinding::GetSamplers() const
	{
		return this->samplers;
	}

} // end namespace UnlimRealms