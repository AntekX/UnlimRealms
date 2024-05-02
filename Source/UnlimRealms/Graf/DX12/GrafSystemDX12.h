///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: DIRECTX12 IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Graf/GrafSystem.h"
#include "Sys/Windows/WinUtils.h"
#include "Core/Memory.h"
#include <dxgi1_5.h>
#include <d3d12.h>

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSystemDX12 : public GrafSystem
	{
	public:

		GrafSystemDX12(Realm &realm);

		virtual ~GrafSystemDX12();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList);

		virtual Result CreateFence(std::unique_ptr<GrafFence>& grafFence);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);

		virtual Result CreateImage(std::unique_ptr<GrafImage>& grafImage);

		virtual Result CreateImageSubresource(std::unique_ptr<GrafImageSubresource>& grafImageSubresource);

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateSampler(std::unique_ptr<GrafSampler>& grafSampler);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateShaderLib(std::unique_ptr<GrafShaderLib>& grafShaderLib);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);

		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		virtual Result CreateComputePipeline(std::unique_ptr<GrafComputePipeline>& grafComputePipeline);

		virtual Result CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline);

		virtual Result CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct);

		virtual const char* GetShaderExtension() const;

		inline IDXGIFactory5* GetDXGIFactory() const;

		inline IDXGIAdapter1* GetDXGIAdapter(ur_uint deviceId) const;

	private:

		Result Deinitialize();

		shared_ref<IDXGIFactory5> dxgiFactory;
		std::vector<shared_ref<IDXGIAdapter1>> dxgiAdapters;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum GrafShaderVisibleDescriptorHeapTypeDX12
	{
		GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv = 0,
		GrafShaderVisibleDescriptorHeapTypeDX12_Sampler,
		GrafShaderVisibleDescriptorHeapTypeDX12_Count
	};
	static_assert(GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	static_assert(GrafShaderVisibleDescriptorHeapTypeDX12_Sampler == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorHeapDX12
	{
	public:

		// base class for GrafDeviceDX12::DescriptorHeap

		inline ur_bool GetIsShaderVisible() const;

		inline ur_size GetDescriptorIncrementSize() const;

		inline ID3D12DescriptorHeap* GetD3DDescriptorHeap() const;

	protected:

		ur_bool isShaderVisible;
		ur_size descriptorIncrementSize;
		shared_ref<ID3D12DescriptorHeap> d3dDescriptorHeap;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorHandleDX12
	{
	public:

		inline ur_bool IsValid() const;

		inline const GrafDescriptorHeapDX12* GetHeap() const;

		inline const Allocation& GetAllocation() const;

		inline D3D12_CPU_DESCRIPTOR_HANDLE GetD3DHandleCPU() const;

		inline D3D12_GPU_DESCRIPTOR_HANDLE GetD3DHandleGPU() const;
		
	private:

		friend class GrafDeviceDX12;

		GrafDescriptorHeapDX12* heap;
		Allocation allocation;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDeviceDX12 : public GrafDevice
	{
	public:

		GrafDeviceDX12(GrafSystem &grafSystem);

		~GrafDeviceDX12();

		virtual Result Initialize(ur_uint deviceId);

		virtual Result Record(GrafCommandList* grafCommandList);

		virtual Result Submit();

		virtual Result WaitIdle();

		inline ID3D12Device5* GetD3DDevice() const;

		inline ID3D12CommandQueue* GetD3DGraphicsCommandQueue() const;

		inline ID3D12CommandQueue* GetD3DComputeCommandQueue() const;

		inline ID3D12CommandQueue* GetD3DTransferCommandQueue() const;

		GrafDescriptorHandleDX12 AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE type, ur_size count);

		GrafDescriptorHandleDX12 AllocateDescriptorRangeGPU(D3D12_DESCRIPTOR_HEAP_TYPE type, ur_size count);

		void ReleaseDescriptorRange(const GrafDescriptorHandleDX12& range);

	private:

		friend class GrafCommandListDX12;
		struct CommandAllocator;
		struct CommandAllocatorPool;
		struct DeviceQueue;

		struct CommandAllocator
		{
			shared_ref<ID3D12CommandAllocator> d3dCommandAllocator;
			CommandAllocatorPool* pool;
			ur_uint64 submitFenceValue;
			ur_uint64 resetFenceValue;
		};

		struct CommandAllocatorPool
		{
			std::vector<std::unique_ptr<CommandAllocator>> commandAllocators;
			//std::mutex commandAllocatorsMutex;
		};

		struct DeviceQueue
		{
			shared_ref<ID3D12CommandQueue> d3dQueue;
			shared_ref<ID3D12Fence1> d3dSubmitFence;
			ur_uint64 nextSubmitFenceValue;
			std::unordered_map<std::thread::id, std::unique_ptr<CommandAllocatorPool>> commandPools;
			//std::mutex commandPoolsMutex;
			std::vector<GrafCommandList*> recordedCommandLists;
			//std::mutex recordedCommandListsMutex;
			std::mutex submitMutex;
		};

		struct DescriptorHeap : public GrafDescriptorHeapDX12
		{
			friend class GrafDeviceDX12;
			D3D12_DESCRIPTOR_HEAP_DESC d3dDesc;
			D3D12_CPU_DESCRIPTOR_HANDLE d3dHeapStartCpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE d3dHeapStartGpuHandle;
			LinearAllocator allocator;
		};

		struct DescriptorPool
		{
			std::unique_ptr<DescriptorHeap> descriptorHeapCPU;
			std::unique_ptr<DescriptorHeap> descriptorHeapGPU;
		};

		CommandAllocator* GetGraphicsCommandAllocator();
		CommandAllocator* GetComputeCommandAllocator();
		CommandAllocator* GetTransferCommandAllocator();

		Result Deinitialize();

		shared_ref<ID3D12Device5> d3dDevice;
		std::unique_ptr<DeviceQueue> graphicsQueue;
		std::unique_ptr<DeviceQueue> computeQueue;
		std::unique_ptr<DeviceQueue> transferQueue;
		std::unique_ptr<DescriptorPool> descriptorPool[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCommandListDX12 : public GrafCommandList
	{
	public:

		GrafCommandListDX12(GrafSystem &grafSystem);

		~GrafCommandListDX12();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result Begin();

		virtual Result End();

		virtual Result Wait(ur_uint64 timeout = ur_uint64(-1));

		virtual Result InsertDebugLabel(const char* name, const ur_float4& color = ur_float4::Zero);

		virtual Result BeginDebugLabel(const char* name, const ur_float4& color = ur_float4::Zero);

		virtual Result EndDebugLabel();

		virtual Result BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState);

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result ImageMemoryBarrier(GrafImageSubresource* grafImageSubresource, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result ClearColorImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue);

		virtual Result ClearDepthStencilImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result ClearDepthStencilImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue);

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues = ur_null);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset = 0);

		virtual Result BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset = 0);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImageSubresource* dstImageSubresource, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImageSubresource* srcImageSubresource, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result Copy(GrafImageSubresource* srcImageSubresource, GrafImageSubresource* dstImageSubresource, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result BindComputePipeline(GrafComputePipeline* grafPipeline);

		virtual Result BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline);

		virtual Result Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ);

		virtual Result BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount);

		virtual Result BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline);

		virtual Result BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline);

		virtual Result DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
			const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
			const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable);

		inline GrafDeviceDX12::CommandAllocator* GetCommandAllocator() const;

		inline ID3D12GraphicsCommandList4* GetD3DCommandList() const;

	private:

		friend class GrafDeviceDX12;

		Result Deinitialize();

		GrafDeviceDX12::CommandAllocator* commandAllocator;
		shared_ref<ID3D12GraphicsCommandList4> d3dCommandList;
		ur_bool closed;
		ur_uint64 submitFenceValue;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafFenceDX12 : public GrafFence
	{
	public:

		GrafFenceDX12(GrafSystem &grafSystem);

		~GrafFenceDX12();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result SetState(GrafFenceState state);

		virtual Result GetState(GrafFenceState& state);

		virtual Result WaitSignaled();

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCanvasDX12 : public GrafCanvas
	{
	public:

		GrafCanvasDX12(GrafSystem &grafSystem);

		~GrafCanvasDX12();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

		virtual Result Present();

		virtual GrafImage* GetCurrentImage();

		virtual GrafImage* GetSwapChainImage(ur_uint imageId);

	private:

		Result Deinitialize();

		Result AcquireNextImage();

		shared_ref<IDXGISwapChain4> dxgiSwapChain;
		std::vector<std::unique_ptr<GrafImage>> swapChainImages;

		// per frame data
		std::vector<std::unique_ptr<GrafCommandList>> imageTransitionCmdListBegin;
		std::vector<std::unique_ptr<GrafCommandList>> imageTransitionCmdListEnd;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImageDX12 : public GrafImage
	{
	public:

		GrafImageDX12(GrafSystem &grafSystem);

		~GrafImageDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		Result InitializeFromD3DResource(GrafDevice *grafDevice, const InitParams& initParams, shared_ref<ID3D12Resource>& d3dResource);

		inline ID3D12Resource* GetD3DResource() const;

		inline GrafImageSubresource* GetDefaultSubresource() const;

	private:

		Result Deinitialize();

		Result CreateDefaultSubresource();

		friend class GrafCommandListDX12;
		inline void SetState(GrafImageState state);

		shared_ref<ID3D12Resource> d3dResource;
		std::unique_ptr<GrafImageSubresource> grafDefaultSubresource;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImageSubresourceDX12 : public GrafImageSubresource
	{
	public:

		GrafImageSubresourceDX12(GrafSystem &grafSystem);

		~GrafImageSubresourceDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafDescriptorHandleDX12& GetRTVDescriptorHandle() const;

		inline const GrafDescriptorHandleDX12& GetDSVDescriptorHandle() const;

		inline const GrafDescriptorHandleDX12& GetSRVDescriptorHandle() const;

		inline const GrafDescriptorHandleDX12& GetUAVDescriptorHandle() const;

	protected:

		Result Deinitialize();

		Result CreateD3DImageView();

		friend class GrafCommandListDX12;
		inline void SetState(GrafImageState state);

		GrafDescriptorHandleDX12 rtvDescriptorHandle;
		GrafDescriptorHandleDX12 dsvDescriptorHandle;
		GrafDescriptorHandleDX12 srvDescriptorHandle;
		GrafDescriptorHandleDX12 uavDescriptorHandle;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafBufferDX12 : public GrafBuffer
	{
	public:

		GrafBufferDX12(GrafSystem &grafSystem);

		~GrafBufferDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline ID3D12Resource* GetD3DResource() const;

		inline const GrafDescriptorHandleDX12& GetCBVDescriptorHandle() const;

		inline const GrafDescriptorHandleDX12& GetSRVDescriptorHandle() const;

		inline const GrafDescriptorHandleDX12& GetUAVDescriptorHandle() const;

		inline const D3D12_VERTEX_BUFFER_VIEW* GetD3DVertexBufferView() const;

		inline const D3D12_INDEX_BUFFER_VIEW* GetD3DIndexBufferView() const;

	private:

		Result Deinitialize();

		friend class GrafCommandListDX12;
		inline void SetState(GrafBufferState& state);

		shared_ref<ID3D12Resource> d3dResource;
		GrafDescriptorHandleDX12 cbvDescriptorHandle;
		GrafDescriptorHandleDX12 srvDescriptorHandle;
		GrafDescriptorHandleDX12 uavDescriptorHandle;
		union
		{
			D3D12_VERTEX_BUFFER_VIEW d3dVBView;
			D3D12_INDEX_BUFFER_VIEW d3dIBView;
		};
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSamplerDX12 : public GrafSampler
	{
	public:

		GrafSamplerDX12(GrafSystem &grafSystem);

		~GrafSamplerDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafDescriptorHandleDX12& GetDescriptorHandle() const;

	protected:

		Result Deinitialize();

		GrafDescriptorHandleDX12 samplerDescriptorHandle;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShaderDX12 : public GrafShader
	{
	public:

		GrafShaderDX12(GrafSystem &grafSystem);

		~GrafShaderDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const void* GetByteCodePtr() const;

		inline ur_size GetByteCodeSize() const;

	private:

		Result Deinitialize();

		std::unique_ptr<ur_byte[]> byteCodeBuffer;
		ur_size byteCodeSize;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShaderLibDX12 : public GrafShaderLib
	{
	public:

		GrafShaderLibDX12(GrafSystem& grafSystem);

		~GrafShaderLibDX12();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

		inline const void* GetByteCodePtr() const;

		inline ur_size GetByteCodeSize() const;

	protected:

		Result Deinitialize();

		std::unique_ptr<ur_byte[]> byteCodeBuffer; // required for RT state sub ojects
		ur_size byteCodeSize;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderPassDX12 : public GrafRenderPass
	{
	public:

		GrafRenderPassDX12(GrafSystem& grafSystem);

		~GrafRenderPassDX12();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderTargetDX12 : public GrafRenderTarget
	{
	public:

		GrafRenderTargetDX12(GrafSystem &grafSystem);

		~GrafRenderTargetDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableLayoutDX12 : public GrafDescriptorTableLayout
	{
	public:

		GrafDescriptorTableLayoutDX12(GrafSystem &grafSystem);

		~GrafDescriptorTableLayoutDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const D3D12_ROOT_PARAMETER& GetSrvUavCbvTableD3DRootParameter() const;

		inline const D3D12_ROOT_PARAMETER& GetSamplerTableD3DRootParameter() const;

	protected:

		struct DescriptorTableDesc
		{
			std::vector<D3D12_DESCRIPTOR_RANGE> d3dDescriptorRanges;
			D3D12_ROOT_PARAMETER d3dRootParameter;
			ur_uint descriptorCount;
		};

		inline const DescriptorTableDesc& GetTableDescForHeapType(GrafShaderVisibleDescriptorHeapTypeDX12 heapType);

		friend class GrafDescriptorTableDX12;

	private:

		Result Deinitialize();

		DescriptorTableDesc descriptorTableDesc[GrafShaderVisibleDescriptorHeapTypeDX12_Count];
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableDX12 : public GrafDescriptorTable
	{
	public:

		GrafDescriptorTableDX12(GrafSystem &grafSystem);

		~GrafDescriptorTableDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs = 0, ur_size bufferRange = 0);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource, GrafSampler* sampler);

		virtual Result SetSampler(ur_uint bindingIdx, GrafSampler* sampler);

		virtual Result SetImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource);

		virtual Result SetImageArray(ur_uint bindingIdx, GrafImage** images, ur_uint imageCount);

		virtual Result SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetBufferArray(ur_uint bindingIdx, GrafBuffer** buffers, ur_uint bufferCount);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource);

		virtual Result SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure);

		inline const GrafDescriptorHandleDX12& GetSrvUavCbvDescriptorHeapHandle() const;

		inline const GrafDescriptorHandleDX12& GetSamplerDescriptorHeapHandle() const;

	private:

		Result Deinitialize();

		Result CopyResourceDescriptorToTable(ur_uint bindingIdx, const D3D12_CPU_DESCRIPTOR_HANDLE& d3dResourceHandle, D3D12_DESCRIPTOR_RANGE_TYPE d3dRangeType);

		inline const Result GetD3DDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE& d3dHandle, ur_uint bindingIdx, D3D12_DESCRIPTOR_RANGE_TYPE d3dRangeType) const;

		struct DescriptorTableData
		{
			GrafDescriptorHandleDX12 descriptorHeapHandle;
		};

		DescriptorTableData descriptorTableData[GrafShaderVisibleDescriptorHeapTypeDX12_Count];
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafPipelineDX12 : public GrafPipeline
	{
	public:

		GrafPipelineDX12(GrafSystem &grafSystem);

		~GrafPipelineDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline ID3D12PipelineState* GetD3DPipelineState() const;
		
		inline ID3D12RootSignature* GetD3DRootSignature() const;

		inline D3D12_PRIMITIVE_TOPOLOGY GetD3DPrimitiveTopology() const;

	protected:

		Result Deinitialize();

		shared_ref<ID3D12PipelineState> d3dPipelineState;
		shared_ref<ID3D12RootSignature> d3dRootSignature;
		D3D_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafComputePipelineDX12 : public GrafComputePipeline
	{
	public:

		GrafComputePipelineDX12(GrafSystem &grafSystem);

		~GrafComputePipelineDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline ID3D12PipelineState* GetD3DPipelineState() const;

		inline ID3D12RootSignature* GetD3DRootSignature() const;

	protected:

		Result Deinitialize();

		shared_ref<ID3D12PipelineState> d3dPipelineState;
		shared_ref<ID3D12RootSignature> d3dRootSignature;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRayTracingPipelineDX12 : public GrafRayTracingPipeline
	{
	public:

		GrafRayTracingPipelineDX12(GrafSystem &grafSystem);

		~GrafRayTracingPipelineDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr);

		inline ID3D12StateObject* GetD3DStateObject() const;

		inline ID3D12RootSignature* GetD3DRootSignature() const;

	protected:

		Result Deinitialize();

		shared_ref<ID3D12StateObject> d3dStateObject;
		shared_ref<ID3D12RootSignature> d3dRootSignature;
		std::vector<std::wstring> shaderGroupNames;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafAccelerationStructureDX12 : public GrafAccelerationStructure
	{
	public:

		GrafAccelerationStructureDX12(GrafSystem &grafSystem);

		~GrafAccelerationStructureDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline GrafBuffer* GetScratchBuffer() const;

		inline GrafBuffer* GetStorageBuffer() const;

	protected:

		Result Deinitialize();

		std::unique_ptr<GrafBuffer> grafScratchBuffer;
		std::unique_ptr<GrafBuffer> grafStorageBuffer;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO d3dPrebuildInfo;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafUtilsDX12
	{
	public:

		static inline D3D12_RESOURCE_STATES GrafToD3DBufferState(GrafBufferState state);
		static inline D3D12_RESOURCE_STATES GrafToD3DImageState(GrafImageState state);
		static inline D3D12_RESOURCE_DIMENSION GrafToD3DResDimension(GrafImageType imageType);
		static inline D3D12_RESOURCE_FLAGS GrafToD3DResourceFlags(GrafImageUsageFlags imageUsageFlags);
		static inline D3D12_RTV_DIMENSION GrafToD3DRTVDimension(GrafImageType imageType);
		static inline D3D12_DSV_DIMENSION GrafToD3DDSVDimension(GrafImageType imageType);
		static inline D3D12_SRV_DIMENSION GrafToD3DSRVDimension(GrafImageType imageType);
		static inline D3D12_UAV_DIMENSION GrafToD3DUAVDimension(GrafImageType imageType);
		static inline D3D12_HEAP_TYPE GrafToD3DBufferHeapType(GrafBufferUsageFlags bufferUsage, GrafDeviceMemoryFlags memoryFlags);
		static inline D3D12_RESOURCE_STATES GrafToD3DBufferInitialState(GrafBufferUsageFlags bufferUsage, GrafDeviceMemoryFlags memoryFlags);
		static inline D3D12_HEAP_TYPE GrafToD3DImageHeapType(GrafImageUsageFlags imageUsage, GrafDeviceMemoryFlags memoryFlags);
		static inline D3D12_RESOURCE_STATES GrafToD3DImageInitialState(GrafImageUsageFlags imageUsage, GrafDeviceMemoryFlags memoryFlags);
		static inline D3D12_FILTER GrafToD3DFilter(GrafFilterType filterMin, GrafFilterType filterMax, GrafFilterType filterMip);
		static inline D3D12_TEXTURE_ADDRESS_MODE GrafToD3DAddressMode(GrafAddressMode addressMode);
		static inline D3D12_DESCRIPTOR_RANGE_TYPE GrafToD3DDescriptorType(GrafDescriptorType descriptorType);
		static inline D3D12_SHADER_VISIBILITY GrafToD3DShaderVisibility(GrafShaderStageFlags sgaderStageFlags);
		static inline D3D12_BLEND_OP GrafToD3DBlendOp(GrafBlendOp blendOp);
		static inline D3D12_BLEND GrafToD3DBlendFactor(GrafBlendFactor blendFactor);
		static inline D3D12_CULL_MODE GrafToD3DCullMode(GrafCullMode cullMode);
		static inline D3D12_COMPARISON_FUNC GrafToD3DCompareOp(GrafCompareOp compareOp);
		static inline D3D12_STENCIL_OP GrafToD3DStencilOp(GrafStencilOp stencilOp);
		static inline D3D12_PRIMITIVE_TOPOLOGY GrafToD3DPrimitiveTopology(GrafPrimitiveTopology topology);
		static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE GrafToD3DPrimitiveTopologyType(GrafPrimitiveTopology topology);
		static inline const char* ParseVertexElementSemantic(const std::string& semantic, std::string& semanticName, ur_uint& semanticIdx);
		static inline DXGI_FORMAT GrafToDXGIFormat(GrafIndexType indexType);
		static inline DXGI_FORMAT GrafToDXGIFormat(GrafFormat grafFormat);
		static inline GrafFormat DXGIToGrafFormat(DXGI_FORMAT dxgiFormat);
		//static inline VkRayTracingShaderGroupTypeKHR GrafToVkRayTracingShaderGroupType(GrafRayTracingShaderGroupType shaderGroupType);
		static inline D3D12_RAYTRACING_GEOMETRY_TYPE GrafToD3DAccelerationStructureGeometryType(GrafAccelerationStructureGeometryType geometryType);
		static inline D3D12_RAYTRACING_GEOMETRY_FLAGS GrafToD3DAccelerationStructureGeometryFlags(GrafAccelerationStructureGeometryFlags geometryFlags);
		static inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE GrafToD3DAccelerationStructureType(GrafAccelerationStructureType structureType);
		static inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GrafToD3DAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlags buildFlags);
	};

} // end namespace UnlimRealms

#include "GrafSystemDX12.inline.h"