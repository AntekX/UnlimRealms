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

	inline GfxSystemD3D12::DescriptorHeap* GfxSystemD3D12::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		return this->descriptorHeaps[ur_size(heapType)].get();
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE GfxSystemD3D12::Descriptor::CpuHandle() const
	{
		return this->cpuHandle;
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE GfxSystemD3D12::Descriptor::GpuHandle() const
	{
		return this->gpuHandle;
	}

	inline ID3D12Resource* GfxTextureD3D12::GetD3DResource() const
	{
		return this->d3dResource;
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

} // end namespace UnlimRealms