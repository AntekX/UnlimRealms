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

	inline ID3D12CommandList* GrafCommandListDX12::GetD3DCommandList() const
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

	inline GrafBuffer* GrafAccelerationStructureDX12::GetScratchBuffer() const
	{
		return this->grafScratchBuffer.get();
	}

} // end namespace UnlimRealms