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

	inline const GfxResourceBinding* GfxPipelineStateObject::GetResourceBinding() const
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

	template <typename TResource>
	GfxResourceBinding::ResourceRange<TResource>::ResourceRange()
	{
		this->slotFrom = ur_uint(-1);
		this->slotTo = 0;
		this->rangeState = State::Unused;
		this->commonSlotsState = State::Unused;
		this->slots.reserve(RespurceRangeReserveSize);
	}

	template <typename TResource>
	Result GfxResourceBinding::ResourceRange<TResource>::SetResource(ur_uint slot, TResource* resource)
	{
		ur_uint setSlotFrom = std::min(slot, this->slotFrom);
		ur_uint setSlotTo = std::max(slot, this->slotTo);
		ur_uint rangeSize = setSlotTo - setSlotFrom + 1;
		if (setSlotFrom < this->slotFrom)
		{
			if (this->IsValid())
			{
				std::vector<Slot> crntSlots;
				crntSlots.resize(this->slots.size());
				this->slots.resize(rangeSize, { State::Unused , ur_null });
				memcpy(this->slots.data() + (this->slotFrom - setSlotFrom), crntSlots.data(), sizeof(Slot) * crntSlots.size());
			}
			this->slotFrom = setSlotFrom;
			this->rangeState = State::UsedModified;
		}
		if (setSlotTo > this->slotTo)
		{
			this->slotTo = setSlotTo;
			this->rangeState = State::UsedModified;
		}
		if (rangeSize > this->slots.size())
		{
			this->slots.resize(rangeSize, { State::Unused , ur_null });
		}

		Slot& slotEntry = this->slots[slot - this->slotFrom];
		if (State::Unused == slotEntry.state) this->rangeState = State::UsedModified;
		slotEntry.state = State::UsedModified;
		slotEntry.resource = resource;

		this->commonSlotsState = State::UsedModified;

		return Success;
	}

	template <typename TResource>
	void GfxResourceBinding::ResourceRange<TResource>::OnInitialized()
	{
		if (this->rangeState != State::Unused)
		{
			this->rangeState = State::UsedUnmodified;
		}
		if (this->commonSlotsState != State::Unused)
		{
			this->commonSlotsState = State::UsedUnmodified;
			ur_uint usedRangeSize = this->slotTo - this->slotFrom + 1;
			for (ur_uint i = 0; i < usedRangeSize; ++i)
			{
				if (this->slots[i].state != State::Unused)
					this->slots[i].state = State::UsedUnmodified;
			}
		}
	}

	template <typename TResource>
	inline ur_bool GfxResourceBinding::ResourceRange<TResource>::IsValid() const
	{
		return (this->slotTo >= this->slotFrom);
	}

	inline const GfxResourceBinding::ResourceRange<GfxBuffer>& GfxResourceBinding::GetBufferRange() const
	{
		return this->bufferRange;
	}

	inline const GfxResourceBinding::ResourceRange<GfxTexture>& GfxResourceBinding::GetTextureRange() const
	{
		return this->textureRange;
	}

	inline const GfxResourceBinding::ResourceRange<GfxSamplerState>& GfxResourceBinding::GetSamplerRange() const
	{
		return this->samplerRange;
	}

} // end namespace UnlimRealms