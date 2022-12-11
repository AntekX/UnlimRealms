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

	inline ur_size GrafDescriptorHeapDX12::GetDescriptorIncrementSize() const
	{
		return this->descriptorIncrementSize;
	}

	inline ur_bool GrafDescriptorHandleDX12::IsValid() const
	{
		return (this->allocation.Size > 0);
	}

	inline const GrafDescriptorHeapDX12* GrafDescriptorHandleDX12::GetHeap() const
	{
		return this->heap;
	}

	inline const Allocation& GrafDescriptorHandleDX12::GetAllocation() const
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

	inline const GrafDescriptorHandleDX12& GrafDescriptorTableDX12::GetSrvUavCbvDescriptorHeapHandle() const
	{
		return this->descriptorTableData[ShaderVisibleDescriptorHeap_SrvUavCbv].descriptorHeapHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafDescriptorTableDX12::GetSamplerDescriptorHeapHandle() const
	{
		return this->descriptorTableData[ShaderVisibleDescriptorHeap_Sampler].descriptorHeapHandle;
	}

	inline const D3D12_ROOT_PARAMETER& GrafDescriptorTableDX12::GetSrvUavCbvTableD3DRootParameter() const
	{
		return this->descriptorTableData[ShaderVisibleDescriptorHeap_SrvUavCbv].d3dRootParameter;
	}

	inline const D3D12_ROOT_PARAMETER& GrafDescriptorTableDX12::GetSamplerTableD3DRootParameter() const
	{
		return this->descriptorTableData[ShaderVisibleDescriptorHeap_Sampler].d3dRootParameter;
	}

	inline const Result GrafDescriptorTableDX12::GetD3DDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE& d3dHandle,
		ur_uint bindingIdx, GrafDescriptorHandleDX12& tableHandle,
		D3D12_DESCRIPTOR_RANGE_TYPE d3dRangeType) const
	{
		// get heap related table data
		const DescriptorTableData* descriptorTable = nullptr;
		if (D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER == d3dRangeType)
		{
			descriptorTable = &this->descriptorTableData[ShaderVisibleDescriptorHeap_Sampler];
		}
		else
		{
			descriptorTable = &this->descriptorTableData[ShaderVisibleDescriptorHeap_SrvUavCbv];
		}

		// find corresponding range
		const D3D12_DESCRIPTOR_RANGE* bindingDescriptorRange = nullptr;
		for (const D3D12_DESCRIPTOR_RANGE& d3dDescriptorRange : descriptorTable->d3dDescriptorRanges)
		{
			if (d3dDescriptorRange.RangeType == d3dRangeType &&
				d3dDescriptorRange.BaseShaderRegister <= bindingIdx &&
				d3dDescriptorRange.BaseShaderRegister + d3dDescriptorRange.NumDescriptors > bindingIdx)
			{
				bindingDescriptorRange = &d3dDescriptorRange;
				break;
			}
		}
		if (nullptr == bindingDescriptorRange)
			return Result(NotFound);

		// compute descriptor handle
		UINT bindingOffsetInTable = ((UINT)bindingIdx - bindingDescriptorRange->BaseShaderRegister) + bindingDescriptorRange->OffsetInDescriptorsFromTableStart;
		SIZE_T bindingOffsetHeapSize = (SIZE_T)bindingOffsetInTable * (SIZE_T)tableHandle.GetHeap()->GetDescriptorIncrementSize();
		d3dHandle.ptr = tableHandle.GetD3DHandleCPU().ptr + bindingOffsetHeapSize;

		return Result(Success);
	}

	inline const GrafDescriptorHandleDX12& GrafSamplerDX12::GetDescriptorHandle() const
	{
		return this->samplerDescriptorHandle;
	}

	inline const void* GrafShaderDX12::GetByteCodePtr() const
	{
		return this->byteCodeBuffer.get();
	}

	inline ur_size GrafShaderDX12::GetByteCodeSize() const
	{
		return this->byteCodeSize;
	}

	inline GrafBuffer* GrafAccelerationStructureDX12::GetScratchBuffer() const
	{
		return this->grafScratchBuffer.get();
	}

} // end namespace UnlimRealms