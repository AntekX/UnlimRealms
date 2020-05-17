///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline IDXGIAdapter1* GrafSystemDX12::GetDXGIAdapter(ur_uint deviceId) const
	{
		return (deviceId < this->dxgiAdapters.size() ? this->dxgiAdapters[deviceId].get() : ur_null);
	}

	inline ID3D12Device5* GrafDeviceDX12::GetD3DDevice() const
	{
		return this->d3dDevice.get();
	}

	inline GrafDeviceDX12::CommandAllocator* GrafCommandListDX12::GetCommandAllocator() const
	{
		return this->commandAllocator;
	}

	inline ID3D12CommandList* GrafCommandListDX12::GetD3DCommandList() const
	{
		return this->d3dCommandList.get();
	}

	inline GrafBuffer* GrafAccelerationStructureDX12::GetScratchBuffer() const
	{
		return this->grafScratchBuffer.get();
	}

} // end namespace UnlimRealms