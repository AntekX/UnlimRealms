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
		return this->dxgiFactory;
	}

	inline ID3D12Device* GfxSystemD3D12::GetD3DDevice() const
	{
		return this->d3dDevice;
	}

	inline ID3D12CommandQueue* GfxSystemD3D12::GetD3DCommandQueue() const
	{
		return this->d3dCommandQueue;
	}

	inline ID3D12CommandAllocator* GfxSystemD3D12::GetD3DCommandAllocator() const
	{
		return this->d3dCommandAllocators[this->frameIndex];
	}

	inline GfxContextD3D12* GfxSystemD3D12::GetResourceContext() const
	{
		return this->resourceContext.get();
	}

	inline GfxSystemD3D12::DescriptorHeap* GfxSystemD3D12::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		return this->descriptorHeaps[ur_size(heapType)].get();
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

	inline ID3D12Resource* GfxResourceD3D12::GetD3DResource() const
	{
		return this->d3dResource;
	}

	inline D3D12_RESOURCE_STATES GfxResourceD3D12::GetD3DResourceState()
	{
		return this->d3dCurrentState;
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

} // end namespace UnlimRealms