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

	inline ur_bool GrafDescriptorHeapDX12::GetIsShaderVisible() const
	{
		return this->isShaderVisible;
	}

	inline ur_size GrafDescriptorHeapDX12::GetDescriptorIncrementSize() const
	{
		return this->descriptorIncrementSize;
	}

	inline ID3D12DescriptorHeap* GrafDescriptorHeapDX12::GetD3DDescriptorHeap() const
	{
		return this->d3dDescriptorHeap.get();
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

	inline ID3D12GraphicsCommandList4* GrafCommandListDX12::GetD3DCommandList() const
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

	inline void GrafImageDX12::SetState(GrafImageState state)
	{
		this->imageState = state;
	}

	inline void GrafImageSubresourceDX12::SetState(GrafImageState state)
	{
		this->subresourceState = state;
	}

	inline const GrafDescriptorHandleDX12& GrafImageSubresourceDX12::GetRTVDescriptorHandle() const
	{
		return this->rtvDescriptorHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafImageSubresourceDX12::GetDSVDescriptorHandle() const
	{
		return this->dsvDescriptorHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafImageSubresourceDX12::GetSRVDescriptorHandle() const
	{
		return this->srvDescriptorHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafImageSubresourceDX12::GetUAVDescriptorHandle() const
	{
		return this->uavDescriptorHandle;
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

	inline void GrafBufferDX12::SetState(GrafBufferState& state)
	{
		this->bufferState = state;
	}

	inline const GrafDescriptorHandleDX12& GrafDescriptorTableDX12::GetSrvUavCbvDescriptorHeapHandle() const
	{
		return this->descriptorTableData[GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv].descriptorHeapHandle;
	}

	inline const GrafDescriptorHandleDX12& GrafDescriptorTableDX12::GetSamplerDescriptorHeapHandle() const
	{
		return this->descriptorTableData[GrafShaderVisibleDescriptorHeapTypeDX12_Sampler].descriptorHeapHandle;
	}

	inline const D3D12_ROOT_PARAMETER& GrafDescriptorTableLayoutDX12::GetSrvUavCbvTableD3DRootParameter() const
	{
		return this->descriptorTableDesc[GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv].d3dRootParameter;
	}

	inline const D3D12_ROOT_PARAMETER& GrafDescriptorTableLayoutDX12::GetSamplerTableD3DRootParameter() const
	{
		return this->descriptorTableDesc[GrafShaderVisibleDescriptorHeapTypeDX12_Sampler].d3dRootParameter;
	}

	inline const GrafDescriptorTableLayoutDX12::DescriptorTableDesc& GrafDescriptorTableLayoutDX12::GetTableDescForHeapType(GrafShaderVisibleDescriptorHeapTypeDX12 heapType)
	{
		return this->descriptorTableDesc[heapType];
	}

	inline const Result GrafDescriptorTableDX12::GetD3DDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE& d3dHandle, ur_uint bindingIdx, D3D12_DESCRIPTOR_RANGE_TYPE d3dRangeType) const
	{
		GrafDescriptorTableLayoutDX12* descriptorTableLayoutDX12 = static_cast<GrafDescriptorTableLayoutDX12*>(this->GetLayout());

		// get heap related table data
		const GrafDescriptorTableLayoutDX12::DescriptorTableDesc* tableDesc = nullptr;
		const DescriptorTableData* tableData = nullptr;
		if (D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER == d3dRangeType)
		{
			tableDesc = &descriptorTableLayoutDX12->GetTableDescForHeapType(GrafShaderVisibleDescriptorHeapTypeDX12_Sampler);
			tableData = &this->descriptorTableData[GrafShaderVisibleDescriptorHeapTypeDX12_Sampler];
		}
		else
		{
			tableDesc = &descriptorTableLayoutDX12->GetTableDescForHeapType(GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv);
			tableData = &this->descriptorTableData[GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv];
		}
		if (nullptr == tableDesc || nullptr == tableData)
			return Result(NotFound);

		// find corresponding range
		const D3D12_DESCRIPTOR_RANGE* bindingDescriptorRange = nullptr;
		for (const D3D12_DESCRIPTOR_RANGE& d3dDescriptorRange : tableDesc->d3dDescriptorRanges)
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
		SIZE_T bindingOffsetHeapSize = (SIZE_T)bindingOffsetInTable * (SIZE_T)tableData->descriptorHeapHandle.GetHeap()->GetDescriptorIncrementSize();
		d3dHandle.ptr = tableData->descriptorHeapHandle.GetD3DHandleCPU().ptr + bindingOffsetHeapSize;

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

	inline ID3D12PipelineState* GrafPipelineDX12::GetD3DPipelineState() const
	{
		return this->d3dPipelineState.get();
	}

	inline ID3D12RootSignature* GrafPipelineDX12::GetD3DRootSignature() const
	{
		return this->d3dRootSignature.get();
	}

	inline D3D12_PRIMITIVE_TOPOLOGY GrafPipelineDX12::GetD3DPrimitiveTopology() const
	{
		return this->d3dPrimitiveTopology;
	}

	inline ID3D12PipelineState* GrafComputePipelineDX12::GetD3DPipelineState() const
	{
		return this->d3dPipelineState.get();
	}

	inline ID3D12RootSignature* GrafComputePipelineDX12::GetD3DRootSignature() const
	{
		return this->d3dRootSignature.get();
	}

	inline ID3D12StateObject* GrafRayTracingPipelineDX12::GetD3DStateObject() const
	{
		return this->d3dStateObject.get();
	}

	inline ID3D12RootSignature* GrafRayTracingPipelineDX12::GetD3DRootSignature() const
	{
		return this->d3dRootSignature.get();
	}

	inline GrafBuffer* GrafAccelerationStructureDX12::GetScratchBuffer() const
	{
		return this->grafScratchBuffer.get();
	}

} // end namespace UnlimRealms