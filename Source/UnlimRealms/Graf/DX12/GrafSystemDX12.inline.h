///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline IDXGIFactory5* GrafSystemDX12::GetDXGIFactory() const
	{
		return this->dxgiFactory.get();
	}

	inline IDXGIAdapter1* GrafSystemDX12::GetDXGIAdapter(ur_uint deviceId) const
	{
		return (deviceId < this->dxgiAdapters.size() ? this->dxgiAdapters[deviceId].get() : ur_null);
	}

	inline ur_bool GrafDescriptorHandleDX12::IsValid() const
	{
		return (this->allocation.Size > 0);
	}

	inline const Allocation& GrafDescriptorHandleDX12::GetAllocation()
	{
		return this->allocation;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE GrafDescriptorHandleDX12::GetD3DHandleCPU() const
	{
		return this->cpuHandle;
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE GrafDescriptorHandleDX12::GetD3DHandleGPU() const
	{
		return this->gpuHandle;
	}

	inline ID3D12Device5* GrafDeviceDX12::GetD3DDevice() const
	{
		return this->d3dDevice.get();
	}

	inline ID3D12CommandQueue* GrafDeviceDX12::GetD3DGraphicsCommandQueue() const
	{
		return (this->graphicsQueue.get() ? this->graphicsQueue.get()->d3dQueue.get() : nullptr);
	}

	inline ID3D12CommandQueue* GrafDeviceDX12::GetD3DComputeCommandQueue() const
	{
		return (this->computeQueue.get() ? this->computeQueue.get()->d3dQueue.get() : nullptr);
	}

	inline ID3D12CommandQueue* GrafDeviceDX12::GetD3DTransferCommandQueue() const
	{
		return (this->transferQueue.get() ? this->transferQueue.get()->d3dQueue.get() : nullptr);
	}

	inline GrafDeviceDX12::CommandAllocator* GrafCommandListDX12::GetCommandAllocator() const
	{
		return this->commandAllocator;
	}

	inline ID3D12GraphicsCommandList1* GrafCommandListDX12::GetD3DCommandList() const
	{
		return this->d3dCommandList.get();
	}

	inline ID3D12Resource* GrafImageDX12::GetD3DResource() const
	{
		return this->d3dResource.get();
	}

	inline GrafImageSubresource* GrafImageDX12::GetDefaultSubresource() const
	{
		return this->grafDefaultSubresource.get();
	}

	inline void GrafImageDX12::SetState(GrafImageState& state)
	{
		this->imageState = state;
	}

	inline ID3D12Resource* GrafBufferDX12::GetD3DResource() const
	{
		return this->d3dResource.get();
	}

	inline const GrafDescriptorHandleDX12& GrafBufferDX12::GetCBVDescriptorHandle() const
	{
		return this->cbvDescriptorHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafBufferDX12::GetSRVDescriptorHandle() const
	{
		return this->srvDescriptorHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafBufferDX12::GetUAVDescriptorHandle() const
	{
		return this->uavDescriptorHandle;
	}

	inline const D3D12_VERTEX_BUFFER_VIEW* GrafBufferDX12::GetD3DVertexBufferView() const
	{
		return &this->d3dVBView;
	}

	inline const D3D12_INDEX_BUFFER_VIEW* GrafBufferDX12::GetD3DIndexBufferView() const
	{
		return &this->d3dIBView;
	}

	inline const D3D12_ROOT_PARAMETER* GrafDescriptorTableDX12::GetD3DRootParameter() const
	{
		return &this->d3dRootParameter;
	}

	inline GrafBuffer* GrafAccelerationStructureDX12::GetScratchBuffer() const
	{
		return this->grafScratchBuffer.get();
	}

} // end namespace UnlimRealms