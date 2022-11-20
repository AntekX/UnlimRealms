///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "GrafSystemDX12.h"
#include "Sys/Log.h"
#if defined(_WINDOWS)
#include "Sys/Windows/WinCanvas.h"
#endif
#include "Gfx/D3D12/d3dx12.h"
#include "Gfx/DXGIUtils/DXGIUtils.h"
#include "comdef.h"
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace UnlimRealms
{

	#if defined(_DEBUG) 
	#define UR_GRAF_DX12_DEBUG_MODE
	#endif

	// command buffer synchronisation policy
	#define UR_GRAF_DX12_COMMAND_LIST_SYNC_DESTROY	1
	#define UR_GRAF_DX12_COMMAND_LIST_SYNC_RESET	1
	#define UR_GRAF_DX12_SLEEPZERO_WHILE_WAIT		0

	// descritor heap sizes per type
	static const ur_uint DescriptorHeapSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		4 * 1024,	// D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		1 * 1024,	// D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		1 * 1024,	// D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		1 * 1024,	// D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

	// descriptor heap default flags per type
	static const D3D12_DESCRIPTOR_HEAP_FLAGS DescriptorHeapFlags[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,	// D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,	// D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,			// D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,			// D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

	static const char* HResultToString(HRESULT res)
	{
		_com_error err(res);
		return (const char*)err.ErrorMessage();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafSystemDX12::GrafSystemDX12(Realm &realm) : 
		GrafSystem(realm)
	{
	}

	GrafSystemDX12::~GrafSystemDX12()
	{
		this->Deinitialize();
	}

	Result GrafSystemDX12::Deinitialize()
	{
		this->dxgiAdapters.clear();
		this->dxgiFactory.reset();

		return Result(Success);
	}

	Result GrafSystemDX12::Initialize(Canvas *canvas)
	{
		this->Deinitialize();

		LogNoteGrafDbg("GrafSystemDX12: initialization...");

		GrafSystem::Initialize(canvas);

		// initiaize DXGI

		HRESULT hres = CreateDXGIFactory2(0, __uuidof(this->dxgiFactory), this->dxgiFactory);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafSystemDX12: CreateDXGIFactory2 failed with HRESULT = ") + HResultToString(hres));
		}

		// enumerate physical devices

		shared_ref<IDXGIAdapter1> dxgiAdapter;
		ur_size adapterCount = 0;
		while (this->dxgiFactory->EnumAdapters1((ur_uint)adapterCount, dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			this->dxgiAdapters.push_back(dxgiAdapter);
			dxgiAdapter.reset();
			++adapterCount;
		}
		this->grafPhysicalDeviceDesc.resize(adapterCount);

		for (ur_size iadapter = 0; iadapter < adapterCount; ++iadapter)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
			hres = this->dxgiAdapters[iadapter]->GetDesc1(&dxgiAdapterDesc);
			if (FAILED(hres))
				break;

			GrafPhysicalDeviceDesc& grafDeviceDesc = this->grafPhysicalDeviceDesc[iadapter];
			grafDeviceDesc = {};
			grafDeviceDesc.Description = _bstr_t(dxgiAdapterDesc.Description);
			grafDeviceDesc.VendorId = (ur_uint)dxgiAdapterDesc.VendorId;
			grafDeviceDesc.DeviceId = (ur_uint)dxgiAdapterDesc.DeviceId;
			grafDeviceDesc.LocalMemory = (ur_size)dxgiAdapterDesc.DedicatedVideoMemory;
			grafDeviceDesc.LocalMemoryExclusive = grafDeviceDesc.LocalMemory;
			grafDeviceDesc.LocalMemoryHostVisible = grafDeviceDesc.LocalMemory;
			grafDeviceDesc.SystemMemory = (ur_size)(dxgiAdapterDesc.SharedSystemMemory + dxgiAdapterDesc.DedicatedSystemMemory);
			grafDeviceDesc.ConstantBufferOffsetAlignment = 256;

			#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
			LogNoteGrafDbg(std::string("GrafSystemDX12: device available ") + grafDeviceDesc.Description +
				", VRAM = " + std::to_string(grafDeviceDesc.LocalMemory >> 20) + " Mb");
			#endif

			// create temporary device object to check features support

			shared_ref<ID3D12Device5> d3dDevice;
			hres = D3D12CreateDevice(dxgiAdapters[iadapter], D3D_FEATURE_LEVEL_12_1, __uuidof(d3dDevice), d3dDevice);
			if (FAILED(hres))
				continue;

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3dOptions5;
			hres = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3dOptions5, sizeof(d3dOptions5));
			if (FAILED(hres))
				continue;

			grafDeviceDesc.RayTracing.RayTraceSupported = (ur_bool)(d3dOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
			grafDeviceDesc.RayTracing.RayQuerySupported = (ur_bool)(d3dOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1);
			grafDeviceDesc.RayTracing.ShaderGroupHandleSize = 8;
			grafDeviceDesc.RayTracing.RecursionDepthMax = ~ur_uint32(0);
			grafDeviceDesc.RayTracing.GeometryCountMax = ~ur_uint64(0);
			grafDeviceDesc.RayTracing.InstanceCountMax = ~ur_uint64(0);
			grafDeviceDesc.RayTracing.PrimitiveCountMax = ~ur_uint64(0);
		}
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafSystemDX12: failed to enumerate adapters with HRESULT = ") + HResultToString(hres));
		}

		// enable the D3D12 debug layer
		#if defined(UR_GRAF_DX12_DEBUG_MODE)
		shared_ref<ID3D12Debug> debugController;
		hres = D3D12GetDebugInterface(__uuidof(debugController), debugController);
		if (SUCCEEDED(hres))
		{
			debugController->EnableDebugLayer();
		}
		#endif

		return Result(Success);
	}

	Result GrafSystemDX12::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
	{
		grafDevice.reset(new GrafDeviceDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList)
	{
		grafCommandList.reset(new GrafCommandListDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateFence(std::unique_ptr<GrafFence>& grafFence)
	{
		grafFence.reset(new GrafFenceDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas)
	{
		grafCanvas.reset(new GrafCanvasDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateImage(std::unique_ptr<GrafImage>& grafImage)
	{
		grafImage.reset(new GrafImageDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateImageSubresource(std::unique_ptr<GrafImageSubresource>& grafImageSubresource)
	{
		grafImageSubresource.reset(new GrafImageSubresourceDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer)
	{
		grafBuffer.reset(new GrafBufferDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateSampler(std::unique_ptr<GrafSampler>& grafSampler)
	{
		grafSampler.reset(new GrafSamplerDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateShader(std::unique_ptr<GrafShader>& grafShader)
	{
		grafShader.reset(new GrafShaderDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass)
	{
		grafRenderPass.reset(new GrafRenderPassDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget)
	{
		grafRenderTarget.reset(new GrafRenderTargetDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout)
	{
		grafDescriptorTableLayout.reset(new GrafDescriptorTableLayoutDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable)
	{
		grafDescriptorTable.reset(new GrafDescriptorTableDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline)
	{
		grafPipeline.reset(new GrafPipelineDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateComputePipeline(std::unique_ptr<GrafComputePipeline>& grafComputePipeline)
	{
		grafComputePipeline.reset(new GrafComputePipelineDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline)
	{
		grafRayTracingPipeline.reset(new GrafRayTracingPipelineDX12(*this));
		return Result(Success);
	}

	Result GrafSystemDX12::CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct)
	{
		grafAccelStruct.reset(new GrafAccelerationStructureDX12(*this));
		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDeviceDX12::GrafDeviceDX12(GrafSystem &grafSystem) :
		GrafDevice(grafSystem)
	{
	}

	GrafDeviceDX12::~GrafDeviceDX12()
	{
		this->Deinitialize();
	}

	Result GrafDeviceDX12::Deinitialize()
	{
		this->WaitIdle();

		for (std::unique_ptr<DescriptorPool>& descriptorPool : this->descriptorPool)
		{
			descriptorPool.reset();
		}
		this->graphicsQueue.reset();
		this->computeQueue.reset();
		this->transferQueue.reset();
		this->d3dDevice.reset();

		return Result(Success);
	}

	Result GrafDeviceDX12::Initialize(ur_uint deviceId)
	{
		this->Deinitialize();

		LogNoteGrafDbg("GrafDeviceDX12: initialization...");

		GrafDevice::Initialize(deviceId);

		// get corresponding physical device

		GrafSystemDX12& grafSystemDX12 = static_cast<GrafSystemDX12&>(this->GetGrafSystem());
		IDXGIAdapter1* dxgiAdapter = grafSystemDX12.GetDXGIAdapter(deviceId);
		if (ur_null == dxgiAdapter)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceDX12: can not find IDXGIAdapter for deviceId = ") + std::to_string(this->GetDeviceId()));
		}

		// create d3d device

		HRESULT hres = D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(d3dDevice), d3dDevice);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceDX12: D3D12CreateDevice failed with HRESULT = ") + HResultToString(hres));
		}

		// initialize graphics command queue

		this->graphicsQueue.reset(new DeviceQueue());
		this->graphicsQueue->nextSubmitFenceValue = 1;
		
		D3D12_COMMAND_QUEUE_DESC d3dGraphicsQueueDesc = {};
		d3dGraphicsQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		d3dGraphicsQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		d3dGraphicsQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dGraphicsQueueDesc.NodeMask = 0;
		
		hres = this->d3dDevice->CreateCommandQueue(&d3dGraphicsQueueDesc, __uuidof(this->graphicsQueue->d3dQueue), this->graphicsQueue->d3dQueue);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceDX12: CreateCommandQueue failed with HRESULT = ") + HResultToString(hres));
		}

		hres = this->d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(this->graphicsQueue->d3dSubmitFence), this->graphicsQueue->d3dSubmitFence);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceDX12: CreateFence failed with HRESULT = ") + HResultToString(hres));
		}

		// create default descriptor heaps

		for (ur_uint heapTypeIdx = 0; heapTypeIdx < ur_uint(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES); ++heapTypeIdx)
		{
			std::unique_ptr<DescriptorHeap> heap(new DescriptorHeap());
			heap->d3dDesc = {};
			heap->d3dDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(heapTypeIdx);
			heap->d3dDesc.Flags = DescriptorHeapFlags[heapTypeIdx];
			heap->d3dDesc.NumDescriptors = DescriptorHeapSize[heapTypeIdx];
			heap->descriptorIncrementSize = this->d3dDevice->GetDescriptorHandleIncrementSize(heap->d3dDesc.Type);
			heap->allocator.Init(ur_size(heap->d3dDesc.NumDescriptors));
			hres = this->d3dDevice->CreateDescriptorHeap(&heap->d3dDesc, __uuidof(heap->d3dDescriptorHeap), heap->d3dDescriptorHeap);
			if (FAILED(hres))
			{
				this->Deinitialize();
				return ResultError(Failure, std::string("GrafDeviceDX12: CreateDescriptorHeap failed with HRESULT = ") + HResultToString(hres));
			}
			heap->d3dHeapStartCpuHandle = heap->d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			heap->d3dHeapStartGpuHandle = heap->d3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

			std::unique_ptr<DescriptorPool> pool(new DescriptorPool());
			pool->descriptorHeaps.push_back(std::move(heap));

			this->descriptorPool[heapTypeIdx] = std::move(pool);
		}

		return Result(Success);
	}

	Result GrafDeviceDX12::Record(GrafCommandList* grafCommandList)
	{
		// NOTE: currently all command lists are recorded to graphics queue

		Result res = NotFound;

		if (this->graphicsQueue != ur_null)
		{
			std::lock_guard<std::mutex> lock(this->graphicsQueue->recordedCommandListsMutex);
			this->graphicsQueue->recordedCommandLists.push_back(grafCommandList);
			res = Success;
		}

		return Result(res);
	}

	Result GrafDeviceDX12::Submit()
	{
		// NOTE: support submission to different queue families
		// currently everything's done on the graphics queue

		std::vector<ID3D12DescriptorHeap*> d3dDescriptorHeaps;
		d3dDescriptorHeaps.reserve(ur_array_size(descriptorPool));
		for (auto& pool : descriptorPool)
		{
			for (auto& heap : pool->descriptorHeaps)
			{
				d3dDescriptorHeaps.push_back(heap->d3dDescriptorHeap.get());
			}
		}

		if (this->graphicsQueue)
		{
			this->graphicsQueue->commandPoolsMutex.lock(); // lock pools list modification
			for (auto& pool : this->graphicsQueue->commandPools) pool.second->commandAllocatorsMutex.lock(); // lock per thread allocators write access
			this->graphicsQueue->recordedCommandListsMutex.lock(); // lock recording
			
			// execute command lists
			for (auto& grafCommandList : this->graphicsQueue->recordedCommandLists)
			{
				GrafCommandListDX12* grafCommandListDX12 = static_cast<GrafCommandListDX12*>(grafCommandList);
				GrafDeviceDX12::CommandAllocator* grafCmdAllocator = grafCommandListDX12->GetCommandAllocator();
				ID3D12GraphicsCommandList1* d3dGraphicsCommandList = grafCommandListDX12->GetD3DCommandList();

				// bind descriptor heaps
				d3dGraphicsCommandList->SetDescriptorHeaps((UINT)d3dDescriptorHeaps.size(), &d3dDescriptorHeaps.front());
				
				// execute
				ID3D12CommandList* d3dCommandLists[] = { d3dGraphicsCommandList };
				this->graphicsQueue->d3dQueue->ExecuteCommandLists(1, d3dCommandLists);
				
				grafCommandListDX12->submitFenceValue = this->graphicsQueue->nextSubmitFenceValue;
				grafCmdAllocator->submitFenceValue = this->graphicsQueue->nextSubmitFenceValue;
			}
			this->graphicsQueue->recordedCommandLists.clear();

			this->graphicsQueue->recordedCommandListsMutex.unlock();
			for (auto& pool : this->graphicsQueue->commandPools) pool.second->commandAllocatorsMutex.unlock();
			this->graphicsQueue->commandPoolsMutex.unlock();

			// signal submission fence
			this->graphicsQueue->d3dQueue->Signal(this->graphicsQueue->d3dSubmitFence, this->graphicsQueue->nextSubmitFenceValue);
			this->graphicsQueue->nextSubmitFenceValue += 1;
		}
		
		return Result(Success);
	}

	Result GrafDeviceDX12::WaitIdle()
	{
		auto WaitQueue = [](DeviceQueue* deviceQueue) -> void
		{
			if (ur_null == deviceQueue)
				return;
			while (deviceQueue->nextSubmitFenceValue - 1 > deviceQueue->d3dSubmitFence->GetCompletedValue())
			{
				#if (UR_GRAF_DX12_SLEEPZERO_WHILE_WAIT)
				Sleep(0);
				#endif
			}
		};

		WaitQueue(this->graphicsQueue.get());
		WaitQueue(this->computeQueue.get());
		WaitQueue(this->transferQueue.get());
		
		return Result(Success);
	}

	GrafDeviceDX12::CommandAllocator* GrafDeviceDX12::GetGraphicsCommandAllocator()
	{
		if (ur_null == this->graphicsQueue)
			return ur_null;

		// get an existing pool for current thread or create a new one

		CommandAllocatorPool* commandAllocatorPool = ur_null;
		std::thread::id thisThreadId = std::this_thread::get_id();
		this->graphicsQueue->commandPoolsMutex.lock();
		auto& poolIter = this->graphicsQueue->commandPools.find(thisThreadId);
		if (poolIter != this->graphicsQueue->commandPools.end())
		{
			commandAllocatorPool = poolIter->second.get();
		}
		else
		{
			std::unique_ptr<CommandAllocatorPool> newPool(new CommandAllocatorPool());
			commandAllocatorPool = newPool.get();
			this->graphicsQueue->commandPools[thisThreadId] = std::move(newPool);
		}
		this->graphicsQueue->commandPoolsMutex.unlock();

		// get a reusable allocator or create a mew one

		CommandAllocator* commandAllocator = ur_null;
		ur_uint64 completedFenceValue = this->graphicsQueue->d3dSubmitFence->GetCompletedValue();
		commandAllocatorPool->commandAllocatorsMutex.lock();
		for (auto& cachedAllocator : commandAllocatorPool->commandAllocators)
		{
			if (cachedAllocator->submitFenceValue > completedFenceValue)
				continue; // allocated commands potentially still used on gpu

			commandAllocator = cachedAllocator.get();
			break;
		}
		if (ur_null == commandAllocator)
		{
			std::unique_ptr<CommandAllocator> newAllocator(new CommandAllocator());
			HRESULT hres = this->d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(newAllocator->d3dCommandAllocator), newAllocator->d3dCommandAllocator);
			if (FAILED(hres))
			{
				LogError(std::string("GrafDeviceDX12: CreateCommandAllocator failed with HRESULT = ") + HResultToString(hres));
				return nullptr;
			}
			newAllocator->pool = commandAllocatorPool;
			newAllocator->submitFenceValue = 0;
			newAllocator->resetFenceValue = 0;
			commandAllocator = newAllocator.get();
			commandAllocatorPool->commandAllocators.push_back(std::move(newAllocator));
		}
		commandAllocatorPool->commandAllocatorsMutex.unlock();

		return commandAllocator;
	}

	GrafDeviceDX12::CommandAllocator* GrafDeviceDX12::GetComputeCommandAllocator()
	{
		// not implemented

		return ur_null;
	}

	GrafDeviceDX12::CommandAllocator* GrafDeviceDX12::GetTransferCommandAllocator()
	{
		// not implemented

		return ur_null;
	}

	GrafDescriptorHandleDX12 GrafDeviceDX12::AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE type, ur_size count)
	{
		GrafDescriptorHandleDX12 handle = {};

		if (type >= D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
			return handle; // invalid type

		DescriptorPool* pool = this->descriptorPool[ur_uint(type)].get();
		if (nullptr == pool || pool->descriptorHeaps.empty())
			return handle; // not initialized

		DescriptorHeap* heap = pool->descriptorHeaps.back().get();
		handle.allocation = heap->allocator.Allocate(count);
		if (0 == handle.allocation.Size)
			return handle; // out of memory

		handle.heap = heap;
		handle.cpuHandle.ptr = heap->d3dHeapStartCpuHandle.ptr + SIZE_T(handle.allocation.Offset * heap->descriptorIncrementSize);
		handle.gpuHandle.ptr = heap->d3dHeapStartGpuHandle.ptr + UINT64(handle.allocation.Offset * heap->descriptorIncrementSize);

		return handle;
	}

	void GrafDeviceDX12::ReleaseDescriptorRange(const GrafDescriptorHandleDX12& range)
	{
		// TODO:
		// currently used LinearAllocator can only grow, proper allocator must be implemented/used
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafCommandListDX12::GrafCommandListDX12(GrafSystem &grafSystem) :
		GrafCommandList(grafSystem)
	{
		this->commandAllocator = ur_null;
		this->submitFenceValue = 0;
		this->closed = true;
	}

	GrafCommandListDX12::~GrafCommandListDX12()
	{
		this->Deinitialize();
	}

	Result GrafCommandListDX12::Deinitialize()
	{
		#if (UR_GRAF_DX12_COMMAND_LIST_SYNC_DESTROY)
		this->Wait(ur_uint64(-1));
		#endif
		this->submitFenceValue = 0;
		this->commandAllocator = ur_null;
		this->closed = true;
		
		return Result(Success);
	}

	Result GrafCommandListDX12::Initialize(GrafDevice *grafDevice)
	{
		this->Deinitialize();

		GrafCommandList::Initialize(grafDevice);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafCommandListDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// request allocator from device pool

		this->commandAllocator = grafDeviceDX12->GetGraphicsCommandAllocator();
		if (ur_null == this->commandAllocator)
		{
			return ResultError(Failure, std::string("GrafCommandListDX12: failed to initialize, invalid command allocator"));
		}

		// create command list in closed state

		const ur_uint nodeMask = 0;
		this->closed = true;
		HRESULT hres = d3dDevice->CreateCommandList1(nodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
			__uuidof(this->d3dCommandList), this->d3dCommandList);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafCommandListDX12: d3dDevice->CreateCommandList1 failed with HRESULT = ") + HResultToString(hres));
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::Begin()
	{
		if (ur_null == this->d3dCommandList.get())
			return Result(NotInitialized);

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());
		
		#if (UR_GRAF_DX12_COMMAND_LIST_SYNC_RESET)
		// make sure this command list can be reused safely
		this->Wait(ur_uint64(-1));
		#endif

		this->commandAllocator->pool->commandAllocatorsMutex.lock();
		
		if (this->commandAllocator->resetFenceValue <= this->commandAllocator->submitFenceValue)
		{
			// first time allocator reset after submitted commands done
			HRESULT hres = this->commandAllocator->d3dCommandAllocator->Reset();
			if (FAILED(hres))
			{
				this->commandAllocator->pool->commandAllocatorsMutex.unlock();
				return ResultError(Failure, std::string("GrafCommandListDX12: d3dCommandAllocator->Reset() failed with HRESULT = ") + HResultToString(hres));
			}
			this->commandAllocator->resetFenceValue = this->commandAllocator->submitFenceValue + 1;
		}

		if (this->closed)
		{
			// reset command list
			this->closed = false;
			HRESULT hres = this->d3dCommandList->Reset(this->commandAllocator->d3dCommandAllocator, ur_null);
			if (FAILED(hres))
			{
				this->commandAllocator->pool->commandAllocatorsMutex.unlock();
				return ResultError(Failure, std::string("GrafCommandListDX12: d3dCommandList->Reset() failed with HRESULT = ") + HResultToString(hres));
			}
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::End()
	{
		// close command list
		HRESULT hres = this->d3dCommandList->Close();
		this->closed = true;
		this->commandAllocator->pool->commandAllocatorsMutex.unlock();
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafCommandListDX12: d3dCommandList->Close() failed with HRESULT = ") + HResultToString(hres));
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::Wait(ur_uint64 timeout)
	{
		if (ur_null == this->d3dCommandList.get())
			return Result(NotInitialized);

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());
		Result res(Success);
		while (this->submitFenceValue > grafDeviceDX12->graphicsQueue->d3dSubmitFence->GetCompletedValue())
		{
			if (timeout != ur_uint64(-1))
			{
				// infinite and zero wait time are supported only
				res.Code = TimeOut;
				break;
			}
			#if (UR_GRAF_DX12_SLEEPZERO_WHILE_WAIT)
			Sleep(0);
			#endif
		}

		return res;
	}

	Result GrafCommandListDX12::InsertDebugLabel(const char* name, const ur_float4& color)
	{
		// requires pix shite
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BeginDebugLabel(const char* name, const ur_float4& color)
	{
		// requires pix shite
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::EndDebugLabel()
	{
		// requires pix shite
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState)
	{
		if (ur_null == grafBuffer)
			return Result(InvalidArgs);

		if (grafBuffer->GetState() == dstState)
			return Result(Success);

		srcState = (GrafBufferState::Current == srcState ? grafBuffer->GetState() : srcState);

		GrafBufferDX12* grafBufferDX12 = static_cast<GrafBufferDX12*>(grafBuffer);
		ID3D12Resource* d3dResource = grafBufferDX12->GetD3DResource();

		D3D12_RESOURCE_BARRIER d3dBarrier = {};
		d3dBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dBarrier.Transition.pResource = d3dResource;
		d3dBarrier.Transition.Subresource = 0;
		d3dBarrier.Transition.StateBefore = GrafUtilsDX12::GrafToD3DBufferState(srcState);
		d3dBarrier.Transition.StateAfter = GrafUtilsDX12::GrafToD3DBufferState(dstState);

		this->d3dCommandList->ResourceBarrier(1, &d3dBarrier);
		
		return Result(Success);
	}

	Result GrafCommandListDX12::ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState)
	{
		if (ur_null == grafImage)
			return Result(InvalidArgs);

		GrafImageDX12* grafImageDX12 = static_cast<GrafImageDX12*>(grafImage);
		Result res = this->ImageMemoryBarrier(grafImageDX12->GetDefaultSubresource(), srcState, dstState);
		if (Succeeded(res))
		{
			// image state = default subresource state (all mips/layers)
			static_cast<GrafImageDX12*>(grafImage)->SetState(dstState);
		}

		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ImageMemoryBarrier(GrafImageSubresource* grafImageSubresource, GrafImageState srcState, GrafImageState dstState)
	{
		if (ur_null == grafImageSubresource)
			return Result(InvalidArgs);

		const GrafImageDX12* grafImageDX12 = static_cast<const GrafImageDX12*>(grafImageSubresource->GetImage());
		if (ur_null == grafImageDX12)
			return Result(InvalidArgs);

		srcState = (GrafImageState::Current == srcState ? grafImageSubresource->GetState() : srcState);

		ID3D12Resource* d3dResource = grafImageDX12->GetD3DResource();

		ur_uint32 subresFirst = grafImageSubresource->GetDesc().BaseMipLevel;
		ur_uint32 subresCount = grafImageSubresource->GetDesc().LevelCount;
		std::vector<D3D12_RESOURCE_BARRIER> d3dBarriers(subresCount);
		for (ur_uint32 subresIdx = 0; subresIdx < subresCount; ++subresIdx)
		{
			D3D12_RESOURCE_BARRIER& d3dBarrier = d3dBarriers[subresIdx];
			d3dBarrier = {};
			d3dBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			d3dBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dBarrier.Transition.pResource = d3dResource;
			d3dBarrier.Transition.Subresource = subresFirst + subresIdx;
			d3dBarrier.Transition.StateBefore = GrafUtilsDX12::GrafToD3DImageState(srcState);
			d3dBarrier.Transition.StateAfter = GrafUtilsDX12::GrafToD3DImageState(dstState);
		}

		this->d3dCommandList->ResourceBarrier(UINT(subresCount), d3dBarriers.data());

		return Result(Success);
	}

	Result GrafCommandListDX12::SetFenceState(GrafFence* grafFence, GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearColorImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearDepthStencilImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearDepthStencilImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::SetScissorsRect(const RectI& scissorsRect)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::EndRenderPass()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindPipeline(GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset)
	{
		if (ur_null == grafVertexBuffer)
			return Result(InvalidArgs);

		this->GetD3DCommandList()->IASetVertexBuffers(0, 1, static_cast<GrafBufferDX12*>(grafVertexBuffer)->GetD3DVertexBufferView());

		return Result(Success);
	}

	Result GrafCommandListDX12::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset)
	{
		if (ur_null == grafIndexBuffer)
			return Result(InvalidArgs);

		this->GetD3DCommandList()->IASetIndexBuffer(static_cast<GrafBufferDX12*>(grafIndexBuffer)->GetD3DIndexBufferView());

		return Result(Success);
	}

	Result GrafCommandListDX12::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafImageSubresource* dstImageSubresource, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImageSubresource* srcImageSubresource, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion, BoxI dstRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImageSubresource* srcImageSubresource, GrafImageSubresource* dstImageSubresource, BoxI srcRegion, BoxI dstRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindComputePipeline(GrafComputePipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
		const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
		const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafFenceDX12::GrafFenceDX12(GrafSystem &grafSystem) :
		GrafFence(grafSystem)
	{
	}

	GrafFenceDX12::~GrafFenceDX12()
	{
		this->Deinitialize();
	}

	Result GrafFenceDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::Initialize(GrafDevice *grafDevice)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::SetState(GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::GetState(GrafFenceState& state)
	{
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::WaitSignaled()
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafCanvasDX12::GrafCanvasDX12(GrafSystem &grafSystem) :
		GrafCanvas(grafSystem)
	{
		this->frameCount = 0;
		this->frameIdx = 0;
	}

	GrafCanvasDX12::~GrafCanvasDX12()
	{
		this->Deinitialize();
	}

	Result GrafCanvasDX12::Deinitialize()
	{
		this->frameCount = 0;
		this->frameIdx = 0;
		this->swapChainImageCount = 0;
		this->swapChainCurrentImageId = 0;

		this->swapChainImages.clear();
		if (!this->dxgiSwapChain.empty())
		{
			this->dxgiSwapChain.reset(nullptr);
			LogNoteGrafDbg("GrafCanvasDX12: swap chain destroyed");
		}

		return Result(Success);
	}

	Result GrafCanvasDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		//return Result(NotImplemented);

		this->Deinitialize();

		LogNoteGrafDbg("GrafCanvasDX12: initialization...");

		GrafCanvas::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (nullptr == grafDeviceDX12 || nullptr == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafCanvasDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// init DXGI swap chain

		#if defined(_WINDOWS)

		WinCanvas* winCanvas = static_cast<WinCanvas*>(this->GetRealm().GetCanvas());
		if (ur_null == winCanvas)
		{
			return ResultError(InvalidArgs, std::string("GrafCanvasDX12: failed to initialize, invalid WinCanvas"));
		}

		GrafSystemDX12& grafSystemDX12 = static_cast<GrafSystemDX12&>(grafDeviceDX12->GetGrafSystem());
		IDXGIFactory5* dxgiFactory = grafSystemDX12.GetDXGIFactory();

		DXGI_SWAP_CHAIN_DESC1 dxgiChainDesc = {};
		dxgiChainDesc.Width = std::max((ur_uint)winCanvas->GetClientBound().Width(), ur_uint(1));
		dxgiChainDesc.Height = std::max((ur_uint)winCanvas->GetClientBound().Height(), ur_uint(1));
		dxgiChainDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(initParams.Format);
		dxgiChainDesc.Stereo = false;
		dxgiChainDesc.SampleDesc.Count = 1;
		dxgiChainDesc.SampleDesc.Quality = 0;
		dxgiChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		dxgiChainDesc.BufferCount = (UINT)initParams.SwapChainImageCount;
		dxgiChainDesc.Scaling = DXGI_SCALING_STRETCH;
		dxgiChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		dxgiChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		dxgiChainDesc.Flags = (DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

		shared_ref<IDXGISwapChain1> dxgiSwapChain1;
		HRESULT hres = dxgiFactory->CreateSwapChainForHwnd(grafDeviceDX12->GetD3DGraphicsCommandQueue(), winCanvas->GetHwnd(),
			&dxgiChainDesc, nullptr, nullptr, dxgiSwapChain1);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafCanvasDX12: IDXGIFactory::CreateSwapChainForHwnd failed with HRESULT = ") + HResultToString(hres));
		}
		LogNoteGrafDbg("GrafCanvasDX12: IDXGISwapChain1 created");

		hres = dxgiSwapChain1->QueryInterface(__uuidof(IDXGISwapChain4), this->dxgiSwapChain);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafCanvasDX12: QueryInterface(IDXGISwapChain4) failed with HRESULT = ") + HResultToString(hres));
		}

		// init swap chain images

		this->swapChainImages.reserve(initParams.SwapChainImageCount);
		for (ur_uint imageIdx = 0; imageIdx < initParams.SwapChainImageCount; ++imageIdx)
		{
			shared_ref<ID3D12Resource> d3dImageResource;
			hres = this->dxgiSwapChain->GetBuffer(imageIdx, __uuidof(ID3D12Resource), d3dImageResource);
			if (FAILED(hres))
			{
				this->Deinitialize();
				return ResultError(Failure, std::string("GrafCanvasDX12: IDXGISwapChain::GetBuffer failed with HRESULT = ") + HResultToString(hres));
			}

			GrafImage::InitParams imageParams = {};
			imageParams.ImageDesc.Type = GrafImageType::Tex2D;
			imageParams.ImageDesc.Format = GrafUtilsDX12::DXGIToGrafFormat(dxgiChainDesc.Format);
			imageParams.ImageDesc.Size.x = dxgiChainDesc.Width;
			imageParams.ImageDesc.Size.y = dxgiChainDesc.Height;
			imageParams.ImageDesc.Size.z = 0;
			imageParams.ImageDesc.MipLevels = 1;
			imageParams.ImageDesc.Usage = GrafImageUsageFlags(GrafImageUsageFlag::ColorRenderTarget);
			imageParams.ImageDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;

			std::unique_ptr<GrafImage> grafImage;
			grafSystemDX12.CreateImage(grafImage);
			Result res = static_cast<GrafImageDX12*>(grafImage.get())->InitializeFromD3DResource(grafDevice, imageParams, d3dImageResource);
			if (Failed(res))
			{
				this->Deinitialize();
				return ResultError(Failure, "GrafCanvasDX12: failed to create swap chain images");
			}
			this->swapChainImages.push_back(std::move(grafImage));
		}

		#else

		return ResultError(NotImplemented, std::string("GrafCanvasDX12: failed to initialize, unsupported platform"));

		#endif

		return Result(Success);
	}

	Result GrafCanvasDX12::AcquireNextImage()
	{
		return Result(NotImplemented);
	}

	Result GrafCanvasDX12::Present()
	{
		return Result(NotImplemented);
	}

	GrafImage* GrafCanvasDX12::GetCurrentImage()
	{
		return ur_null;
	}

	GrafImage* GrafCanvasDX12::GetSwapChainImage(ur_uint imageId)
	{
		return ur_null;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafImageDX12::GrafImageDX12(GrafSystem &grafSystem) :
		GrafImage(grafSystem)
	{
	}

	GrafImageDX12::~GrafImageDX12()
	{
		this->Deinitialize();
	}

	Result GrafImageDX12::Deinitialize()
	{
		this->grafDefaultSubresource.reset(ur_null);
		this->d3dResource.reset(nullptr);

		return Result(Success);
	}

	Result GrafImageDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::InitializeFromD3DResource(GrafDevice *grafDevice, const InitParams& initParams, shared_ref<ID3D12Resource>& d3dResource)
	{
		this->Deinitialize();

		GrafImage::Initialize(grafDevice, initParams);

		// validate device 

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || nullptr == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageDX12: failed to initialize, invalid GrafDevice"));
		}

		// aquire resosurce reference

		this->d3dResource = d3dResource;

		// create subresource views

		if (this->d3dResource.get() != nullptr)
		{
			Result res = this->CreateDefaultSubresource();
			if (Failed(res))
				return res;
		}

		return Result(Success);
	}

	Result GrafImageDX12::CreateDefaultSubresource()
	{
		if (nullptr == this->d3dResource.get())
		{
			return ResultError(InvalidArgs, std::string("GrafImageDX12: failed to create default subresource, d3dResource is nullptr"));
		}

		GrafSystemDX12& grafSystemDX12 = static_cast<GrafSystemDX12&>(this->GetGrafSystem());
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice()); // device is expected to be validated previously

		Result res = grafSystemDX12.CreateImageSubresource(this->grafDefaultSubresource);
		if (Failed(res))
		{
			return ResultError(InvalidArgs, std::string("GrafImageDX12: failed to create image subresource"));
		}

		GrafImageSubresource::InitParams grafSubresParams = {};
		grafSubresParams.Image = this;
		grafSubresParams.SubresourceDesc.BaseMipLevel = 0;
		grafSubresParams.SubresourceDesc.LevelCount = this->GetDesc().MipLevels;
		grafSubresParams.SubresourceDesc.BaseArrayLayer = 0;
		grafSubresParams.SubresourceDesc.LayerCount = 1;

		res = this->grafDefaultSubresource->Initialize(grafDeviceDX12, grafSubresParams);
		if (Failed(res))
		{
			this->grafDefaultSubresource.reset(ur_null);
			return ResultError(InvalidArgs, std::string("GrafImageDX12: failed to initialize image subresource"));
		}

		return Result(Success);
	}

	Result GrafImageDX12::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafImageSubresourceDX12::GrafImageSubresourceDX12(GrafSystem &grafSystem) :
		GrafImageSubresource(grafSystem)
	{
	}

	GrafImageSubresourceDX12::~GrafImageSubresourceDX12()
	{
		this->Deinitialize();
	}

	Result GrafImageSubresourceDX12::Deinitialize()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		if (this->rtvDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->rtvDescriptorHandle);
		}

		return Result(Success);
	}

	Result GrafImageSubresourceDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafImageSubresource::Initialize(grafDevice, initParams);

		// validate device 

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to initialize, invalid GrafDevice"));
		}

		// create view

		Result res = this->CreateD3DImageView();
		if (Failed(res))
			return res;

		return Result(Success);
	}

	Result GrafImageSubresourceDX12::CreateD3DImageView()
	{
		//return Result(NotImplemented);

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		// validate image

		const GrafImageDX12* grafImageDX12 = static_cast<const GrafImageDX12*>(this->GetImage());
		if (ur_null == grafImageDX12 || nullptr == grafImageDX12->GetD3DResource())
		{
			return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to initialize, invalid image"));
		}

		const GrafImageDesc& grafImageDesc = grafImageDX12->GetDesc();
		const GrafImageSubresourceDesc& grafSubresDesc = this->GetDesc();

		// create corresponding view(s)

		if (grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::ColorRenderTarget))
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
			rtvDesc.ViewDimension = GrafUtilsDX12::GrafToD3DRTVDimenstion(grafImageDesc.Type);
			switch (grafImageDesc.Type)
			{
			case GrafImageType::Tex1D:
				rtvDesc.Texture1D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				break;
			case GrafImageType::Tex2D:
				rtvDesc.Texture2D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				rtvDesc.Texture2D.PlaneSlice = UINT(grafSubresDesc.BaseArrayLayer);
				break;
			case GrafImageType::Tex3D:
				rtvDesc.Texture3D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				rtvDesc.Texture3D.FirstWSlice = UINT(grafSubresDesc.BaseArrayLayer);
				rtvDesc.Texture3D.WSize = UINT(grafSubresDesc.LayerCount);
				break;
			};

			this->rtvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
			if (!this->rtvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to allocate RTV descriptor"));
			}
			grafDeviceDX12->GetD3DDevice()->CreateRenderTargetView(grafImageDX12->GetD3DResource(), &rtvDesc, this->rtvDescriptorHandle.GetD3DHandleCPU());
		}

		// TODO: support SRV & UAV

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafBufferDX12::GrafBufferDX12(GrafSystem &grafSystem) :
		GrafBuffer(grafSystem)
	{
		this->d3dVBView = {};
		this->d3dIBView = {};
	}

	GrafBufferDX12::~GrafBufferDX12()
	{
		this->Deinitialize();
	}

	Result GrafBufferDX12::Deinitialize()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		if (this->cbvDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->cbvDescriptorHandle);
		}
		if (this->srvDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->srvDescriptorHandle);
		}
		if (this->uavDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->uavDescriptorHandle);
		}

		this->d3dVBView = {};
		this->d3dIBView = {};

		return Result(Success);
	}

	Result GrafBufferDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafBuffer::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// create resource

		D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
		d3dHeapProperties.Type = GrafUtilsDX12::GrafToD3DHeapType(initParams.BufferDesc.Usage);
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC d3dResDesc = {};
		d3dResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dResDesc.Alignment = 0;
		d3dResDesc.Width = (UINT64)initParams.BufferDesc.SizeInBytes;
		d3dResDesc.Height = 1;
		d3dResDesc.DepthOrArraySize = 1;
		d3dResDesc.MipLevels = 1;
		d3dResDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dResDesc.SampleDesc.Count = 1;
		d3dResDesc.SampleDesc.Quality = 0;
		d3dResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		d3dResDesc.Flags = (initParams.BufferDesc.Usage & ur_uint(GrafBufferUsageFlag::StorageBuffer) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);

		D3D12_RESOURCE_STATES d3dResStates = GrafUtilsDX12::GrafToD3DBufferInitialState(initParams.BufferDesc.Usage);

		HRESULT hres = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dResStates, ur_null,
			__uuidof(ID3D12Resource), this->d3dResource);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafBufferDX12: CreateCommittedResource failed with HRESULT = ") + HResultToString(hres));
		}

		// create CBV

		if (ur_uint(GrafBufferUsageFlag::ConstantBuffer) & initParams.BufferDesc.Usage)
		{
			this->cbvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->cbvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to allocate CBV descriptor"));
			}

			D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCbvDesc = {};
			d3dCbvDesc.BufferLocation = this->d3dResource->GetGPUVirtualAddress();
			d3dCbvDesc.SizeInBytes = (UINT)initParams.BufferDesc.SizeInBytes;

			grafDeviceDX12->GetD3DDevice()->CreateConstantBufferView(&d3dCbvDesc, this->cbvDescriptorHandle.GetD3DHandleCPU());
		}

		// create SRV

		if (ur_uint(GrafBufferUsageFlag::StorageBuffer) & initParams.BufferDesc.Usage)
		{
			this->srvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->srvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to allocate SRV descriptor"));
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {};
			d3dSrvDesc.Format = DXGI_FORMAT_R32_UINT;
			d3dSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			d3dSrvDesc.Buffer.FirstElement = 0;
			d3dSrvDesc.Buffer.NumElements = (UINT)(initParams.BufferDesc.SizeInBytes / sizeof(ur_uint32));
			d3dSrvDesc.Buffer.StructureByteStride = 0;
			d3dSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

			grafDeviceDX12->GetD3DDevice()->CreateShaderResourceView(this->d3dResource, &d3dSrvDesc, this->srvDescriptorHandle.GetD3DHandleCPU());
		}

		// create UAV

		if (ur_uint(GrafBufferUsageFlag::StorageBuffer) & initParams.BufferDesc.Usage)
		{
			this->uavDescriptorHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->uavDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to allocate UAV descriptor"));
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUavDesc = {};
			d3dUavDesc.Format = DXGI_FORMAT_R32_UINT;
			d3dUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			d3dUavDesc.Buffer.FirstElement = 0;
			d3dUavDesc.Buffer.NumElements = (UINT)(initParams.BufferDesc.SizeInBytes / sizeof(ur_uint32));
			d3dUavDesc.Buffer.StructureByteStride = 0;
			d3dUavDesc.Buffer.CounterOffsetInBytes = 0;
			d3dUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

			grafDeviceDX12->GetD3DDevice()->CreateUnorderedAccessView(this->d3dResource, nullptr, &d3dUavDesc, this->uavDescriptorHandle.GetD3DHandleCPU());
		}

		// pre-init VB view desc

		if (ur_uint(GrafBufferUsageFlag::VertexBuffer) & initParams.BufferDesc.Usage)
		{
			this->d3dVBView.BufferLocation = this->d3dResource->GetGPUVirtualAddress();
			this->d3dVBView.SizeInBytes = (UINT)initParams.BufferDesc.SizeInBytes;
			this->d3dVBView.StrideInBytes = (UINT)initParams.BufferDesc.ElementSize;
		}

		// pre-init IB view desc

		if (ur_uint(GrafBufferUsageFlag::IndexBuffer) & initParams.BufferDesc.Usage)
		{
			this->d3dIBView.BufferLocation = this->d3dResource->GetGPUVirtualAddress();
			this->d3dIBView.SizeInBytes = (UINT)initParams.BufferDesc.SizeInBytes;
			this->d3dIBView.Format = (
				initParams.BufferDesc.ElementSize == 32 ? DXGI_FORMAT_R32_UINT :
				initParams.BufferDesc.ElementSize == 16 ? DXGI_FORMAT_R16_UINT :
				DXGI_FORMAT_UNKNOWN);
		}

		return Result(Success);
	}

	Result GrafBufferDX12::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafSamplerDX12::GrafSamplerDX12(GrafSystem &grafSystem) :
		GrafSampler(grafSystem)
	{
	}

	GrafSamplerDX12::~GrafSamplerDX12()
	{
		this->Deinitialize();
	}

	Result GrafSamplerDX12::Deinitialize()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		if (this->samplerDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->samplerDescriptorHandle);
		}

		return Result(Success);
	}

	Result GrafSamplerDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafSampler::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafSamplerDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// create sampler

		this->samplerDescriptorHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
		if (!this->samplerDescriptorHandle.IsValid())
		{
			this->Deinitialize();
			return ResultError(InvalidArgs, std::string("GrafSamplerDX12: failed to allocate sampler descriptor"));
		}

		D3D12_SAMPLER_DESC d3dSamplerDesc = {};
		d3dSamplerDesc.Filter = GrafUtilsDX12::GrafToD3DFilter(initParams.SamplerDesc.FilterMin, initParams.SamplerDesc.FilterMag, initParams.SamplerDesc.FilterMip);
		d3dSamplerDesc.AddressU = GrafUtilsDX12::GrafToD3DAddressMode(initParams.SamplerDesc.AddressModeU);
		d3dSamplerDesc.AddressV = GrafUtilsDX12::GrafToD3DAddressMode(initParams.SamplerDesc.AddressModeV);
		d3dSamplerDesc.AddressW = GrafUtilsDX12::GrafToD3DAddressMode(initParams.SamplerDesc.AddressModeW);
		d3dSamplerDesc.MipLODBias = (FLOAT)initParams.SamplerDesc.MipLodBias;
		d3dSamplerDesc.MinLOD = (FLOAT)initParams.SamplerDesc.MipLodMin;
		d3dSamplerDesc.MaxLOD = (FLOAT)initParams.SamplerDesc.MipLodMax;
		d3dSamplerDesc.MaxAnisotropy = (UINT)initParams.SamplerDesc.AnisoFilterMax;
		d3dSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		d3dSamplerDesc.BorderColor[0] = (FLOAT)0.0f;
		d3dSamplerDesc.BorderColor[1] = (FLOAT)0.0f;
		d3dSamplerDesc.BorderColor[2] = (FLOAT)0.0f;
		d3dSamplerDesc.BorderColor[3] = (FLOAT)0.0f;

		d3dDevice->CreateSampler(&d3dSamplerDesc, this->samplerDescriptorHandle.GetD3DHandleCPU());

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafShaderDX12::GrafShaderDX12(GrafSystem &grafSystem) :
		GrafShader(grafSystem)
	{
	}

	GrafShaderDX12::~GrafShaderDX12()
	{
		this->Deinitialize();
	}

	Result GrafShaderDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafShaderDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		if (nullptr == initParams.ByteCode || 0 == initParams.ByteCodeSize)
		{
			return ResultError(InvalidArgs, std::string("GrafShaderDX12: failed to initialize, invalid byte code"));
		}

		GrafShader::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafShaderDX12: failed to initialize, invalid GrafDevice"));
		}

		// store byte code locally

		this->byteCodeSize = initParams.ByteCodeSize;
		this->byteCodeBuffer.reset(new ur_byte[this->byteCodeSize]);
		memcpy(this->byteCodeBuffer.get(), initParams.ByteCode, this->byteCodeSize);

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRenderTargetDX12::GrafRenderTargetDX12(GrafSystem &grafSystem) :
		GrafRenderTarget(grafSystem)
	{
	}

	GrafRenderTargetDX12::~GrafRenderTargetDX12()
	{
		this->Deinitialize();
	}

	Result GrafRenderTargetDX12::Deinitialize()
	{
		return Result(Success);
	}

	Result GrafRenderTargetDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafRenderTarget::Initialize(grafDevice, initParams);
		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableLayoutDX12::GrafDescriptorTableLayoutDX12(GrafSystem &grafSystem) :
		GrafDescriptorTableLayout(grafSystem)
	{
	}

	GrafDescriptorTableLayoutDX12::~GrafDescriptorTableLayoutDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableLayoutDX12::Deinitialize()
	{
		return Result(Success);
	}

	Result GrafDescriptorTableLayoutDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafDescriptorTableLayout::Initialize(grafDevice, initParams);

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableDX12::GrafDescriptorTableDX12(GrafSystem &grafSystem) :
		GrafDescriptorTable(grafSystem)
	{
		this->d3dRootParameter = {};
	}

	GrafDescriptorTableDX12::~GrafDescriptorTableDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableDX12::Deinitialize()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		if (this->srvUavCbvDescriptorRangeHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->srvUavCbvDescriptorRangeHandle);
		}
		if (this->samplerDescriptorRangeHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->samplerDescriptorRangeHandle);
		}

		this->d3dRootParameter = {};
		this->d3dDescriptorRanges.clear();

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafDescriptorTable::Initialize(grafDevice, initParams);

		if (nullptr == this->GetLayout())
		{
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to initialize, invalid layout"));
		}

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to initialize, invalid GrafDevice"));
		}

		// init D3D ranges & count descriptors per heap type

		const GrafDescriptorTableLayoutDesc& grafTableLayoutDesc = this->GetLayout()->GetLayoutDesc();
		this->d3dDescriptorRanges.resize(grafTableLayoutDesc.DescriptorRangeCount);

		ur_uint srvUavCbvDescriptorCount = 0;
		ur_uint samplerDescriptorCount = 0;
		for (ur_uint layoutRangeIdx = 0; layoutRangeIdx < grafTableLayoutDesc.DescriptorRangeCount; ++layoutRangeIdx)
		{
			const GrafDescriptorRangeDesc& grafDescriptorRange = grafTableLayoutDesc.DescriptorRanges[layoutRangeIdx];
			D3D12_DESCRIPTOR_RANGE d3dDescriptorRange = this->d3dDescriptorRanges[layoutRangeIdx];
			d3dDescriptorRange.RangeType = GrafUtilsDX12::GrafToD3DDescriptorType(grafDescriptorRange.Type);
			d3dDescriptorRange.BaseShaderRegister = (UINT)grafDescriptorRange.BindingOffset;
			d3dDescriptorRange.NumDescriptors = (UINT)grafDescriptorRange.BindingCount;
			d3dDescriptorRange.RegisterSpace = 0;
			d3dDescriptorRange.OffsetInDescriptorsFromTableStart = 0;

			if (GrafDescriptorType::ConstantBuffer == grafDescriptorRange.Type ||
				GrafDescriptorType::Texture == grafDescriptorRange.Type || GrafDescriptorType::Buffer == grafDescriptorRange.Type ||
				GrafDescriptorType::RWTexture == grafDescriptorRange.Type || GrafDescriptorType::RWBuffer == grafDescriptorRange.Type)
			{
				d3dDescriptorRange.OffsetInDescriptorsFromTableStart = srvUavCbvDescriptorCount;
				srvUavCbvDescriptorCount += grafDescriptorRange.BindingCount;
			}
			if (GrafDescriptorType::Sampler == grafDescriptorRange.Type)
			{
				d3dDescriptorRange.OffsetInDescriptorsFromTableStart = samplerDescriptorCount;
				samplerDescriptorCount += grafDescriptorRange.BindingCount;
			}
		}

		// allocate ranges in descriptor heap(s)

		if (srvUavCbvDescriptorCount > 0)
		{
			this->srvUavCbvDescriptorRangeHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvUavCbvDescriptorCount);
			if (!this->srvUavCbvDescriptorRangeHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to allocate SRV/UAV/CBV descriptor range"));
			}
		}
		if (samplerDescriptorCount > 0)
		{
			this->samplerDescriptorRangeHandle = grafDeviceDX12->AllocateDescriptorRange(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerDescriptorCount);
			if (!this->samplerDescriptorRangeHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to allocate sampler descriptor range"));
			}
		}

		// init D3D root parameter

		this->d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		this->d3dRootParameter.ShaderVisibility = GrafUtilsDX12::GrafToD3DShaderVisibility(grafTableLayoutDesc.ShaderStageVisibility);
		this->d3dRootParameter.DescriptorTable.NumDescriptorRanges = (UINT)this->d3dDescriptorRanges.size();
		this->d3dRootParameter.DescriptorTable.pDescriptorRanges = &this->d3dDescriptorRanges.front();
		for (D3D12_DESCRIPTOR_RANGE& d3dDescriptorRange : this->d3dDescriptorRanges)
		{
			// ranges share descriptor heaps, offset must be updated correspondingly
			if (D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER == d3dDescriptorRange.RangeType)
			{
				d3dDescriptorRange.OffsetInDescriptorsFromTableStart += (UINT)this->samplerDescriptorRangeHandle.GetAllocation().Offset;
			}
			else // SRV/UAV/CBV
			{
				d3dDescriptorRange.OffsetInDescriptorsFromTableStart += (UINT)this->srvUavCbvDescriptorRangeHandle.GetAllocation().Offset;
			}
		}

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::CopyResourceDescriptorToTable(D3D12_CPU_DESCRIPTOR_HANDLE resourceDescriptorHandle, ur_uint bindingIdx,
		D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType, D3D12_DESCRIPTOR_RANGE_TYPE d3dRangeType)
	{
		ID3D12Device5* d3dDevice = static_cast<GrafDeviceDX12*>(this->GetGrafDevice())->GetD3DDevice();

		// find corresponding range

		const D3D12_DESCRIPTOR_RANGE* d3dDescriptorRange = this->FindD3DDescriptorRange(d3dRangeType, bindingIdx);
		if (nullptr == d3dDescriptorRange)
			return ResultError(NotFound, std::string("GrafDescriptorTableDX12: failed to find descriptor range"));

		// copy resource descriptor to table's heap region

		D3D12_CPU_DESCRIPTOR_HANDLE d3dDescriptorHandleDst;
		ur_size rangeOffsetInTable = (ur_size)d3dDescriptorRange->OffsetInDescriptorsFromTableStart - this->srvUavCbvDescriptorRangeHandle.GetAllocation().Offset;
		ur_size bindingOffsetInTable = (bindingIdx - (ur_size)d3dDescriptorRange->BaseShaderRegister) + rangeOffsetInTable;
		SIZE_T bindingOffsetHeapSize = (SIZE_T)(bindingOffsetInTable * this->srvUavCbvDescriptorRangeHandle.GetHeap()->GetDescriptorIncrementSize());
		d3dDescriptorHandleDst.ptr = this->srvUavCbvDescriptorRangeHandle.GetD3DHandleCPU().ptr + bindingOffsetHeapSize;

		d3dDevice->CopyDescriptorsSimple(1, d3dDescriptorHandleDst, resourceDescriptorHandle, d3dHeapType);

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs, ur_size bufferRange)
	{
		return CopyResourceDescriptorToTable(static_cast<GrafBufferDX12*>(buffer)->GetCBVDescriptorHandle().GetD3DHandleCPU(), bindingIdx,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
	}

	Result GrafDescriptorTableDX12::SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetSampledImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetSampler(ur_uint bindingIdx, GrafSampler* sampler)
	{
		return CopyResourceDescriptorToTable(static_cast<GrafSamplerDX12*>(sampler)->GetDescriptorHandle().GetD3DHandleCPU(), bindingIdx,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
	}

	Result GrafDescriptorTableDX12::SetImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetImageArray(ur_uint bindingIdx, GrafImage** images, ur_uint imageCount)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return CopyResourceDescriptorToTable(static_cast<GrafBufferDX12*>(buffer)->GetSRVDescriptorHandle().GetD3DHandleCPU(), bindingIdx,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
	}

	Result GrafDescriptorTableDX12::SetBufferArray(ur_uint bindingIdx, GrafBuffer** buffers, ur_uint bufferCount)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetRWImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetRWImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return CopyResourceDescriptorToTable(static_cast<GrafBufferDX12*>(buffer)->GetUAVDescriptorHandle().GetD3DHandleCPU(), bindingIdx,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
	}

	Result GrafDescriptorTableDX12::SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafPipelineDX12::GrafPipelineDX12(GrafSystem &grafSystem) :
		GrafPipeline(grafSystem)
	{
	}

	GrafPipelineDX12::~GrafPipelineDX12()
	{
		this->Deinitialize();
	}

	Result GrafPipelineDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafPipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafComputePipelineDX12::GrafComputePipelineDX12(GrafSystem &grafSystem) :
		GrafComputePipeline(grafSystem)
	{
	}

	GrafComputePipelineDX12::~GrafComputePipelineDX12()
	{
		this->Deinitialize();
	}

	Result GrafComputePipelineDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafComputePipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRayTracingPipelineDX12::GrafRayTracingPipelineDX12(GrafSystem &grafSystem) :
		GrafRayTracingPipeline(grafSystem)
	{
	}

	GrafRayTracingPipelineDX12::~GrafRayTracingPipelineDX12()
	{
		this->Deinitialize();
	}

	Result GrafRayTracingPipelineDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafRayTracingPipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafRayTracingPipelineDX12::GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRenderPassDX12::GrafRenderPassDX12(GrafSystem& grafSystem) :
		GrafRenderPass(grafSystem)
	{
	}

	GrafRenderPassDX12::~GrafRenderPassDX12()
	{
		this->Deinitialize();
	}

	Result GrafRenderPassDX12::Deinitialize()
	{
		return Result(Success);
	}

	Result GrafRenderPassDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		GrafRenderPass::Initialize(grafDevice, initParams);
		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafAccelerationStructureDX12::GrafAccelerationStructureDX12(GrafSystem &grafSystem) :
		GrafAccelerationStructure(grafSystem)
	{
	}

	GrafAccelerationStructureDX12::~GrafAccelerationStructureDX12()
	{
		this->Deinitialize();
	}

	Result GrafAccelerationStructureDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafAccelerationStructureDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GrafUtilsDX12

	D3D12_RESOURCE_STATES GrafUtilsDX12::GrafToD3DBufferState(GrafBufferState state)
	{
		D3D12_RESOURCE_STATES d3dState = D3D12_RESOURCE_STATE_COMMON;
		switch (state)
		{
		case GrafBufferState::TransferSrc:
			d3dState = D3D12_RESOURCE_STATE_COPY_SOURCE;
			break;
		case GrafBufferState::TransferDst:
			d3dState = D3D12_RESOURCE_STATE_COPY_DEST;
			break;
		case GrafBufferState::VertexBuffer:
			d3dState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		case GrafBufferState::IndexBuffer:
			d3dState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			break;
		case GrafBufferState::ConstantBuffer:
			d3dState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		case GrafBufferState::ShaderRead:
			d3dState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			break;
		case GrafBufferState::ShaderReadWrite:
			d3dState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		case GrafBufferState::ComputeConstantBuffer:
			d3dState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		case GrafBufferState::ComputeRead:
			d3dState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			break;
		case GrafBufferState::ComputeReadWrite:
			d3dState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		case GrafBufferState::RayTracingConstantBuffer:
			d3dState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		case GrafBufferState::RayTracingRead:
			d3dState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			break;
		case GrafBufferState::RayTracingReadWrite:
			d3dState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		};
		return d3dState;
	}

	D3D12_RESOURCE_STATES GrafUtilsDX12::GrafToD3DImageState(GrafImageState state)
	{
		D3D12_RESOURCE_STATES d3dState = D3D12_RESOURCE_STATE_COMMON;
		switch (state)
		{
		case GrafImageState::TransferSrc:
			d3dState = D3D12_RESOURCE_STATE_COPY_SOURCE;
			break;
		case GrafImageState::TransferDst:
			d3dState = D3D12_RESOURCE_STATE_COPY_DEST;
			break;
		case GrafImageState::Present:
			d3dState = D3D12_RESOURCE_STATE_PRESENT;
			break;
		case GrafImageState::ColorWrite:
			d3dState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			break;
		case GrafImageState::DepthStencilWrite:
			d3dState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			break;
		case GrafImageState::DepthStencilRead:
			d3dState = D3D12_RESOURCE_STATE_DEPTH_READ;
			break;
		case GrafImageState::ShaderRead:
			d3dState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			break;
		case GrafImageState::ShaderReadWrite:
			d3dState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		case GrafImageState::ComputeRead:
			d3dState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			break;
		case GrafImageState::ComputeReadWrite:
			d3dState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		case GrafImageState::RayTracingRead:
			d3dState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			break;
		case GrafImageState::RayTracingReadWrite:
			d3dState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		};
		return d3dState;
	}

	D3D12_RTV_DIMENSION GrafUtilsDX12::GrafToD3DRTVDimenstion(GrafImageType imageType)
	{
		D3D12_RTV_DIMENSION d3dRTVDimension = D3D12_RTV_DIMENSION_UNKNOWN;
		switch (imageType)
		{
		case GrafImageType::Tex1D: d3dRTVDimension = D3D12_RTV_DIMENSION_TEXTURE1D; break;
		case GrafImageType::Tex2D: d3dRTVDimension = D3D12_RTV_DIMENSION_TEXTURE2D; break;
		case GrafImageType::Tex3D: d3dRTVDimension = D3D12_RTV_DIMENSION_TEXTURE3D; break;
		};
		return d3dRTVDimension;
	}

	D3D12_HEAP_TYPE GrafUtilsDX12::GrafToD3DHeapType(GrafBufferUsageFlags bufferUsage)
	{
		D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferSrc))
			d3dHeapType = D3D12_HEAP_TYPE_UPLOAD;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferDst))
			d3dHeapType = D3D12_HEAP_TYPE_READBACK;
		return d3dHeapType;
	}

	D3D12_RESOURCE_STATES GrafUtilsDX12::GrafToD3DBufferInitialState(GrafBufferUsageFlags bufferUsage)
	{
		D3D12_RESOURCE_STATES d3dStates = D3D12_RESOURCE_STATE_COMMON;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferSrc))
			d3dStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferDst))
			d3dStates = D3D12_RESOURCE_STATE_COPY_DEST;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::VertexBuffer))
			d3dStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::IndexBuffer))
			d3dStates = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::ConstantBuffer))
			d3dStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::StorageBuffer))
			d3dStates = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (bufferUsage & ur_uint(GrafBufferUsageFlag::AccelerationStructure))
			d3dStates = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		return d3dStates;
	}

	D3D12_FILTER GrafUtilsDX12::GrafToD3DFilter(GrafFilterType filterMin, GrafFilterType filterMax, GrafFilterType filterMip)
	{
		if (GrafFilterType::Nearest == filterMin	&& GrafFilterType::Nearest == filterMax		&& GrafFilterType::Nearest == filterMip)	return D3D12_FILTER_MIN_MAG_MIP_POINT;
		if (GrafFilterType::Nearest == filterMin	&& GrafFilterType::Nearest == filterMax		&& GrafFilterType::Linear == filterMip)		return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		if (GrafFilterType::Nearest == filterMin	&& GrafFilterType::Linear == filterMax		&& GrafFilterType::Nearest == filterMip)	return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		if (GrafFilterType::Nearest == filterMin	&& GrafFilterType::Linear == filterMax		&& GrafFilterType::Linear == filterMip)		return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		if (GrafFilterType::Linear == filterMin		&& GrafFilterType::Nearest == filterMax		&& GrafFilterType::Nearest == filterMip)	return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		if (GrafFilterType::Linear == filterMin		&& GrafFilterType::Nearest == filterMax		&& GrafFilterType::Linear == filterMip)		return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		if (GrafFilterType::Linear == filterMin		&& GrafFilterType::Linear == filterMax		&& GrafFilterType::Nearest == filterMip)	return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		if (GrafFilterType::Linear == filterMin		&& GrafFilterType::Linear == filterMax		&& GrafFilterType::Linear == filterMip)		return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		return D3D12_FILTER(0);
	}

	D3D12_TEXTURE_ADDRESS_MODE GrafUtilsDX12::GrafToD3DAddressMode(GrafAddressMode addressMode)
	{
		D3D12_TEXTURE_ADDRESS_MODE d3dAddressMode = D3D12_TEXTURE_ADDRESS_MODE(0);
		switch (addressMode)
		{
		case GrafAddressMode::Wrap: d3dAddressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP; break;
		case GrafAddressMode::Mirror: d3dAddressMode = D3D12_TEXTURE_ADDRESS_MODE_MIRROR; break;
		case GrafAddressMode::Clamp: d3dAddressMode = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; break;
		case GrafAddressMode::Border: d3dAddressMode = D3D12_TEXTURE_ADDRESS_MODE_BORDER; break;
		case GrafAddressMode::MirrorOnce: d3dAddressMode = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE; break;
		}
		return d3dAddressMode;
	}

	D3D12_DESCRIPTOR_RANGE_TYPE GrafUtilsDX12::GrafToD3DDescriptorType(GrafDescriptorType descriptorType)
	{
		D3D12_DESCRIPTOR_RANGE_TYPE d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE(-1);
		switch (descriptorType)
		{
		case GrafDescriptorType::ConstantBuffer: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
		case GrafDescriptorType::Sampler: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
		case GrafDescriptorType::Texture: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
		case GrafDescriptorType::Buffer: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
		case GrafDescriptorType::RWTexture: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
		case GrafDescriptorType::RWBuffer: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
		case GrafDescriptorType::AccelerationStructure: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
		case GrafDescriptorType::TextureDynamicArray: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
		case GrafDescriptorType::BufferDynamicArray: d3dDescriptorType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
		}
		return d3dDescriptorType;
	}

	D3D12_SHADER_VISIBILITY GrafUtilsDX12::GrafToD3DShaderVisibility(GrafShaderStageFlags sgaderStageFlags)
	{
		D3D12_SHADER_VISIBILITY d3dShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		switch (GrafShaderStageFlag(sgaderStageFlags))
		{
			case GrafShaderStageFlag::Vertex: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; break;
			//case GrafShaderStageFlag::Hull: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_HULL; break;
			//case GrafShaderStageFlag::Domain: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN; break;
			//case GrafShaderStageFlag::Geometry: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY; break;
			case GrafShaderStageFlag::Pixel: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; break;
			//case GrafShaderStageFlag::Amplification: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION; break;
			//case GrafShaderStageFlag::Mesh: d3dShaderVisibility = D3D12_SHADER_VISIBILITY_MESH; break;
		}
		return d3dShaderVisibility;
	}

	static const DXGI_FORMAT GrafToDXGIFormatLUT[ur_uint(GrafFormat::Count)] = {
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R8_SNORM,
		DXGI_FORMAT_R8_UINT,
		DXGI_FORMAT_R8_SINT,
		DXGI_FORMAT_R8G8_UINT,
		DXGI_FORMAT_R8G8_SINT,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_SNORM,
		DXGI_FORMAT_R8G8B8A8_UINT,
		DXGI_FORMAT_R8G8B8A8_SINT,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		DXGI_FORMAT_R16_UINT,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R16G16_UINT,
		DXGI_FORMAT_R16G16_SINT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_R16G16B16A16_UNORM,
		DXGI_FORMAT_R16G16B16A16_SNORM,
		DXGI_FORMAT_R16G16B16A16_UINT,
		DXGI_FORMAT_R16G16B16A16_SINT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R32_UINT,
		DXGI_FORMAT_R32_SINT,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_D16_UNORM,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_D32_FLOAT,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_BC1_UNORM,
		DXGI_FORMAT_BC1_UNORM_SRGB,
		DXGI_FORMAT_BC3_UNORM,
		DXGI_FORMAT_BC3_UNORM_SRGB
	};
	DXGI_FORMAT GrafUtilsDX12::GrafToDXGIFormat(GrafFormat grafFormat)
	{
		return GrafToDXGIFormatLUT[ur_uint(grafFormat)];
	}

	GrafFormat DXGIToGrafFormatLUT[] = {
		GrafFormat::Undefined,				// DXGI_FORMAT_UNKNOWN = 0,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
		GrafFormat::R32G32B32A32_SFLOAT,	// DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32B32A32_UINT = 3,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32B32A32_SINT = 4,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32B32_TYPELESS = 5,
		GrafFormat::R32G32B32_SFLOAT,		// DXGI_FORMAT_R32G32B32_FLOAT = 6,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32B32_UINT = 7,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32B32_SINT = 8,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
		GrafFormat::R16G16B16A16_SFLOAT,	// DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
		GrafFormat::R16G16B16A16_UNORM,		// DXGI_FORMAT_R16G16B16A16_UNORM = 11,
		GrafFormat::R16G16B16A16_UINT,		// DXGI_FORMAT_R16G16B16A16_UINT = 12,
		GrafFormat::R16G16B16A16_SNORM,		// DXGI_FORMAT_R16G16B16A16_SNORM = 13,
		GrafFormat::R16G16B16A16_SINT,		// DXGI_FORMAT_R16G16B16A16_SINT = 14,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32_TYPELESS = 15,
		GrafFormat::R32G32_SFLOAT,			// DXGI_FORMAT_R32G32_FLOAT = 16,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32_UINT = 17,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G32_SINT = 18,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32G8X24_TYPELESS = 19,
		GrafFormat::Unsupported,			// DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
		GrafFormat::Unsupported,			// DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R10G10B10A2_UNORM = 24,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R10G10B10A2_UINT = 25,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R11G11B10_FLOAT = 26,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
		GrafFormat::R8G8B8A8_UNORM,			// DXGI_FORMAT_R8G8B8A8_UNORM = 28,
		GrafFormat::R8G8B8A8_SRGB,			// DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
		GrafFormat::R8G8B8A8_UINT,			// DXGI_FORMAT_R8G8B8A8_UINT = 30,
		GrafFormat::R8G8B8A8_SNORM,			// DXGI_FORMAT_R8G8B8A8_SNORM = 31,
		GrafFormat::R8G8B8A8_SINT,			// DXGI_FORMAT_R8G8B8A8_SINT = 32,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16G16_TYPELESS = 33,
		GrafFormat::R16G16_SFLOAT,			// DXGI_FORMAT_R16G16_FLOAT = 34,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16G16_UNORM = 35,
		GrafFormat::R16G16_UINT,			// DXGI_FORMAT_R16G16_UINT = 36,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16G16_SNORM = 37,
		GrafFormat::R16G16_SINT,			// DXGI_FORMAT_R16G16_SINT = 38,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R32_TYPELESS = 39,
		GrafFormat::D32_SFLOAT,				// DXGI_FORMAT_D32_FLOAT = 40,
		GrafFormat::R32_SFLOAT,				// DXGI_FORMAT_R32_FLOAT = 41,
		GrafFormat::R32_UINT,				// DXGI_FORMAT_R32_UINT = 42,
		GrafFormat::R32_SINT,				// DXGI_FORMAT_R32_SINT = 43,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R24G8_TYPELESS = 44,
		GrafFormat::D24_UNORM_S8_UINT,		// DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
		GrafFormat::Unsupported,			// DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R8G8_TYPELESS = 48,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R8G8_UNORM = 49,
		GrafFormat::R8G8_UINT,				// DXGI_FORMAT_R8G8_UINT = 50,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R8G8_SNORM = 51,
		GrafFormat::R8G8_SINT,				// DXGI_FORMAT_R8G8_SINT = 52,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16_TYPELESS = 53,
		GrafFormat::R16_SFLOAT,				// DXGI_FORMAT_R16_FLOAT = 54,
		GrafFormat::D16_UNORM,				// DXGI_FORMAT_D16_UNORM = 55,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16_UNORM = 56,
		GrafFormat::R16_UINT,				// DXGI_FORMAT_R16_UINT = 57,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R16_SNORM = 58,
		GrafFormat::R16_SINT,				// DXGI_FORMAT_R16_SINT = 59,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R8_TYPELESS = 60,
		GrafFormat::R8_UNORM,				// DXGI_FORMAT_R8_UNORM = 61,
		GrafFormat::R8_UINT,				// DXGI_FORMAT_R8_UINT = 62,
		GrafFormat::R8_SNORM,				// DXGI_FORMAT_R8_SNORM = 63,
		GrafFormat::R8_SINT,				// DXGI_FORMAT_R8_SINT = 64,
		GrafFormat::Unsupported,			// DXGI_FORMAT_A8_UNORM = 65,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R1_UNORM = 66,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
		GrafFormat::Unsupported,			// DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC1_TYPELESS = 70,
		GrafFormat::BC1_RGBA_UNORM_BLOCK,	// DXGI_FORMAT_BC1_UNORM = 71,
		GrafFormat::BC1_RGBA_SRGB_BLOCK,	// DXGI_FORMAT_BC1_UNORM_SRGB = 72,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC2_TYPELESS = 73,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC2_UNORM = 74,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC2_UNORM_SRGB = 75,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC3_TYPELESS = 76,
		GrafFormat::BC3_UNORM_BLOCK,		// DXGI_FORMAT_BC3_UNORM = 77,
		GrafFormat::BC3_SRGB_BLOCK,			// DXGI_FORMAT_BC3_UNORM_SRGB = 78,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC4_TYPELESS = 79,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC4_UNORM = 80,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC4_SNORM = 81,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC5_TYPELESS = 82,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC5_UNORM = 83,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC5_SNORM = 84,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B5G6R5_UNORM = 85,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B5G5R5A1_UNORM = 86,
		GrafFormat::B8G8R8A8_UNORM,			// DXGI_FORMAT_B8G8R8A8_UNORM = 87,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B8G8R8X8_UNORM = 88,
		GrafFormat::Unsupported,			// DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
		GrafFormat::B8G8R8A8_SRGB,			// DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC6H_TYPELESS = 94,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC6H_UF16 = 95,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC6H_SF16 = 96,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC7_TYPELESS = 97,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC7_UNORM = 98,
		GrafFormat::Unsupported,			// DXGI_FORMAT_BC7_UNORM_SRGB = 99,
		GrafFormat::Unsupported,			// DXGI_FORMAT_AYUV = 100,
		GrafFormat::Unsupported,			// DXGI_FORMAT_Y410 = 101,
		GrafFormat::Unsupported,			// DXGI_FORMAT_Y416 = 102,
		GrafFormat::Unsupported,			// DXGI_FORMAT_NV12 = 103,
		GrafFormat::Unsupported,			// DXGI_FORMAT_P010 = 104,
		GrafFormat::Unsupported,			// DXGI_FORMAT_P016 = 105,
		GrafFormat::Unsupported,			// DXGI_FORMAT_420_OPAQUE = 106,
		GrafFormat::Unsupported,			// DXGI_FORMAT_YUY2 = 107,
		GrafFormat::Unsupported,			// DXGI_FORMAT_Y210 = 108,
		GrafFormat::Unsupported,			// DXGI_FORMAT_Y216 = 109,
		GrafFormat::Unsupported,			// DXGI_FORMAT_NV11 = 110,
		GrafFormat::Unsupported,			// DXGI_FORMAT_AI44 = 111,
		GrafFormat::Unsupported,			// DXGI_FORMAT_IA44 = 112,
		GrafFormat::Unsupported,			// DXGI_FORMAT_P8 = 113,
		GrafFormat::Unsupported,			// DXGI_FORMAT_A8P8 = 114,
		GrafFormat::Unsupported,			// DXGI_FORMAT_B4G4R4A4_UNORM = 115,
	};
	GrafFormat GrafUtilsDX12::DXGIToGrafFormat(DXGI_FORMAT dxgiFormat)
	{
		ur_uint formatIdx = ur_uint(dxgiFormat);
		return (formatIdx < ur_array_size(DXGIToGrafFormatLUT) ? DXGIToGrafFormatLUT[formatIdx] : GrafFormat::Unsupported);
	}

} // end namespace UnlimRealms