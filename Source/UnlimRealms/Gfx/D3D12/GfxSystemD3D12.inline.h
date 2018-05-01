///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{	

	inline ur_bool GfxSystemD3D12::IsFrameComplete(ur_uint frameIndex)
	{
		return !(this->frameFenceValues[this->frameIndex] > this->d3dFrameFence->GetCompletedValue());
	}

	inline ur_bool GfxSystemD3D12::IsCurrentFrameComplete()
	{
		return this->IsFrameComplete(this->frameIndex);
	}

	inline ur_uint GfxSystemD3D12::CurrentFrameIndex() const
	{
		//return this->frameIndex;
		return this->frameFenceValues[this->frameIndex];
	}

	inline WinCanvas* GfxSystemD3D12::GetWinCanvas() const
	{
		return this->winCanvas;
	}

	inline IDXGIFactory4* GfxSystemD3D12::GetDXGIFactory() const
	{
		return this->dxgiFactory.get();
	}

	inline ID3D12Device* GfxSystemD3D12::GetD3DDevice() const
	{
		return this->d3dDevice.get();
	}

	inline ID3D12CommandQueue* GfxSystemD3D12::GetD3DCommandQueue() const
	{
		return this->d3dCommandQueue.get();
	}

	inline ID3D12CommandAllocator* GfxSystemD3D12::GetD3DCommandAllocator() const
	{
		return this->d3dCommandAllocators[this->frameIndex].get();
	}

	inline GfxSystemD3D12::DescriptorHeap* GfxSystemD3D12::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		return this->descriptorHeaps[ur_size(heapType)].get();
	}

	inline ID3D12DescriptorHeap* GfxSystemD3D12::DescriptorHeap::GetD3DDescriptorHeap(DescriptorSet& descriptorSet)
	{
		if (descriptorSet.heap != this)
			return ur_null;
		ur_size pageIdx = descriptorSet.pagePoolPos / DescriptorsPerHeap;
		return this->pagePool[descriptorSet.shaderVisible][pageIdx]->d3dHeap;
	}

	inline ur_size GfxSystemD3D12::DescriptorSet::GetDescriptorCount() const
	{
		return this->descriptorCount;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE GfxSystemD3D12::DescriptorSet::FirstCpuHandle() const
	{
		return this->firstCpuHandle;
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE GfxSystemD3D12::DescriptorSet::FirstGpuHandle() const
	{
		return this->firstGpuHandle;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE GfxSystemD3D12::Descriptor::CpuHandle() const
	{
		return this->FirstCpuHandle();
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE GfxSystemD3D12::Descriptor::GpuHandle() const
	{
		return this->FirstGpuHandle();
	}

	inline ID3D12GraphicsCommandList* GfxContextD3D12::GetD3DCommandList() const
	{
		return this->d3dCommandList.get();
	}

	inline ID3D12Resource* GfxResourceD3D12::GetD3DResource() const
	{
		return this->d3dResource.get();
	}

	inline D3D12_RESOURCE_STATES GfxResourceD3D12::GetD3DResourceState()
	{
		return this->d3dCurrentState;
	}

	inline GfxResourceD3D12& GfxBufferD3D12::GetResource()
	{
		return this->resource;
	}

	inline const D3D12_VERTEX_BUFFER_VIEW& GfxBufferD3D12::GetD3DViewVB() const
	{
		return this->d3dVBView;
	}

	inline const D3D12_INDEX_BUFFER_VIEW& GfxBufferD3D12::GetD3DViewIB() const
	{
		return this->d3dIBView;
	}

	inline const D3D12_CONSTANT_BUFFER_VIEW_DESC& GfxBufferD3D12::GetD3DViewCB() const
	{
		return this->d3dCBView;
	}

	inline GfxSystemD3D12::Descriptor* GfxBufferD3D12::GetDescriptor() const
	{
		return this->descriptor.get();
	}

	inline GfxSystemD3D12::Descriptor* GfxSamplerD3D12::GetDescriptor() const
	{
		return this->descriptor.get();
	}

	inline GfxResourceD3D12& GfxTextureD3D12::GetResource()
	{
		return this->resource;
	}

	inline GfxSystemD3D12::Descriptor* GfxTextureD3D12::GetSRVDescriptor() const
	{
		return this->srvDescriptor.get();
	}

	inline GfxSystemD3D12::Descriptor* GfxRenderTargetD3D12::GetRTVDescriptor() const
	{
		return this->rtvDescriptor.get();
	}

	inline GfxSystemD3D12::Descriptor* GfxRenderTargetD3D12::GetDSVDescriptor() const
	{
		return this->dsvDescriptor.get();
	}

	inline IDXGISwapChain3* GfxSwapChainD3D12::GetDXGISwapChain() const
	{
		return this->dxgiSwapChain;
	}

	inline ID3D12PipelineState* GfxPipelineStateObjectD3D12::GetD3DPipelineState() const
	{
		return this->d3dPipelineState.get();
	}

	inline D3D12_PRIMITIVE_TOPOLOGY GfxPipelineStateObjectD3D12::GetD3DPrimitiveTopology() const
	{
		return this->d3dPrimitiveTopology;
	}

	inline ID3D12RootSignature* GfxResourceBindingD3D12::GetD3DRootSignature() const
	{
		return this->d3dRootSignature.get();
	}

	template <typename TGfxResource, D3D12_DESCRIPTOR_RANGE_TYPE d3dDescriptorRangeType>
	void GfxResourceBindingD3D12::InitializeRanges(
		const GfxResourceBinding::ResourceRange<TGfxResource>& resourceRange,
		std::vector<D3D12_DESCRIPTOR_RANGE>& d3dDescriptorRanges)
	{
		if (!resourceRange.IsValid())
			return;

		// calculate ranges
		ur_uint baseRegister = ur_uint(-2);
		for (ur_uint slotIdx = resourceRange.slotFrom; slotIdx <= resourceRange.slotTo; ++slotIdx)
		{
			if (State::Unused == resourceRange.slots[slotIdx - resourceRange.slotFrom].state)
				continue;

			if (slotIdx - baseRegister > 1)
			{
				// start new range
				baseRegister = slotIdx;
				D3D12_DESCRIPTOR_RANGE range = CD3DX12_DESCRIPTOR_RANGE(d3dDescriptorRangeType, 0, UINT(baseRegister));
				d3dDescriptorRanges.emplace_back(range);
			}
			++d3dDescriptorRanges.back().NumDescriptors;
		}
	}

} // end namespace UnlimRealms