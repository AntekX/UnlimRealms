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
#include "3rdParty/DXCompiler/inc/dxcapi.h"
#include "comdef.h"
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "../../../Source/3rdParty/DXCompiler/lib/x64/dxcompiler.lib")

namespace UnlimRealms
{

	// hidden warnings
	#if defined(_DEBUG)
	#define UR_GRAF_DX12_DEBUG_MODE
	D3D12_MESSAGE_ID D3D12DebugLayerMessagesFilter[] =
	{
		D3D12_MESSAGE_ID_UNKNOWN,
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
	};
	#endif

	// command buffer synchronisation policy
	#define UR_GRAF_DX12_COMMAND_LIST_SYNC_DESTROY	1
	#define UR_GRAF_DX12_COMMAND_LIST_SYNC_RESET	1
	#define UR_GRAF_DX12_SLEEPZERO_WHILE_WAIT		0

	// descritor heap sizes per type
	static const ur_uint DescriptorHeapSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		8 * 1024,	// D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
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

	// default target shader model (for run time linker)
	static const std::wstring DXCLinkerShaderModel = L"6_3";

	// string utils

	void StringToWstring(const std::string& src, std::wstring& dst)
	{
		#pragma warning(push)
		#pragma warning(disable: 4996) // std::wstring_convert is depricated
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		dst = std::wstring(strConverter.from_bytes(src));
		#pragma warning(pop)
	}

	void WstringToString(const std::wstring& src, std::string& dst)
	{
		#pragma warning(push)
		#pragma warning(disable: 4996) // std::wstring_convert is depricated
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		dst = std::string(strConverter.to_bytes(src));
		#pragma warning(pop)
	}

	static std::string HResultToString(HRESULT res)
	{
		_com_error err(res);
		#ifdef _UNICODE
		std::string errStr;
		WstringToString(err.ErrorMessage(), errStr);
		return errStr;
		#else
		return std::string(err.ErrorMessage());
		#endif
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
			grafDeviceDesc.ImageDataPlacementAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
			grafDeviceDesc.ImageDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

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

	Result GrafSystemDX12::CreateShaderLib(std::unique_ptr<GrafShaderLib>& grafShaderLib)
	{
		grafShaderLib.reset(new GrafShaderLibDX12(*this));
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

	const char* GrafSystemDX12::GetShaderExtension() const
	{
		static const char* DX12ShaderExt = "cso";
		return DX12ShaderExt;
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
			std::unique_ptr<DescriptorPool> pool(new DescriptorPool());

			// CPU read / write heap
			{
				std::unique_ptr<DescriptorHeap> heap(new DescriptorHeap());
				heap->d3dDesc = {};
				heap->d3dDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(heapTypeIdx);
				heap->d3dDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				heap->d3dDesc.NumDescriptors = DescriptorHeapSize[heapTypeIdx];
				heap->descriptorIncrementSize = this->d3dDevice->GetDescriptorHandleIncrementSize(heap->d3dDesc.Type);
				heap->allocator.Init(ur_size(heap->d3dDesc.NumDescriptors));
				heap->isShaderVisible = false;
				hres = this->d3dDevice->CreateDescriptorHeap(&heap->d3dDesc, __uuidof(heap->d3dDescriptorHeap), heap->d3dDescriptorHeap);
				if (FAILED(hres))
				{
					this->Deinitialize();
					return ResultError(Failure, std::string("GrafDeviceDX12: CreateDescriptorHeap failed with HRESULT = ") + HResultToString(hres));
				}
				heap->d3dHeapStartCpuHandle = heap->d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				heap->d3dHeapStartGpuHandle = {};

				pool->descriptorHeapCPU = std::move(heap);
			}

			// CPU write / GPU read heap
			if (D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE & DescriptorHeapFlags[heapTypeIdx])
			{
				std::unique_ptr<DescriptorHeap> heap(new DescriptorHeap());
				heap->d3dDesc = {};
				heap->d3dDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE(heapTypeIdx);
				heap->d3dDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				heap->d3dDesc.NumDescriptors = DescriptorHeapSize[heapTypeIdx];
				heap->descriptorIncrementSize = this->d3dDevice->GetDescriptorHandleIncrementSize(heap->d3dDesc.Type);
				heap->allocator.Init(ur_size(heap->d3dDesc.NumDescriptors));
				heap->isShaderVisible = true;
				hres = this->d3dDevice->CreateDescriptorHeap(&heap->d3dDesc, __uuidof(heap->d3dDescriptorHeap), heap->d3dDescriptorHeap);
				if (FAILED(hres))
				{
					this->Deinitialize();
					return ResultError(Failure, std::string("GrafDeviceDX12: CreateDescriptorHeap failed with HRESULT = ") + HResultToString(hres));
				}
				heap->d3dHeapStartCpuHandle = heap->d3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				heap->d3dHeapStartGpuHandle = heap->d3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

				pool->descriptorHeapGPU = std::move(heap);
			}

			this->descriptorPool[heapTypeIdx] = std::move(pool);
		}

		// setup D3D12 debug layer messages filter

		#if defined(UR_GRAF_DX12_DEBUG_MODE)
		shared_ref<ID3D12InfoQueue> d3dInfoQueue;
		hres = this->d3dDevice->QueryInterface<ID3D12InfoQueue>(d3dInfoQueue);
		if (SUCCEEDED(hres))
		{
			D3D12_INFO_QUEUE_FILTER d3dInfoQueueFilter = {};
			d3dInfoQueueFilter.DenyList.NumIDs = _countof(D3D12DebugLayerMessagesFilter);
			d3dInfoQueueFilter.DenyList.pIDList = D3D12DebugLayerMessagesFilter;
			d3dInfoQueue->AddStorageFilterEntries(&d3dInfoQueueFilter);
		}
		#endif

		return Result(Success);
	}

	Result GrafDeviceDX12::Record(GrafCommandList* grafCommandList)
	{
		// NOTE: currently all command lists are recorded to graphics queue

		Result res = NotFound;

		if (this->graphicsQueue != ur_null)
		{
			//std::lock_guard<std::mutex> lock(this->graphicsQueue->recordedCommandListsMutex);
			GrafCommandListDX12* grafCommandListDX12 = static_cast<GrafCommandListDX12*>(grafCommandList);
			ur_assert(grafCommandListDX12->submitFenceValue <= this->graphicsQueue->nextSubmitFenceValue);
			this->graphicsQueue->recordedCommandLists.push_back(grafCommandList);
			res = Success;
		}

		this->graphicsQueue->submitMutex.unlock();

		return Result(res);
	}

	Result GrafDeviceDX12::Submit()
	{
		// NOTE: support submission to different queue families
		// currently everything's done on the graphics queue

		if (this->graphicsQueue)
		{
			//this->graphicsQueue->commandPoolsMutex.lock(); // lock pools list modification
			//for (auto& pool : this->graphicsQueue->commandPools) pool.second->commandAllocatorsMutex.lock(); // lock per thread allocators write access
			//this->graphicsQueue->recordedCommandListsMutex.lock(); // lock recording
			this->graphicsQueue->submitMutex.lock();
			
			// execute command lists
			for (auto& grafCommandList : this->graphicsQueue->recordedCommandLists)
			{
				GrafCommandListDX12* grafCommandListDX12 = static_cast<GrafCommandListDX12*>(grafCommandList);
				GrafDeviceDX12::CommandAllocator* grafCmdAllocator = grafCommandListDX12->GetCommandAllocator();
				ID3D12GraphicsCommandList4* d3dGraphicsCommandList = grafCommandListDX12->GetD3DCommandList();

				// execute
				ID3D12CommandList* d3dCommandLists[] = { d3dGraphicsCommandList };
				this->graphicsQueue->d3dQueue->ExecuteCommandLists(1, d3dCommandLists);
				
				grafCommandListDX12->submitFenceValue = this->graphicsQueue->nextSubmitFenceValue;
				grafCommandListDX12->closed = true; // make sure GrafCommandList::Wait() fails ater opening/begin and until submitted & executed
				grafCmdAllocator->submitFenceValue = this->graphicsQueue->nextSubmitFenceValue;
			}
			this->graphicsQueue->recordedCommandLists.clear();

			// signal submission fence
			this->graphicsQueue->d3dQueue->Signal(this->graphicsQueue->d3dSubmitFence, this->graphicsQueue->nextSubmitFenceValue);
			this->graphicsQueue->nextSubmitFenceValue += 1;

			//this->graphicsQueue->recordedCommandListsMutex.unlock();
			//for (auto& pool : this->graphicsQueue->commandPools) pool.second->commandAllocatorsMutex.unlock();
			//this->graphicsQueue->commandPoolsMutex.unlock();
			this->graphicsQueue->submitMutex.unlock();
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

		this->graphicsQueue->submitMutex.lock();

		// get an existing pool for current thread or create a new one

		CommandAllocatorPool* commandAllocatorPool = ur_null;
		std::thread::id thisThreadId = std::this_thread::get_id();
		//this->graphicsQueue->commandPoolsMutex.lock();
		const auto& poolIter = this->graphicsQueue->commandPools.find(thisThreadId);
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
		//this->graphicsQueue->commandPoolsMutex.unlock();

		// get a reusable allocator or create a mew one

		CommandAllocator* commandAllocator = ur_null;
		ur_uint64 completedFenceValue = this->graphicsQueue->d3dSubmitFence->GetCompletedValue();
		//commandAllocatorPool->commandAllocatorsMutex.lock();
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

		// reset allocator the first time it is accessed after the last submit

		if (commandAllocator->resetFenceValue <= commandAllocator->submitFenceValue)
		{
			HRESULT hres = commandAllocator->d3dCommandAllocator->Reset();
			if (FAILED(hres))
			{
				//commandAllocatorPool->commandAllocatorsMutex.unlock();
				LogError(std::string("GrafCommandListDX12: d3dCommandAllocator->Reset() failed with HRESULT = ") + HResultToString(hres));
				return nullptr;
			}
			commandAllocator->resetFenceValue = commandAllocator->submitFenceValue + 1;
		}

		//commandAllocatorPool->commandAllocatorsMutex.unlock();

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

	GrafDescriptorHandleDX12 GrafDeviceDX12::AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE type, ur_size count)
	{
		GrafDescriptorHandleDX12 handle = {};

		if (type >= D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
			return handle; // invalid type

		DescriptorPool* pool = this->descriptorPool[ur_uint(type)].get();
		if (nullptr == pool || nullptr == pool->descriptorHeapCPU.get())
			return handle; // not initialized

		DescriptorHeap* heap = pool->descriptorHeapCPU.get();
		handle.allocation = heap->allocator.Allocate(count);
		if (0 == handle.allocation.Size)
			return handle; // out of memory

		handle.heap = heap;
		handle.cpuHandle.ptr = heap->d3dHeapStartCpuHandle.ptr + SIZE_T(handle.allocation.Offset * heap->descriptorIncrementSize);

		return handle;
	}

	GrafDescriptorHandleDX12 GrafDeviceDX12::AllocateDescriptorRangeGPU(D3D12_DESCRIPTOR_HEAP_TYPE type, ur_size count)
	{
		GrafDescriptorHandleDX12 handle = {};

		if (type >= D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
			return handle; // invalid type

		DescriptorPool* pool = this->descriptorPool[ur_uint(type)].get();
		if (nullptr == pool || nullptr == pool->descriptorHeapGPU.get())
			return handle; // not initialized

		DescriptorHeap* heap = pool->descriptorHeapGPU.get();
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
		this->closed = true;
		this->submitFenceValue = 0;
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
		this->closed = true;
		this->submitFenceValue = 0;
		this->commandAllocator = ur_null;
		
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

		if (!this->closed)
			return Result(Success); // already open

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());
		
		#if (UR_GRAF_DX12_COMMAND_LIST_SYNC_RESET)
		// make sure this command list can be reused safely
		this->Wait(ur_uint64(-1));
		#endif

		// request allocator from device pool

		this->commandAllocator = grafDeviceDX12->GetGraphicsCommandAllocator();
		if (ur_null == this->commandAllocator)
		{
			return ResultError(Failure, std::string("GrafCommandListDX12: failed to initialize, invalid command allocator"));
		}

		// update allocator's fence

		//this->commandAllocator->pool->commandAllocatorsMutex.lock();

		if (this->closed)
		{
			// reset command list
			this->closed = false;
			HRESULT hres = this->d3dCommandList->Reset(this->commandAllocator->d3dCommandAllocator, ur_null);
			if (FAILED(hres))
			{
				//this->commandAllocator->pool->commandAllocatorsMutex.unlock();
				return ResultError(Failure, std::string("GrafCommandListDX12: d3dCommandList->Reset() failed with HRESULT = ") + HResultToString(hres));
			}
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::End()
	{
		// close command list
		HRESULT hres = this->d3dCommandList->Close();
		//this->commandAllocator->pool->commandAllocatorsMutex.unlock();
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
		// note: waiting for closed to be true to prevent command list from being reused after opening and before submission in caching strategies
		// (current cache implementation in GrafRenderer checks for Wait() to decide whether a list is reusable)
		// this means that any open command list must be submitted, otherwise Wait() will never succeed
		while (!this->closed || this->submitFenceValue > grafDeviceDX12->graphicsQueue->d3dSubmitFence->GetCompletedValue())
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

		static_cast<GrafBufferDX12*>(grafBuffer)->SetState(dstState);
		
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

		return Result(Success);
	}

	Result GrafCommandListDX12::ImageMemoryBarrier(GrafImageSubresource* grafImageSubresource, GrafImageState srcState, GrafImageState dstState)
	{
		if (ur_null == grafImageSubresource)
			return Result(InvalidArgs);

		srcState = (GrafImageState::Current == srcState ? grafImageSubresource->GetState() : srcState);
		D3D12_RESOURCE_STATES d3dStateSrc = GrafUtilsDX12::GrafToD3DImageState(srcState);
		D3D12_RESOURCE_STATES d3dStateDst = GrafUtilsDX12::GrafToD3DImageState(dstState);
		if (d3dStateSrc == d3dStateDst)
			return Result(Success);

		const GrafImageDX12* grafImageDX12 = static_cast<const GrafImageDX12*>(grafImageSubresource->GetImage());
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
			d3dBarrier.Transition.StateBefore = d3dStateSrc;
			d3dBarrier.Transition.StateAfter = d3dStateDst;
		}

		this->d3dCommandList->ResourceBarrier(UINT(subresCount), d3dBarriers.data());

		static_cast<GrafImageSubresourceDX12*>(grafImageSubresource)->SetState(dstState);

		return Result(Success);
	}

	Result GrafCommandListDX12::SetFenceState(GrafFence* grafFence, GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		GrafImageDX12* grafImageDX12 = static_cast<GrafImageDX12*>(grafImage);

		return ClearColorImage(grafImageDX12->GetDefaultSubresource(), clearValue);
	}

	Result GrafCommandListDX12::ClearColorImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue)
	{
		GrafImageSubresourceDX12* grafImageSubresourceDX12 = static_cast<GrafImageSubresourceDX12*>(grafImageSubresource);

		float colorRGBA[4] = { clearValue.f32[0], clearValue.f32[1], clearValue.f32[2], clearValue.f32[3] };

		this->d3dCommandList->ClearRenderTargetView(grafImageSubresourceDX12->GetRTVDescriptorHandle().GetD3DHandleCPU(), colorRGBA, 0, nullptr);

		return Result(Success);
	}

	Result GrafCommandListDX12::ClearDepthStencilImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		GrafImageDX12* grafImageDX12 = static_cast<GrafImageDX12*>(grafImage);

		return ClearDepthStencilImage(grafImageDX12->GetDefaultSubresource(), clearValue);
	}

	Result GrafCommandListDX12::ClearDepthStencilImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue)
	{
		GrafImageSubresourceDX12* grafImageSubresourceDX12 = static_cast<GrafImageSubresourceDX12*>(grafImageSubresource);

		float colorRGBA[4] = { clearValue.f32[0], clearValue.f32[1], clearValue.f32[2], clearValue.f32[3] };

		D3D12_CLEAR_FLAGS d3dClearFlags = (D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
		this->d3dCommandList->ClearDepthStencilView(grafImageSubresourceDX12->GetDSVDescriptorHandle().GetD3DHandleCPU(), d3dClearFlags, clearValue.f32[0], (UINT8)clearValue.u32[1], 0, nullptr);

		return Result(Success);
	}

	Result GrafCommandListDX12::SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect)
	{
		D3D12_VIEWPORT d3dViewport;
		d3dViewport.TopLeftX = (FLOAT)grafViewportDesc.X;
		d3dViewport.TopLeftY = (FLOAT)grafViewportDesc.Y;
		d3dViewport.Width = (FLOAT)grafViewportDesc.Width;
		d3dViewport.Height = (FLOAT)grafViewportDesc.Height;
		d3dViewport.MinDepth = (FLOAT)grafViewportDesc.Near;
		d3dViewport.MaxDepth = (FLOAT)grafViewportDesc.Far;

		this->d3dCommandList->RSSetViewports(1, &d3dViewport);

		if (resetScissorsRect)
		{
			RectI scissorsRect;
			scissorsRect.Min.x = ur_int(grafViewportDesc.X);
			scissorsRect.Min.y = ur_int(grafViewportDesc.Y);
			scissorsRect.Max.x = ur_int(grafViewportDesc.X + grafViewportDesc.Width);
			scissorsRect.Max.y = ur_int(grafViewportDesc.Y + grafViewportDesc.Height);

			return SetScissorsRect(scissorsRect);
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::SetScissorsRect(const RectI& scissorsRect)
	{
		D3D12_RECT d3dRect;
		d3dRect.left = (LONG)scissorsRect.Min.x;
		d3dRect.top = (LONG)scissorsRect.Min.y;
		d3dRect.right = (LONG)scissorsRect.Max.x;
		d3dRect.bottom = (LONG)scissorsRect.Max.y;

		this->d3dCommandList->RSSetScissorRects(1, &d3dRect);

		return Result(Success);
	}

	Result GrafCommandListDX12::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dColorTargetDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		D3D12_CPU_DESCRIPTOR_HANDLE d3dDepthStencilDescriptor;
		ur_uint colorTargetCount = 0;
		ur_uint depthStencilCount = 0;
		for (ur_uint imageIdx = 0; imageIdx < grafRenderTarget->GetImageCount(); ++imageIdx)
		{
			GrafImageDX12* grafImageDX12 = static_cast<GrafImageDX12*>(grafRenderTarget->GetImage(imageIdx));
			GrafImageSubresourceDX12* grafImageSubresourceDX12 = static_cast<GrafImageSubresourceDX12*>(grafImageDX12->GetDefaultSubresource());
			if (ur_uint(GrafImageUsageFlag::ColorRenderTarget) & grafImageDX12->GetDesc().Usage)
			{
				d3dColorTargetDescriptors[colorTargetCount++] = grafImageSubresourceDX12->GetRTVDescriptorHandle().GetD3DHandleCPU();
				if (rtClearValues != nullptr && grafRenderPass->GetImageDesc(imageIdx).LoadOp == GrafRenderPassDataOp::Clear)
				{
					this->ClearColorImage(grafImageSubresourceDX12, rtClearValues[imageIdx]);
				}
			}
			if (ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget) & grafImageDX12->GetDesc().Usage)
			{
				d3dDepthStencilDescriptor = grafImageSubresourceDX12->GetDSVDescriptorHandle().GetD3DHandleCPU();
				++depthStencilCount;
				if (rtClearValues != nullptr && grafRenderPass->GetImageDesc(imageIdx).LoadOp == GrafRenderPassDataOp::Clear)
				{
					this->ClearDepthStencilImage(grafImageSubresourceDX12, rtClearValues[imageIdx]);
				}
			}
		}

		this->d3dCommandList->OMSetRenderTargets((UINT)colorTargetCount,
			(colorTargetCount > 0 ? d3dColorTargetDescriptors : NULL), FALSE,
			(depthStencilCount > 0 ? &d3dDepthStencilDescriptor : NULL));

		return Result(Success);
	}

	Result GrafCommandListDX12::EndRenderPass()
	{
		return Result(Success);
	}

	Result GrafCommandListDX12::BindPipeline(GrafPipeline* grafPipeline)
	{
		if (ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafPipelineDX12* grafPipelineDX12 = static_cast<GrafPipelineDX12*>(grafPipeline);

		this->d3dCommandList->SetPipelineState(grafPipelineDX12->GetD3DPipelineState());

		this->d3dCommandList->IASetPrimitiveTopology(grafPipelineDX12->GetD3DPrimitiveTopology());

		this->d3dCommandList->SetGraphicsRootSignature(grafPipelineDX12->GetD3DRootSignature());

		return Result(Success);
	}

	Result GrafCommandListDX12::BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		if (ur_null == descriptorTable || ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafDescriptorTableDX12* descriptorTableDX12 = static_cast<GrafDescriptorTableDX12*>(descriptorTable);

		// bind descriptor heaps

		ur_uint descriptorHeapCount = 0;
		ID3D12DescriptorHeap* d3dDescriptorHeaps[2];
		if (descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().IsValid())
		{
			d3dDescriptorHeaps[descriptorHeapCount++] = descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().GetHeap()->GetD3DDescriptorHeap();
		}
		if (descriptorTableDX12->GetSamplerDescriptorHeapHandle().IsValid())
		{
			d3dDescriptorHeaps[descriptorHeapCount++] = descriptorTableDX12->GetSamplerDescriptorHeapHandle().GetHeap()->GetD3DDescriptorHeap();
		}

		this->d3dCommandList->SetDescriptorHeaps((UINT)descriptorHeapCount, d3dDescriptorHeaps);

		// set table offsets in heaps

		ur_uint rootParameterIdx = 0;
		if (descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().IsValid())
		{
			this->d3dCommandList->SetGraphicsRootDescriptorTable(rootParameterIdx++, descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().GetD3DHandleGPU());
		}
		if (descriptorTableDX12->GetSamplerDescriptorHeapHandle().IsValid())
		{
			this->d3dCommandList->SetGraphicsRootDescriptorTable(rootParameterIdx++, descriptorTableDX12->GetSamplerDescriptorHeapHandle().GetD3DHandleGPU());
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset)
	{
		if (ur_null == grafVertexBuffer)
			return Result(InvalidArgs);

		this->d3dCommandList->IASetVertexBuffers(0, 1, static_cast<GrafBufferDX12*>(grafVertexBuffer)->GetD3DVertexBufferView());

		return Result(Success);
	}

	Result GrafCommandListDX12::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset)
	{
		if (ur_null == grafIndexBuffer)
			return Result(InvalidArgs);

		this->d3dCommandList->IASetIndexBuffer(static_cast<GrafBufferDX12*>(grafIndexBuffer)->GetD3DIndexBufferView());

		return Result(Success);
	}

	Result GrafCommandListDX12::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
	{
		this->d3dCommandList->DrawInstanced((UINT)vertexCount, (UINT)instanceCount, (UINT)firstVertex, (UINT)firstInstance);

		return Result(Success);
	}

	Result GrafCommandListDX12::DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance)
	{
		this->d3dCommandList->DrawIndexedInstanced((UINT)indexCount, (UINT)instanceCount, (UINT)firstIndex, (UINT)firstVertex, (UINT)firstInstance);

		return Result(Success);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		GrafBufferDX12* srcBufferDX12 = static_cast<GrafBufferDX12*>(srcBuffer);
		GrafBufferDX12* dstBufferDX12 = static_cast<GrafBufferDX12*>(dstBuffer);

		this->d3dCommandList->CopyBufferRegion(dstBufferDX12->GetD3DResource(), (UINT64)dstOffset, srcBufferDX12->GetD3DResource(), (UINT64)srcOffset, (UINT64)dataSize);;

		return Result(Success);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset, BoxI imageRegion)
	{
		if (ur_null == srcBuffer || ur_null == dstImage)
			return Result(InvalidArgs);

		GrafImageDX12* dstImageDX12 = static_cast<GrafImageDX12*>(dstImage);
		return Copy(srcBuffer, dstImageDX12->GetDefaultSubresource(), bufferOffset, imageRegion);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafImageSubresource* dstImageSubresource, ur_size bufferOffset, BoxI imageRegion)
	{
		if (ur_null == srcBuffer || ur_null == dstImageSubresource)
			return Result(InvalidArgs);

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());
		const GrafBufferDX12* srcBufferDX12 = static_cast<const GrafBufferDX12*>(srcBuffer);
		const GrafImageDX12* dstImageDX12 = static_cast<const GrafImageDX12*>(dstImageSubresource->GetImage());

		D3D12_RESOURCE_DESC d3dDstResDesc = dstImageDX12->GetD3DResource()->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT d3dDstResFootprint;
		UINT dstResNumRows = 0;
		UINT64 dstResRowSizeInBytes = 0;
		UINT64 dstResTotalBytes = 0;
		grafDeviceDX12->GetD3DDevice()->GetCopyableFootprints(&d3dDstResDesc,
			(UINT)dstImageSubresource->GetDesc().BaseMipLevel, (UINT)dstImageSubresource->GetDesc().LevelCount, 0,
			&d3dDstResFootprint, &dstResNumRows, &dstResRowSizeInBytes, &dstResTotalBytes);

		D3D12_TEXTURE_COPY_LOCATION d3dDstLocation = {};
		d3dDstLocation.pResource = dstImageDX12->GetD3DResource();
		d3dDstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		d3dDstLocation.SubresourceIndex = (UINT)dstImageSubresource->GetDesc().BaseMipLevel;

		D3D12_TEXTURE_COPY_LOCATION d3dSrcLocation = {};
		d3dSrcLocation.pResource = srcBufferDX12->GetD3DResource();
		d3dSrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		d3dSrcLocation.PlacedFootprint.Offset = (UINT64)bufferOffset;
		d3dSrcLocation.PlacedFootprint.Footprint.Format = d3dDstResFootprint.Footprint.Format;
		d3dSrcLocation.PlacedFootprint.Footprint.Width = (UINT)(imageRegion.SizeX() > 0 ? imageRegion.SizeX() : d3dDstResFootprint.Footprint.Width);
		d3dSrcLocation.PlacedFootprint.Footprint.Height = (UINT)(imageRegion.SizeY() > 0 ? imageRegion.SizeY() : d3dDstResFootprint.Footprint.Height);
		d3dSrcLocation.PlacedFootprint.Footprint.Depth = (UINT)(imageRegion.SizeZ() > 0 ? imageRegion.SizeZ() : d3dDstResFootprint.Footprint.Depth);
		d3dSrcLocation.PlacedFootprint.Footprint.RowPitch = d3dDstResFootprint.Footprint.RowPitch;

		this->d3dCommandList->CopyTextureRegion(&d3dDstLocation, (UINT)imageRegion.Min.x, (UINT)imageRegion.Min.y, (UINT)imageRegion.Min.z,
			&d3dSrcLocation, nullptr);

		return Result(Success);
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
		if (ur_null == srcImage || ur_null == dstImage)
			return Result(InvalidArgs);

		GrafImageDX12* srcImageDX12 = static_cast<GrafImageDX12*>(srcImage);
		GrafImageDX12* dstImageDX12 = static_cast<GrafImageDX12*>(dstImage);
		return Copy(srcImageDX12->GetDefaultSubresource(), dstImageDX12->GetDefaultSubresource(), srcRegion, dstRegion);
	}

	Result GrafCommandListDX12::Copy(GrafImageSubresource* srcImageSubresource, GrafImageSubresource* dstImageSubresource, BoxI srcRegion, BoxI dstRegion)
	{
		if (ur_null == srcImageSubresource || ur_null == dstImageSubresource)
			return Result(InvalidArgs);

		const GrafImageDX12* srcImageDX12 = static_cast<const GrafImageDX12*>(srcImageSubresource->GetImage());
		const GrafImageDX12* dstImageDX12 = static_cast<const GrafImageDX12*>(dstImageSubresource->GetImage());

		D3D12_TEXTURE_COPY_LOCATION d3dSrcLocation = {};
		d3dSrcLocation.pResource = srcImageDX12->GetD3DResource();
		d3dSrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		d3dSrcLocation.SubresourceIndex = UINT(srcImageSubresource->GetDesc().BaseMipLevel);

		D3D12_TEXTURE_COPY_LOCATION d3dDstLocation = {};
		d3dDstLocation.pResource = dstImageDX12->GetD3DResource();
		d3dDstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		d3dDstLocation.SubresourceIndex = UINT(dstImageSubresource->GetDesc().BaseMipLevel);

		D3D12_BOX d3dSrcBox;
		d3dSrcBox.left = (UINT)srcRegion.Min.x;
		d3dSrcBox.top = (UINT)srcRegion.Min.y;
		d3dSrcBox.front = (UINT)srcRegion.Min.z;
		d3dSrcBox.right = (UINT)(srcRegion.SizeX() > 0 ? srcRegion.Max.x : srcImageDX12->GetDesc().Size.x);
		d3dSrcBox.bottom = (UINT)(srcRegion.SizeY() > 0 ? srcRegion.Max.y : srcImageDX12->GetDesc().Size.y);
		d3dSrcBox.back = (UINT)(srcRegion.SizeZ() > 0 ? srcRegion.Max.z : srcImageDX12->GetDesc().Size.z);

		this->d3dCommandList->CopyTextureRegion(&d3dDstLocation, (UINT)dstRegion.Min.x, (UINT)dstRegion.Min.y, (UINT)dstRegion.Min.z,
			&d3dSrcLocation, &d3dSrcBox);

		return Result(Success);
	}

	Result GrafCommandListDX12::BindComputePipeline(GrafComputePipeline* grafPipeline)
	{
		if (ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafComputePipelineDX12* grafPipelineDX12 = static_cast<GrafComputePipelineDX12*>(grafPipeline);

		this->d3dCommandList->SetPipelineState(grafPipelineDX12->GetD3DPipelineState());

		this->d3dCommandList->SetComputeRootSignature(grafPipelineDX12->GetD3DRootSignature());

		return Result(Success);
	}

	Result GrafCommandListDX12::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline)
	{
		if (ur_null == descriptorTable || ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafDescriptorTableDX12* descriptorTableDX12 = static_cast<GrafDescriptorTableDX12*>(descriptorTable);

		// bind descriptor heaps

		ur_uint descriptorHeapCount = 0;
		ID3D12DescriptorHeap* d3dDescriptorHeaps[2];
		if (descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().IsValid())
		{
			d3dDescriptorHeaps[descriptorHeapCount++] = descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().GetHeap()->GetD3DDescriptorHeap();
		}
		if (descriptorTableDX12->GetSamplerDescriptorHeapHandle().IsValid())
		{
			d3dDescriptorHeaps[descriptorHeapCount++] = descriptorTableDX12->GetSamplerDescriptorHeapHandle().GetHeap()->GetD3DDescriptorHeap();
		}

		this->d3dCommandList->SetDescriptorHeaps((UINT)descriptorHeapCount, d3dDescriptorHeaps);

		// set table offsets in heaps

		ur_uint rootParameterIdx = 0;
		if (descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().IsValid())
		{
			this->d3dCommandList->SetComputeRootDescriptorTable(rootParameterIdx++, descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().GetD3DHandleGPU());
		}
		if (descriptorTableDX12->GetSamplerDescriptorHeapHandle().IsValid())
		{
			this->d3dCommandList->SetComputeRootDescriptorTable(rootParameterIdx++, descriptorTableDX12->GetSamplerDescriptorHeapHandle().GetD3DHandleGPU());
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ)
	{
		this->d3dCommandList->Dispatch((UINT)groupCountX, (UINT)groupCountY, (UINT)groupCountZ);

		return Result(Success);
	}

	Result GrafCommandListDX12::BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount)
	{
		GrafAccelerationStructureDX12* dstStructureDX12 = static_cast<GrafAccelerationStructureDX12*>(dstStructrure);
		if (ur_null == dstStructureDX12 || ur_null == dstStructureDX12->GetScratchBuffer() || ur_null == geometryData || 0 == geometryCount)
			return Result(InvalidArgs);

		// fill geometry data structures

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3dBuildDesc = {};
		d3dBuildDesc.DestAccelerationStructureData = D3D12_GPU_VIRTUAL_ADDRESS(dstStructureDX12->GetDeviceAddress());
		d3dBuildDesc.SourceAccelerationStructureData = D3D12_GPU_VIRTUAL_ADDRESS(0);
		d3dBuildDesc.ScratchAccelerationStructureData = D3D12_GPU_VIRTUAL_ADDRESS(dstStructureDX12->GetScratchBuffer()->GetDeviceAddress());
		d3dBuildDesc.Inputs.Type = GrafUtilsDX12::GrafToD3DAccelerationStructureType(dstStructureDX12->GetStructureType());
		d3dBuildDesc.Inputs.Flags = GrafUtilsDX12::GrafToD3DAccelerationStructureBuildFlags(dstStructureDX12->GetStructureBuildFlags());
		d3dBuildDesc.Inputs.NumDescs = geometryCount;
		d3dBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> d3dGeometryDescArray;
		if (GrafAccelerationStructureType::TopLevel == dstStructureDX12->GetStructureType())
		{
			for (ur_uint32 igeom = 0; igeom < geometryCount; ++igeom)
			{
				GrafAccelerationStructureGeometryData& geometry = geometryData[igeom];
				if (ur_null == geometry.InstancesData || 0 == geometry.InstancesData->DeviceAddress)
				{
					LogError(std::string("GrafCommandListDX12::BuildAccelerationStructure: top level AS must have instance data with valid device address"));
					return Result(InvalidArgs);
				}
				if (geometry.InstancesData->IsPointersArray)
				{
					LogError(std::string("GrafCommandListDX12::BuildAccelerationStructure: IsPointersArray per geomtry is not supported"));
					return Result(InvalidArgs);
				}
				d3dBuildDesc.Inputs.InstanceDescs = geometryData->InstancesData->DeviceAddress;
			}
		}
		else if (GrafAccelerationStructureType::BottomLevel == dstStructureDX12->GetStructureType())
		{
			d3dGeometryDescArray.resize(geometryCount);
			for (ur_uint32 igeom = 0; igeom < geometryCount; ++igeom)
			{
				GrafAccelerationStructureGeometryData& geometry = geometryData[igeom];
				D3D12_RAYTRACING_GEOMETRY_DESC& d3dGeometryDesc = d3dGeometryDescArray[igeom];
				d3dGeometryDesc = {};
				d3dGeometryDesc.Type = GrafUtilsDX12::GrafToD3DAccelerationStructureGeometryType(geometry.GeometryType);
				d3dGeometryDesc.Flags = GrafUtilsDX12::GrafToD3DAccelerationStructureGeometryFlags(geometry.GeometryFlags);
				if (GrafAccelerationStructureGeometryType::Triangles == geometry.GeometryType)
				{
					if (ur_null == geometry.TrianglesData || 0 == geometry.TrianglesData->VerticesDeviceAddress)
					{
						LogError(std::string("GrafCommandListDX12::BuildAccelerationStructure: bottom level AS triangles data is invalid"));
						return Result(InvalidArgs);
					}
					D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& d3dGeometryTriangles = d3dGeometryDesc.Triangles;
					d3dGeometryTriangles.IndexFormat = GrafUtilsDX12::GrafToDXGIFormat(geometry.TrianglesData->IndexType);
					d3dGeometryTriangles.VertexFormat = GrafUtilsDX12::GrafToDXGIFormat(geometry.TrianglesData->VertexFormat);
					d3dGeometryTriangles.IndexCount = geometry.PrimitiveCount * 3;
					d3dGeometryTriangles.VertexCount = geometry.TrianglesData->VertexCount;
					d3dGeometryTriangles.VertexBuffer.StrideInBytes = geometry.TrianglesData->VertexStride;
					d3dGeometryTriangles.VertexBuffer.StartAddress = geometry.TrianglesData->VerticesDeviceAddress;
					d3dGeometryTriangles.IndexBuffer = geometry.TrianglesData->IndicesDeviceAddress;
					d3dGeometryTriangles.Transform3x4 = geometry.TrianglesData->TransformsDeviceAddress;
				}
				else if (GrafAccelerationStructureGeometryType::AABBs == geometry.GeometryType)
				{
					if (ur_null == geometry.AabbsData || 0 == geometry.AabbsData->DeviceAddress)
					{
						LogError(std::string("GrafCommandListDX12::BuildAccelerationStructure: bottom level AS aabbs data is invalid"));
						return Result(InvalidArgs);
					}
					D3D12_RAYTRACING_GEOMETRY_AABBS_DESC& d3dGeometryAabbs = d3dGeometryDesc.AABBs;
					d3dGeometryAabbs.AABBCount = geometry.PrimitiveCount;
					d3dGeometryAabbs.AABBs.StrideInBytes = geometry.AabbsData->Stride;
					d3dGeometryAabbs.AABBs.StartAddress = D3D12_GPU_VIRTUAL_ADDRESS(geometry.AabbsData->DeviceAddress);
				}
			}
			d3dBuildDesc.Inputs.pGeometryDescs = d3dGeometryDescArray.data();
		}

		// build

		this->d3dCommandList->BuildRaytracingAccelerationStructure(&d3dBuildDesc, 0, ur_null);

		return Result(Success);
	}

	Result GrafCommandListDX12::BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline)
	{
		if (ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafRayTracingPipelineDX12* grafPipelineDX12 = static_cast<GrafRayTracingPipelineDX12*>(grafPipeline);

		this->d3dCommandList->SetPipelineState1(grafPipelineDX12->GetD3DStateObject());

		this->d3dCommandList->SetComputeRootSignature(grafPipelineDX12->GetD3DRootSignature());

		return Result(Success);
	}

	Result GrafCommandListDX12::BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline)
	{
		if (ur_null == descriptorTable || ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafDescriptorTableDX12* descriptorTableDX12 = static_cast<GrafDescriptorTableDX12*>(descriptorTable);

		// bind descriptor heaps

		ur_uint descriptorHeapCount = 0;
		ID3D12DescriptorHeap* d3dDescriptorHeaps[2];
		if (descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().IsValid())
		{
			d3dDescriptorHeaps[descriptorHeapCount++] = descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().GetHeap()->GetD3DDescriptorHeap();
		}
		if (descriptorTableDX12->GetSamplerDescriptorHeapHandle().IsValid())
		{
			d3dDescriptorHeaps[descriptorHeapCount++] = descriptorTableDX12->GetSamplerDescriptorHeapHandle().GetHeap()->GetD3DDescriptorHeap();
		}

		this->d3dCommandList->SetDescriptorHeaps((UINT)descriptorHeapCount, d3dDescriptorHeaps);

		// set table offsets in heaps

		ur_uint rootParameterIdx = 0;
		if (descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().IsValid())
		{
			this->d3dCommandList->SetComputeRootDescriptorTable(rootParameterIdx++, descriptorTableDX12->GetSrvUavCbvDescriptorHeapHandle().GetD3DHandleGPU());
		}
		if (descriptorTableDX12->GetSamplerDescriptorHeapHandle().IsValid())
		{
			this->d3dCommandList->SetComputeRootDescriptorTable(rootParameterIdx++, descriptorTableDX12->GetSamplerDescriptorHeapHandle().GetD3DHandleGPU());
		}

		return Result(Success);
	}

	Result GrafCommandListDX12::DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
		const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
		const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable)
	{
		static const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE NullD3DRegion = { 0, 0, 0 };
		auto FillD3DRegion = [](D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE& d3dRegion, const GrafStridedBufferRegionDesc* grafStridedBufferRegion) -> void
		{
			if (grafStridedBufferRegion)
			{
				d3dRegion.StartAddress = (D3D12_GPU_VIRTUAL_ADDRESS)(grafStridedBufferRegion->BufferPtr ? grafStridedBufferRegion->BufferPtr->GetDeviceAddress() + grafStridedBufferRegion->Offset : 0);
				d3dRegion.StrideInBytes = (UINT64)grafStridedBufferRegion->Stride;
				d3dRegion.SizeInBytes = (UINT64)grafStridedBufferRegion->Size;
			}
			else
			{
				d3dRegion = NullD3DRegion;
			}
		};

		D3D12_DISPATCH_RAYS_DESC d3dDispatchDesc = {};
		d3dDispatchDesc.Width = width;
		d3dDispatchDesc.Height = height;
		d3dDispatchDesc.Depth = depth;
		if (rayGenShaderTable != ur_null)
		{
			d3dDispatchDesc.RayGenerationShaderRecord.StartAddress = (rayGenShaderTable->BufferPtr ? rayGenShaderTable->BufferPtr->GetDeviceAddress() + rayGenShaderTable->Offset : 0);
			d3dDispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenShaderTable->Size;
		}
		FillD3DRegion(d3dDispatchDesc.MissShaderTable, missShaderTable);
		FillD3DRegion(d3dDispatchDesc.HitGroupTable, hitShaderTable);
		FillD3DRegion(d3dDispatchDesc.CallableShaderTable, callableShaderTable);

		this->d3dCommandList->DispatchRays(&d3dDispatchDesc);

		return Result(Success);
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
	}

	GrafCanvasDX12::~GrafCanvasDX12()
	{
		this->Deinitialize();
	}

	Result GrafCanvasDX12::Deinitialize()
	{
		if (this->GetGrafDevice())
		{
			// make sure resources are no longer used on GPU
			this->GetGrafDevice()->Submit();
			this->GetGrafDevice()->WaitIdle();
		}

		this->imageTransitionCmdListBegin.clear();
		this->imageTransitionCmdListEnd.clear();
		
		this->swapChainImages.clear();
		if (!this->dxgiSwapChain.empty())
		{
			this->dxgiSwapChain.reset(nullptr);
			LogNoteGrafDbg("GrafCanvasDX12: swap chain destroyed");
		}

		this->swapChainImageCount = 0;
		this->swapChainCurrentImageId = 0;
		this->swapChainSyncInterval = 0;

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

		#else

		return ResultError(NotImplemented, std::string("GrafCanvasDX12: failed to initialize, unsupported platform"));

		#endif

		this->swapChainSyncInterval = (GrafPresentMode::Immediate == initParams.PresentMode ? 0 : 1);

		// init swap chain images

		this->swapChainImageCount = initParams.SwapChainImageCount;
		this->swapChainCurrentImageId = ur_uint32(this->swapChainImageCount - 1); // first AcquireNextImage will make it 0
		this->swapChainImages.reserve(this->swapChainImageCount);
		for (ur_uint imageIdx = 0; imageIdx < this->swapChainImageCount; ++imageIdx)
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

		// per frame objects

		this->imageTransitionCmdListBegin.resize(this->swapChainImageCount);
		this->imageTransitionCmdListEnd.resize(this->swapChainImageCount);
		for (ur_uint32 iframe = 0; iframe < this->swapChainImageCount; ++iframe)
		{
			Result urRes = Success;
			urRes &= this->GetGrafSystem().CreateCommandList(this->imageTransitionCmdListBegin[iframe]);
			urRes &= this->GetGrafSystem().CreateCommandList(this->imageTransitionCmdListEnd[iframe]);
			if (Succeeded(urRes))
			{
				urRes &= this->imageTransitionCmdListBegin[iframe]->Initialize(grafDevice);
				urRes &= this->imageTransitionCmdListEnd[iframe]->Initialize(grafDevice);
			}
			if (Failed(urRes))
			{
				this->Deinitialize();
				return ResultError(Failure, "GrafCanvasDX12: failed to create transition command list");
			}
		}

		// acquire an image to use as a current RT

		Result urRes = this->AcquireNextImage();
		if (Failed(urRes))
		{
			this->Deinitialize();
			return urRes;
		}

		return Result(Success);
	}

	Result GrafCanvasDX12::AcquireNextImage()
	{
		this->swapChainCurrentImageId = (this->swapChainCurrentImageId + 1) % this->swapChainImageCount;

		return Result(Success);
	}

	Result GrafCanvasDX12::Present()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// do image layout transition to presentation state

		GrafImage* swapChainCurrentImage = this->swapChainImages[this->swapChainCurrentImageId].get();
		GrafCommandList* imageTransitionCmdListEndCrnt = this->imageTransitionCmdListEnd[this->swapChainCurrentImageId].get();
		imageTransitionCmdListEndCrnt->Begin();
		imageTransitionCmdListEndCrnt->ImageMemoryBarrier(swapChainCurrentImage, GrafImageState::Current, GrafImageState::Present);
		imageTransitionCmdListEndCrnt->End();
		//ID3D12CommandList* d3dCommandListsEnd[] = { static_cast<GrafCommandListDX12*>(imageTransitionCmdListEndCrnt)->GetD3DCommandList() };
		//grafDeviceDX12->GetD3DGraphicsCommandQueue()->ExecuteCommandLists(1, d3dCommandListsEnd);
		grafDeviceDX12->Record(imageTransitionCmdListEndCrnt);
		grafDeviceDX12->Submit();

		// present current image

		this->dxgiSwapChain->Present((UINT)this->swapChainSyncInterval, 0);

		// acquire next available image to use as RT

		Result res = this->AcquireNextImage();
		if (Failed(res))
			return res;

		// do image layout transition to common state

		GrafImage* swapChainNextImage = this->swapChainImages[this->swapChainCurrentImageId].get();
		GrafCommandList* imageTransitionCmdListBeginNext = this->imageTransitionCmdListBegin[this->swapChainCurrentImageId].get();
		imageTransitionCmdListBeginNext->Begin();
		imageTransitionCmdListBeginNext->ImageMemoryBarrier(swapChainNextImage, GrafImageState::Current, GrafImageState::Common);
		imageTransitionCmdListBeginNext->End();
		//ID3D12CommandList* d3dCommandListsBegin[] = { static_cast<GrafCommandListDX12*>(imageTransitionCmdListBeginNext)->GetD3DCommandList() };
		//grafDeviceDX12->GetD3DGraphicsCommandQueue()->ExecuteCommandLists(1, d3dCommandListsBegin);
		grafDeviceDX12->Record(imageTransitionCmdListBeginNext);
		grafDeviceDX12->Submit();

		return Result(Success);
	}

	GrafImage* GrafCanvasDX12::GetCurrentImage()
	{
		return this->GetSwapChainImage((ur_uint)this->swapChainCurrentImageId);
	}

	GrafImage* GrafCanvasDX12::GetSwapChainImage(ur_uint imageId)
	{
		if ((ur_size)imageId > this->swapChainImages.size())
			return ur_null;
		return this->swapChainImages[imageId].get();
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
		this->Deinitialize();

		GrafImage::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (nullptr == grafDeviceDX12 || nullptr == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// create resource

		D3D12_RESOURCE_DESC d3dResDesc = {};
		d3dResDesc.Dimension = GrafUtilsDX12::GrafToD3DResDimension(initParams.ImageDesc.Type);
		d3dResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		d3dResDesc.Width = (UINT64)initParams.ImageDesc.Size.x;
		d3dResDesc.Height = (UINT64)initParams.ImageDesc.Size.y;
		d3dResDesc.DepthOrArraySize = (UINT16)initParams.ImageDesc.Size.z;
		d3dResDesc.MipLevels = (UINT16)initParams.ImageDesc.MipLevels;
		d3dResDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(initParams.ImageDesc.Format);
		d3dResDesc.SampleDesc.Count = 1;
		d3dResDesc.SampleDesc.Quality = 0;
		d3dResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResDesc.Flags = GrafUtilsDX12::GrafToD3DResourceFlags(initParams.ImageDesc.Usage);

		D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
		d3dHeapProperties.Type = GrafUtilsDX12::GrafToD3DImageHeapType(initParams.ImageDesc.Usage, initParams.ImageDesc.MemoryType);
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		D3D12_RESOURCE_STATES d3dResStates = GrafUtilsDX12::GrafToD3DImageInitialState(initParams.ImageDesc.Usage, initParams.ImageDesc.MemoryType);

		HRESULT hres = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dResStates, nullptr,
			__uuidof(ID3D12Resource), d3dResource);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafImageDX12: CreateCommittedResource failed with HRESULT = ") + HResultToString(hres));
		}

		// initial state for image and default subreasource

		GrafImageState initialState = GrafImageState::Common;
		if (d3dResStates & D3D12_RESOURCE_STATE_COPY_SOURCE)
			initialState = GrafImageState::TransferSrc;
		if (d3dResStates & D3D12_RESOURCE_STATE_COPY_DEST)
			initialState = GrafImageState::TransferDst;
		if (d3dResStates & D3D12_RESOURCE_STATE_RENDER_TARGET)
			initialState = GrafImageState::ColorWrite;
		if (d3dResStates & D3D12_RESOURCE_STATE_DEPTH_WRITE)
			initialState = GrafImageState::DepthStencilWrite;
		this->SetState(initialState);

		// create default subresource

		Result res = this->CreateDefaultSubresource();
		if (Failed(res))
		{
			this->Deinitialize();
			return res;
		}

		return Result(Success);
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

		// initial state for image and default subreasource

		GrafImageState initialState = GrafImageState::Common;
		/*if (initParams.ImageDesc.Usage & ur_uint(GrafImageUsageFlag::TransferSrc))
			initialState = GrafImageState::TransferSrc;
		if (initParams.ImageDesc.Usage & ur_uint(GrafImageUsageFlag::TransferDst))
			initialState = GrafImageState::TransferDst;
		if (initParams.ImageDesc.Usage & ur_uint(GrafImageUsageFlag::ColorRenderTarget))
			initialState = GrafImageState::ColorWrite;
		if (initParams.ImageDesc.Usage & ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget))
			initialState = GrafImageState::DepthStencilWrite;*/
		this->SetState(initialState);

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

		if (this->srvDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->srvDescriptorHandle);
		}

		if (this->uavDescriptorHandle.IsValid())
		{
			grafDeviceDX12->ReleaseDescriptorRange(this->uavDescriptorHandle);
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

		// validate image

		const GrafImageDX12* grafImageDX12 = static_cast<const GrafImageDX12*>(this->GetImage());
		if (ur_null == grafImageDX12 || nullptr == grafImageDX12->GetD3DResource())
		{
			return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to initialize, invalid image"));
		}

		// create view

		Result res = this->CreateD3DImageView();
		if (Failed(res))
			return res;

		// set initial state from parent resource

		this->SetState(grafImageDX12->GetState());

		return Result(Success);
	}

	Result GrafImageSubresourceDX12::CreateD3DImageView()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		const GrafImageDX12* grafImageDX12 = static_cast<const GrafImageDX12*>(this->GetImage());
		const GrafImageDesc& grafImageDesc = grafImageDX12->GetDesc();
		const GrafImageSubresourceDesc& grafSubresDesc = this->GetDesc();

		// create corresponding view(s)

		if (grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::ColorRenderTarget))
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
			rtvDesc.ViewDimension = GrafUtilsDX12::GrafToD3DRTVDimension(grafImageDesc.Type);
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

			this->rtvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
			if (!this->rtvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to allocate RTV descriptor"));
			}
			grafDeviceDX12->GetD3DDevice()->CreateRenderTargetView(grafImageDX12->GetD3DResource(), &rtvDesc, this->rtvDescriptorHandle.GetD3DHandleCPU());
		}

		if (grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget))
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
			dsvDesc.ViewDimension = GrafUtilsDX12::GrafToD3DDSVDimension(grafImageDesc.Type);
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			switch (grafImageDesc.Type)
			{
			case GrafImageType::Tex1D:
				dsvDesc.Texture1D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				break;
			case GrafImageType::Tex2D:
				dsvDesc.Texture2D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				break;
			};

			this->dsvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
			if (!this->dsvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to allocate DSV descriptor"));
			}
			grafDeviceDX12->GetD3DDevice()->CreateDepthStencilView(grafImageDX12->GetD3DResource(), &dsvDesc, this->dsvDescriptorHandle.GetD3DHandleCPU());
		}

		if (grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::ShaderRead))
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
			srvDesc.ViewDimension = GrafUtilsDX12::GrafToD3DSRVDimension(grafImageDesc.Type);
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			switch (grafImageDesc.Type)
			{
			case GrafImageType::Tex1D:
				srvDesc.Texture1D.MostDetailedMip = UINT(grafSubresDesc.BaseMipLevel);
				srvDesc.Texture1D.MipLevels = UINT(std::max(grafSubresDesc.LevelCount, 1u));
				srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
				break;
			case GrafImageType::Tex2D:
				srvDesc.Texture2D.MostDetailedMip = UINT(grafSubresDesc.BaseMipLevel);
				srvDesc.Texture2D.MipLevels = UINT(std::max(grafSubresDesc.LevelCount, 1u));
				srvDesc.Texture2D.PlaneSlice = UINT(grafSubresDesc.BaseArrayLayer);
				srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
				break;
			case GrafImageType::Tex3D:
				srvDesc.Texture3D.MostDetailedMip = UINT(grafSubresDesc.BaseMipLevel);
				srvDesc.Texture3D.MipLevels = UINT(std::max(grafSubresDesc.LevelCount, 1u));
				srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
				break;
			};

			if (grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget))
			{
				// depth stencil SRV format override
				switch (grafImageDesc.Format)
				{
				case GrafFormat::D24_UNORM_S8_UINT:
					srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;
				};
			}

			this->srvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->srvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to allocate SRV descriptor"));
			}
			grafDeviceDX12->GetD3DDevice()->CreateShaderResourceView(grafImageDX12->GetD3DResource(), &srvDesc, this->srvDescriptorHandle.GetD3DHandleCPU());
		}

		if (grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::ShaderReadWrite))
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
			uavDesc.ViewDimension = GrafUtilsDX12::GrafToD3DUAVDimension(grafImageDesc.Type);
			switch (grafImageDesc.Type)
			{
			case GrafImageType::Tex1D:
				uavDesc.Texture1D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				break;
			case GrafImageType::Tex2D:
				uavDesc.Texture2D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				uavDesc.Texture2D.PlaneSlice = UINT(grafSubresDesc.BaseArrayLayer);
				break;
			case GrafImageType::Tex3D:
				uavDesc.Texture3D.MipSlice = UINT(grafSubresDesc.BaseMipLevel);
				uavDesc.Texture3D.FirstWSlice = UINT(grafSubresDesc.BaseArrayLayer);
				uavDesc.Texture3D.WSize = UINT(grafSubresDesc.LayerCount);
				break;
			};

			this->uavDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->srvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafImageSubresourceDX12: failed to allocate UAV descriptor"));
			}
			grafDeviceDX12->GetD3DDevice()->CreateUnorderedAccessView(grafImageDX12->GetD3DResource(), nullptr, &uavDesc, this->uavDescriptorHandle.GetD3DHandleCPU());
		}

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

		ur_size sizeInBytesAligned = initParams.BufferDesc.SizeInBytes;
		if (ur_uint(GrafBufferUsageFlag::ConstantBuffer) & initParams.BufferDesc.Usage)
		{
			sizeInBytesAligned = ur_align(initParams.BufferDesc.SizeInBytes, grafDeviceDX12->GetPhysicalDeviceDesc()->ConstantBufferOffsetAlignment);
		}
		else if (ur_uint(GrafBufferUsageFlag::AccelerationStructure) & initParams.BufferDesc.Usage)
		{
			sizeInBytesAligned = ur_align(initParams.BufferDesc.SizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		}
		this->bufferDesc.SizeInBytes = sizeInBytesAligned;

		D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
		d3dHeapProperties.Type = GrafUtilsDX12::GrafToD3DBufferHeapType(initParams.BufferDesc.Usage, initParams.BufferDesc.MemoryType);
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC d3dResDesc = {};
		d3dResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		d3dResDesc.Width = (UINT64)sizeInBytesAligned;
		d3dResDesc.Height = 1;
		d3dResDesc.DepthOrArraySize = 1;
		d3dResDesc.MipLevels = 1;
		d3dResDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dResDesc.SampleDesc.Count = 1;
		d3dResDesc.SampleDesc.Quality = 0;
		d3dResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		d3dResDesc.Flags = (initParams.BufferDesc.Usage & ur_uint(GrafBufferUsageFlag::StorageBuffer) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);

		D3D12_RESOURCE_STATES d3dResStates = GrafUtilsDX12::GrafToD3DBufferInitialState(initParams.BufferDesc.Usage, initParams.BufferDesc.MemoryType);

		HRESULT hres = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dResStates, ur_null,
			__uuidof(ID3D12Resource), this->d3dResource);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafBufferDX12: CreateCommittedResource failed with HRESULT = ") + HResultToString(hres));
		}

		if (ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress) & initParams.BufferDesc.Usage)
		{
			this->bufferDeviceAddress = (ur_uint64)this->d3dResource->GetGPUVirtualAddress();
		}

		// create CBV

		/*if (ur_uint(GrafBufferUsageFlag::ConstantBuffer) & initParams.BufferDesc.Usage)
		{
			this->cbvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->cbvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to allocate CBV descriptor"));
			}

			D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCbvDesc = {};
			d3dCbvDesc.BufferLocation = this->d3dResource->GetGPUVirtualAddress();
			d3dCbvDesc.SizeInBytes = (UINT)sizeInBytesAligned;

			grafDeviceDX12->GetD3DDevice()->CreateConstantBufferView(&d3dCbvDesc, this->cbvDescriptorHandle.GetD3DHandleCPU());
		}*/

		// create SRV

		if (ur_uint(GrafBufferUsageFlag::StorageBuffer) & initParams.BufferDesc.Usage)
		{
			this->srvDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->srvDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to allocate SRV descriptor"));
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {};
			d3dSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
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
			this->uavDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			if (!this->uavDescriptorHandle.IsValid())
			{
				this->Deinitialize();
				return ResultError(InvalidArgs, std::string("GrafBufferDX12: failed to allocate UAV descriptor"));
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUavDesc = {};
			d3dUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
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
				initParams.BufferDesc.ElementSize == 4 ? DXGI_FORMAT_R32_UINT :
				initParams.BufferDesc.ElementSize == 2 ? DXGI_FORMAT_R16_UINT :
				DXGI_FORMAT_UNKNOWN);
		}

		return Result(Success);
	}

	Result GrafBufferDX12::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->GetDesc().SizeInBytes; // entire allocation range

		GrafWriteCallback copyWriteCallback = [&dataPtr, &dataSize, &srcOffset](ur_byte* mappedDataPtr) -> Result
		{
			memcpy(mappedDataPtr, dataPtr + srcOffset, dataSize);
			return Result(Success);
		};

		return this->Write(copyWriteCallback, dataSize, srcOffset, dstOffset);
	}

	Result GrafBufferDX12::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->GetDesc().SizeInBytes; // entire allocation range

		if (ur_null == writeCallback || dstOffset + dataSize > this->GetDesc().SizeInBytes)
			return Result(InvalidArgs);

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		D3D12_RANGE d3dRange;
		d3dRange.Begin = dstOffset;
		d3dRange.End = dstOffset + dataSize;

		void* mappedMemoryPtr = nullptr;
		HRESULT hres = this->d3dResource->Map(0, &d3dRange, &mappedMemoryPtr);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafBufferDX12: Map failed with HRESULT = ") + HResultToString(hres));
		}

		mappedMemoryPtr = (void*)((ur_byte*)mappedMemoryPtr + dstOffset);

		writeCallback((ur_byte*)mappedMemoryPtr);

		this->d3dResource->Unmap(0, &d3dRange);

		return Result(Success);
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

		this->samplerDescriptorHandle = grafDeviceDX12->AllocateDescriptorRangeCPU(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1);
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
		this->byteCodeSize = 0;
		this->byteCodeBuffer.reset(ur_null);

		return Result(Success);
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

	GrafShaderLibDX12::GrafShaderLibDX12(GrafSystem& grafSystem) :
		GrafShaderLib(grafSystem)
	{
	}

	GrafShaderLibDX12::~GrafShaderLibDX12()
	{
		this->Deinitialize();
	}

	Result GrafShaderLibDX12::Deinitialize()
	{
		this->shaders.clear();
		this->byteCodeSize = 0;
		this->byteCodeBuffer.reset(ur_null);

		return Result(Success);
	}

	Result GrafShaderLibDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		if (nullptr == initParams.ByteCode || 0 == initParams.ByteCodeSize)
		{
			return ResultError(InvalidArgs, std::string("GrafShaderLibDX12: failed to initialize, invalid byte code"));
		}

		GrafShaderLib::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafShaderLibDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// initialize DXC linker

		shared_ref<IDxcUtils> dxcUtils;
		HRESULT hres = DxcCreateInstance(CLSID_DxcUtils, __uuidof(IDxcUtils), dxcUtils);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafShaderLibDX12: DxcCreateInstance(CLSID_DxcUtils) failed with HRESULT = ") + HResultToString(hres));
		}

		shared_ref<IDxcBlob> dxcLibrary;
		dxcUtils->CreateBlobFromPinned(initParams.ByteCode, (UINT32)initParams.ByteCodeSize, 0, dxcLibrary);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafShaderLibDX12: CreateBlobFromPinned failed with HRESULT = ") + HResultToString(hres));
		}

		shared_ref<IDxcLinker> dxcLinker;
		hres = DxcCreateInstance(CLSID_DxcLinker, __uuidof(IDxcLinker), dxcLinker);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafShaderLibDX12: DxcCreateInstance(CLSID_DxcLinker) failed with HRESULT = ") + HResultToString(hres));
		}

		LPCWSTR linkLibNames[] = { L"Library" };
		hres = dxcLinker->RegisterLibrary(linkLibNames[0], dxcLibrary);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafShaderLibDX12: RegisterLibrary failed with HRESULT = ") + HResultToString(hres));
		}

		LPCWSTR linkArguments[] = { L"" };

		// initialize shaders

		Result res(Success);
		this->shaders.reserve(initParams.EntryPointCount);
		for (ur_uint ientry = 0; ientry < initParams.EntryPointCount; ++ientry)
		{
			const GrafShaderLib::EntryPoint& libEntryPoint = initParams.EntryPoints[ientry];

			// link shader byte code

			std::wstring linkEntryPoint;
			StringToWstring(libEntryPoint.Name, linkEntryPoint);

			std::wstring linkTargetProfile;
			switch (libEntryPoint.Type)
			{
			case GrafShaderType::Vertex: linkTargetProfile = L"vs_"; break;
			case GrafShaderType::Pixel: linkTargetProfile = L"ps_"; break;
			case GrafShaderType::Compute: linkTargetProfile = L"cs_"; break;
			default: linkTargetProfile = L"lib_";
			};
			linkTargetProfile += DXCLinkerShaderModel;

			shared_ref<IDxcOperationResult> dxcLinkResult;
			hres = dxcLinker->Link(linkEntryPoint.c_str(), linkTargetProfile.c_str(),
				linkLibNames, ur_array_size(linkLibNames),
				linkArguments, ur_array_size(linkArguments),
				dxcLinkResult);
			if (FAILED(hres))
			{
				LogError(std::string("GrafShaderLibDX12: Link failed with HRESULT = ") + HResultToString(hres));
				continue;
			}

			shared_ref<IDxcBlob> dxcErrorBuffer;
			dxcLinkResult->GetErrorBuffer(dxcErrorBuffer);
			if (dxcErrorBuffer.get() && dxcErrorBuffer->GetBufferSize() > 0)
			{
				char* errorMsg = (char*)dxcErrorBuffer->GetBufferPointer();
				LogError(std::string("GrafShaderLibDX12: error message while linking ") + libEntryPoint.Name + ": " + errorMsg);
			}

			shared_ref<IDxcBlob> dxcShader;
			hres = dxcLinkResult->GetResult(dxcShader);
			if (FAILED(hres) || nullptr == dxcShader.get())
			{
				LogError(std::string("GrafShaderLibDX12: failed to link shader ") + libEntryPoint.Name);
				continue;
			}

			// create shader

			GrafShader::InitParams shaderParams = {};
			shaderParams.ByteCode = (ur_byte*)dxcShader->GetBufferPointer();
			shaderParams.ByteCodeSize = (ur_size)dxcShader->GetBufferSize();
			shaderParams.ShaderType = libEntryPoint.Type;
			shaderParams.EntryPoint = libEntryPoint.Name;

			std::unique_ptr<GrafShader> grafShader;
			this->GetGrafSystem().CreateShader(grafShader);
			Result shaderRes = grafShader.get()->Initialize(grafDeviceDX12, shaderParams);
			if (Failed(shaderRes))
			{
				LogError(std::string("GrafShaderLibDX12: failed to initialize shader for entry point = ") + libEntryPoint.Name);
				continue;
			}
			res &= shaderRes;

			this->shaders.push_back(std::move(grafShader));
		}

		// store byte code locally

		this->byteCodeSize = initParams.ByteCodeSize;
		this->byteCodeBuffer.reset(new ur_byte[this->byteCodeSize]);
		memcpy(this->byteCodeBuffer.get(), initParams.ByteCode, this->byteCodeSize);

		return res;
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
		for (auto& tableDesc : this->descriptorTableDesc)
		{
			tableDesc = {};
		}
	}

	GrafDescriptorTableLayoutDX12::~GrafDescriptorTableLayoutDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableLayoutDX12::Deinitialize()
	{
		for (auto& tableDesc : this->descriptorTableDesc)
		{
			tableDesc = {};
		}

		return Result(Success);
	}

	Result GrafDescriptorTableLayoutDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafDescriptorTableLayout::Initialize(grafDevice, initParams);

		// init D3D ranges & count descriptors per heap type

		const GrafDescriptorTableLayoutDesc& grafTableLayoutDesc = this->GetLayoutDesc();
		for (ur_uint layoutRangeIdx = 0; layoutRangeIdx < grafTableLayoutDesc.DescriptorRangeCount; ++layoutRangeIdx)
		{
			const GrafDescriptorRangeDesc& grafDescriptorRange = grafTableLayoutDesc.DescriptorRanges[layoutRangeIdx];
			DescriptorTableDesc* descriptorTableDesc = nullptr;
			if (GrafDescriptorType::ConstantBuffer == grafDescriptorRange.Type ||
				GrafDescriptorType::Texture == grafDescriptorRange.Type || GrafDescriptorType::Buffer == grafDescriptorRange.Type ||
				GrafDescriptorType::RWTexture == grafDescriptorRange.Type || GrafDescriptorType::RWBuffer == grafDescriptorRange.Type ||
				GrafDescriptorType::AccelerationStructure == grafDescriptorRange.Type ||
				GrafDescriptorType::TextureDynamicArray == grafDescriptorRange.Type ||
				GrafDescriptorType::BufferDynamicArray == grafDescriptorRange.Type)
			{
				descriptorTableDesc = &this->descriptorTableDesc[GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv];
			}
			if (GrafDescriptorType::Sampler == grafDescriptorRange.Type)
			{
				descriptorTableDesc = &this->descriptorTableDesc[GrafShaderVisibleDescriptorHeapTypeDX12_Sampler];
			}
			if (nullptr == descriptorTableDesc)
				continue;

			D3D12_DESCRIPTOR_RANGE& d3dDescriptorRange = descriptorTableDesc->d3dDescriptorRanges.emplace_back();
			d3dDescriptorRange.RangeType = GrafUtilsDX12::GrafToD3DDescriptorType(grafDescriptorRange.Type);
			d3dDescriptorRange.BaseShaderRegister = (UINT)grafDescriptorRange.BindingOffset;
			d3dDescriptorRange.NumDescriptors = (UINT)grafDescriptorRange.BindingCount;
			d3dDescriptorRange.RegisterSpace = 0;
			d3dDescriptorRange.OffsetInDescriptorsFromTableStart = descriptorTableDesc->descriptorCount;
			descriptorTableDesc->descriptorCount += grafDescriptorRange.BindingCount;
		}

		// init D3D root parameter(s)

		for (auto& descriptorTableDesc : this->descriptorTableDesc)
		{
			D3D12_ROOT_PARAMETER& d3dRootParameter = descriptorTableDesc.d3dRootParameter;
			d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			d3dRootParameter.ShaderVisibility = GrafUtilsDX12::GrafToD3DShaderVisibility(grafTableLayoutDesc.ShaderStageVisibility);
			d3dRootParameter.DescriptorTable.NumDescriptorRanges = (UINT)descriptorTableDesc.d3dDescriptorRanges.size();
			d3dRootParameter.DescriptorTable.pDescriptorRanges = (descriptorTableDesc.d3dDescriptorRanges.size() > 0 ? &descriptorTableDesc.d3dDescriptorRanges.front() : NULL);
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableDX12::GrafDescriptorTableDX12(GrafSystem &grafSystem) :
		GrafDescriptorTable(grafSystem)
	{
		for (auto& tableData : this->descriptorTableData)
		{
			tableData = {};
		}
	}

	GrafDescriptorTableDX12::~GrafDescriptorTableDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableDX12::Deinitialize()
	{
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(this->GetGrafDevice());

		for (auto& tableData : this->descriptorTableData)
		{
			if (tableData.descriptorHeapHandle.IsValid())
			{
				grafDeviceDX12->ReleaseDescriptorRange(tableData.descriptorHeapHandle);
			}
			tableData = {};
		}

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
		GrafDescriptorTableLayoutDX12* descriptorTableLayoutDX12 = static_cast<GrafDescriptorTableLayoutDX12*>(this->GetLayout());

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to initialize, invalid GrafDevice"));
		}

		// allocate ranges in descriptor heap(s)

		for (ur_uint heapIdx = 0; heapIdx < GrafShaderVisibleDescriptorHeapTypeDX12_Count; ++heapIdx)
		{
			const auto& tableDesc = descriptorTableLayoutDX12->GetTableDescForHeapType(GrafShaderVisibleDescriptorHeapTypeDX12(heapIdx));
			auto& tableData = this->descriptorTableData[heapIdx];
			if (tableDesc.descriptorCount > 0)
			{
				tableData.descriptorHeapHandle = grafDeviceDX12->AllocateDescriptorRangeGPU(D3D12_DESCRIPTOR_HEAP_TYPE(heapIdx), tableDesc.descriptorCount);
				if (!tableData.descriptorHeapHandle.IsValid())
				{
					this->Deinitialize();
					return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to allocate descriptor range"));
				}
			}
		}

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::CopyResourceDescriptorToTable(ur_uint bindingIdx, const D3D12_CPU_DESCRIPTOR_HANDLE& d3dResourceHandle, D3D12_DESCRIPTOR_RANGE_TYPE d3dRangeType)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dBindingHandle;
		Result res = GetD3DDescriptorHandle(d3dBindingHandle, bindingIdx, d3dRangeType);
		if (Failed(res))
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to get descriptor for binding"));

		D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType = (D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER == d3dRangeType ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ID3D12Device5* d3dDevice = static_cast<GrafDeviceDX12*>(this->GetGrafDevice())->GetD3DDevice();

		d3dDevice->CopyDescriptorsSimple(1, d3dBindingHandle, d3dResourceHandle, d3dHeapType);

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs, ur_size bufferRange)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dBindingHandle;
		GrafDescriptorHandleDX12& tableHandle = this->descriptorTableData[GrafShaderVisibleDescriptorHeapTypeDX12_SrvUavCbv].descriptorHeapHandle;
		Result res = GetD3DDescriptorHandle(d3dBindingHandle, bindingIdx, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
		if (Failed(res))
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to get descriptor for binding"));

		ID3D12Device5* d3dDevice = static_cast<GrafDeviceDX12*>(this->GetGrafDevice())->GetD3DDevice();
		GrafBufferDX12* grafBufferDX12 = static_cast<GrafBufferDX12*>(buffer);

		D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCbvDesc = {};
		d3dCbvDesc.BufferLocation = grafBufferDX12->GetD3DResource()->GetGPUVirtualAddress() + (UINT64)bufferOfs;
		d3dCbvDesc.SizeInBytes = (UINT)(bufferRange == 0 ? buffer->GetDesc().SizeInBytes : bufferRange);

		d3dDevice->CreateConstantBufferView(&d3dCbvDesc, d3dBindingHandle);

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler)
	{
		GrafImageDX12* imageDX12 = static_cast<GrafImageDX12*>(image);
		return SetSampledImage(bindingIdx, imageDX12->GetDefaultSubresource(), sampler);
	}

	Result GrafDescriptorTableDX12::SetSampledImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource, GrafSampler* sampler)
	{
		Result res = this->SetImage(bindingIdx, imageSubresource);
		res &= this->SetSampler(bindingIdx, sampler);
		return res;
	}

	Result GrafDescriptorTableDX12::SetSampler(ur_uint bindingIdx, GrafSampler* sampler)
	{
		return CopyResourceDescriptorToTable(bindingIdx,
			static_cast<GrafSamplerDX12*>(sampler)->GetDescriptorHandle().GetD3DHandleCPU(),
			D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
	}

	Result GrafDescriptorTableDX12::SetImage(ur_uint bindingIdx, GrafImage* image)
	{
		GrafImageDX12* imageDX12 = static_cast<GrafImageDX12*>(image);
		return this->SetImage(bindingIdx, imageDX12->GetDefaultSubresource());
	}

	Result GrafDescriptorTableDX12::SetImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource)
	{
		GrafImageSubresourceDX12* imageSubresourceDX12 = static_cast<GrafImageSubresourceDX12*>(imageSubresource);
		return CopyResourceDescriptorToTable(bindingIdx,
			imageSubresourceDX12->GetSRVDescriptorHandle().GetD3DHandleCPU(),
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	}

	Result GrafDescriptorTableDX12::SetImageArray(ur_uint bindingIdx, GrafImage** images, ur_uint imageCount)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dBindingHandle;
		Result res = GetD3DDescriptorHandle(d3dBindingHandle, bindingIdx, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		if (Failed(res))
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to get descriptor for binding"));

		GrafImageDX12* imageDX12 = static_cast<GrafImageDX12*>(images[0]);
		GrafImageSubresourceDX12* imageSubresourceDX12 = static_cast<GrafImageSubresourceDX12*>(imageDX12->GetDefaultSubresource());
		D3D12_CPU_DESCRIPTOR_HANDLE d3dResourceHandle = imageSubresourceDX12->GetSRVDescriptorHandle().GetD3DHandleCPU();

		ID3D12Device5* d3dDevice = static_cast<GrafDeviceDX12*>(this->GetGrafDevice())->GetD3DDevice();
		d3dDevice->CopyDescriptorsSimple(imageCount, d3dBindingHandle, d3dResourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return CopyResourceDescriptorToTable(bindingIdx,
			static_cast<GrafBufferDX12*>(buffer)->GetSRVDescriptorHandle().GetD3DHandleCPU(),
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	}

	Result GrafDescriptorTableDX12::SetBufferArray(ur_uint bindingIdx, GrafBuffer** buffers, ur_uint bufferCount)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dBindingHandle;
		Result res = GetD3DDescriptorHandle(d3dBindingHandle, bindingIdx, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		if (Failed(res))
			return ResultError(InvalidArgs, std::string("GrafDescriptorTableDX12: failed to get descriptor for binding"));

		GrafBufferDX12* bufferDX12 = static_cast<GrafBufferDX12*>(buffers[0]);
		D3D12_CPU_DESCRIPTOR_HANDLE d3dResourceHandle = bufferDX12->GetSRVDescriptorHandle().GetD3DHandleCPU();

		ID3D12Device5* d3dDevice = static_cast<GrafDeviceDX12*>(this->GetGrafDevice())->GetD3DDevice();
		d3dDevice->CopyDescriptorsSimple(bufferCount, d3dBindingHandle, d3dResourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		return Result(Success);
	}

	Result GrafDescriptorTableDX12::SetRWImage(ur_uint bindingIdx, GrafImage* image)
	{
		GrafImageDX12* imageDX12 = static_cast<GrafImageDX12*>(image);
		return this->SetRWImage(bindingIdx, imageDX12->GetDefaultSubresource());
	}

	Result GrafDescriptorTableDX12::SetRWImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource)
	{
		GrafImageSubresourceDX12* imageSubresourceDX12 = static_cast<GrafImageSubresourceDX12*>(imageSubresource);
		return CopyResourceDescriptorToTable(bindingIdx,
			imageSubresourceDX12->GetUAVDescriptorHandle().GetD3DHandleCPU(),
			D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	}

	Result GrafDescriptorTableDX12::SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return CopyResourceDescriptorToTable(bindingIdx,
			static_cast<GrafBufferDX12*>(buffer)->GetUAVDescriptorHandle().GetD3DHandleCPU(),
			D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	}

	Result GrafDescriptorTableDX12::SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure)
	{
		GrafAccelerationStructureDX12* accelerationStructureDX12 = static_cast<GrafAccelerationStructureDX12*>(accelerationStructure);
		GrafBufferDX12* accelerationStructureBufferDX12 = static_cast<GrafBufferDX12*>(accelerationStructureDX12->GetStorageBuffer());
		return CopyResourceDescriptorToTable(bindingIdx,
			accelerationStructureBufferDX12->GetSRVDescriptorHandle().GetD3DHandleCPU(),
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafPipelineDX12::GrafPipelineDX12(GrafSystem &grafSystem) :
		GrafPipeline(grafSystem)
	{
		this->d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	GrafPipelineDX12::~GrafPipelineDX12()
	{
		this->Deinitialize();
	}

	Result GrafPipelineDX12::Deinitialize()
	{
		this->d3dPipelineState.reset(ur_null);
		this->d3dRootSignature.reset(ur_null);
		this->d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

		return Result(Success);
	}

	Result GrafPipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		if (ur_null == initParams.RenderPass)
			return Result(InvalidArgs);

		GrafPipeline::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafPipelineDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc = {};

		d3dPipelineDesc.NodeMask = 0;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		// initialize root signature

		D3D12_ROOT_SIGNATURE_DESC d3dRootSigDesc = {};
		d3dRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		std::vector<D3D12_ROOT_PARAMETER> d3dRootParameters;
		for (ur_uint descriptorLayoutIdx = 0; descriptorLayoutIdx < initParams.DescriptorTableLayoutCount; ++descriptorLayoutIdx)
		{
			GrafDescriptorTableLayoutDX12* tableLayoutDX12 = static_cast<GrafDescriptorTableLayoutDX12*>(initParams.DescriptorTableLayouts[descriptorLayoutIdx]);
			const D3D12_ROOT_PARAMETER& tableSrvUavCbvD3DRootPatameter = tableLayoutDX12->GetSrvUavCbvTableD3DRootParameter();
			if (tableSrvUavCbvD3DRootPatameter.DescriptorTable.NumDescriptorRanges > 0)
			{
				d3dRootParameters.emplace_back(tableSrvUavCbvD3DRootPatameter);
				d3dRootSigDesc.NumParameters += 1;
			}
			const D3D12_ROOT_PARAMETER& tableSamplerD3DRootPatameter = tableLayoutDX12->GetSamplerTableD3DRootParameter();
			if (tableSamplerD3DRootPatameter.DescriptorTable.NumDescriptorRanges > 0)
			{
				d3dRootParameters.emplace_back(tableSamplerD3DRootPatameter);
				d3dRootSigDesc.NumParameters += 1;
			}
		}
		d3dRootSigDesc.pParameters = &d3dRootParameters.front();

		shared_ref<ID3DBlob> d3dSignatureBlob;
		shared_ref<ID3DBlob> d3dErrorBlob;
		HRESULT hres = D3D12SerializeRootSignature(&d3dRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, d3dSignatureBlob, d3dErrorBlob);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafPipelineDX12: D3D12SerializeRootSignature failed with HRESULT = ") + HResultToString(hres));
		}

		hres = d3dDevice->CreateRootSignature(d3dPipelineDesc.NodeMask, d3dSignatureBlob->GetBufferPointer(), d3dSignatureBlob->GetBufferSize(),
			__uuidof(this->d3dRootSignature), this->d3dRootSignature);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafPipelineDX12: CreateRootSignature failed with HRESULT = ") + HResultToString(hres));
		}

		d3dPipelineDesc.pRootSignature = this->d3dRootSignature.get();

		// initialize shader stages

		for (ur_uint shaderIdx = 0; shaderIdx < initParams.ShaderStageCount; ++shaderIdx)
		{
			GrafShaderDX12* grafShaderDX12 = static_cast<GrafShaderDX12*>(initParams.ShaderStages[shaderIdx]);
			if (nullptr == grafShaderDX12)
				continue;

			switch (grafShaderDX12->GetShaderType())
			{
			case GrafShaderType::Vertex:
				d3dPipelineDesc.VS.pShaderBytecode = grafShaderDX12->GetByteCodePtr();
				d3dPipelineDesc.VS.BytecodeLength = (SIZE_T)grafShaderDX12->GetByteCodeSize();
				break;
			case GrafShaderType::Pixel:
				d3dPipelineDesc.PS.pShaderBytecode = grafShaderDX12->GetByteCodePtr();
				d3dPipelineDesc.PS.BytecodeLength = (SIZE_T)grafShaderDX12->GetByteCodeSize();
				break;
			}
		}

		// render targets

		d3dPipelineDesc.NumRenderTargets = 0;
		for (ur_size imageIdx = 0; imageIdx < initParams.RenderPass->GetImageCount(); ++imageIdx)
		{
			const GrafRenderPassImageDesc& grafImageDesc = initParams.RenderPass->GetImageDesc(imageIdx);
			if (GrafUtils::IsDepthStencilFormat(grafImageDesc.Format))
				d3dPipelineDesc.DSVFormat = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
			else
				d3dPipelineDesc.RTVFormats[d3dPipelineDesc.NumRenderTargets++] = GrafUtilsDX12::GrafToDXGIFormat(grafImageDesc.Format);
		}

		// multisampling

		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.SampleDesc.Quality = 0;

		// blend state

		d3dPipelineDesc.BlendState.AlphaToCoverageEnable = FALSE;
		d3dPipelineDesc.BlendState.IndependentBlendEnable = TRUE;
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			const GrafColorBlendOpDesc& grafBlendDesc = (i < initParams.ColorBlendOpDescCount ? initParams.ColorBlendOpDesc[i] : GrafColorBlendOpDesc::Default);
			D3D12_RENDER_TARGET_BLEND_DESC& d3dBlendDesc = d3dPipelineDesc.BlendState.RenderTarget[i];
			d3dBlendDesc.BlendEnable = grafBlendDesc.BlendEnable;
			d3dBlendDesc.SrcBlend = GrafUtilsDX12::GrafToD3DBlendFactor(grafBlendDesc.SrcColorFactor);
			d3dBlendDesc.DestBlend = GrafUtilsDX12::GrafToD3DBlendFactor(grafBlendDesc.DstColorFactor);
			d3dBlendDesc.BlendOp = GrafUtilsDX12::GrafToD3DBlendOp(grafBlendDesc.ColorOp);
			d3dBlendDesc.SrcBlendAlpha = GrafUtilsDX12::GrafToD3DBlendFactor(grafBlendDesc.SrcAlphaFactor);
			d3dBlendDesc.DestBlendAlpha = GrafUtilsDX12::GrafToD3DBlendFactor(grafBlendDesc.DstAlphaFactor);
			d3dBlendDesc.BlendOpAlpha = GrafUtilsDX12::GrafToD3DBlendOp(grafBlendDesc.AlphaOp);
			d3dBlendDesc.LogicOpEnable = FALSE;
			d3dBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
			d3dBlendDesc.RenderTargetWriteMask = UINT8(grafBlendDesc.WriteMask);
		}

		d3dPipelineDesc.SampleMask = UINT_MAX;

		// rasterizer state

		d3dPipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		d3dPipelineDesc.RasterizerState.CullMode = GrafUtilsDX12::GrafToD3DCullMode(initParams.CullMode);
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = BOOL(GrafFrontFaceOrder::CounterClockwise == initParams.FrontFaceOrder);
		d3dPipelineDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		d3dPipelineDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		d3dPipelineDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		d3dPipelineDesc.RasterizerState.DepthClipEnable = TRUE;
		d3dPipelineDesc.RasterizerState.MultisampleEnable = FALSE;
		d3dPipelineDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		d3dPipelineDesc.RasterizerState.ForcedSampleCount = 0;
		d3dPipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// depth stencil state

		d3dPipelineDesc.DepthStencilState.DepthEnable = initParams.DepthTestEnable;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dPipelineDesc.DepthStencilState.DepthFunc = GrafUtilsDX12::GrafToD3DCompareOp(initParams.DepthCompareOp);
		d3dPipelineDesc.DepthStencilState.StencilEnable = initParams.StencilTestEnable;
		d3dPipelineDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		d3dPipelineDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFailOp = GrafUtilsDX12::GrafToD3DStencilOp(initParams.StencilFront.FailOp);
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = GrafUtilsDX12::GrafToD3DStencilOp(initParams.StencilFront.DepthFailOp);
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilPassOp = GrafUtilsDX12::GrafToD3DStencilOp(initParams.StencilFront.PassOp);
		d3dPipelineDesc.DepthStencilState.FrontFace.StencilFunc = GrafUtilsDX12::GrafToD3DCompareOp(initParams.StencilFront.CompareOp);
		d3dPipelineDesc.DepthStencilState.BackFace.StencilFailOp = GrafUtilsDX12::GrafToD3DStencilOp(initParams.StencilBack.FailOp);
		d3dPipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = GrafUtilsDX12::GrafToD3DStencilOp(initParams.StencilBack.DepthFailOp);
		d3dPipelineDesc.DepthStencilState.BackFace.StencilPassOp = GrafUtilsDX12::GrafToD3DStencilOp(initParams.StencilBack.PassOp);
		d3dPipelineDesc.DepthStencilState.BackFace.StencilFunc = GrafUtilsDX12::GrafToD3DCompareOp(initParams.StencilBack.CompareOp);

		// input layout

		ur_uint semanticsCount = 0;
		for (ur_uint vertexInputIdx = 0; vertexInputIdx < initParams.VertexInputCount; ++vertexInputIdx)
		{
			semanticsCount += initParams.VertexInputDesc[vertexInputIdx].ElementCount;
		}
		std::vector<std::string> semanticNames;
		semanticNames.reserve(semanticsCount);
		ur_uint semanticIdx = 0;

		std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputElementDescs;
		for (ur_uint vertexInputIdx = 0; vertexInputIdx < initParams.VertexInputCount; ++vertexInputIdx)
		{
			const GrafVertexInputDesc& grafVertexInputDesc = initParams.VertexInputDesc[vertexInputIdx];
			for (ur_uint elementIdx = 0; elementIdx < grafVertexInputDesc.ElementCount; ++elementIdx)
			{
				const GrafVertexElementDesc& grafVertexElementDesc = grafVertexInputDesc.Elements[elementIdx];
				D3D12_INPUT_ELEMENT_DESC& d3dInputElementDesc = d3dInputElementDescs.emplace_back();
				d3dInputElementDesc.SemanticName = GrafUtilsDX12::ParseVertexElementSemantic(grafVertexElementDesc.Semantic, semanticNames.emplace_back(), d3dInputElementDesc.SemanticIndex);
				d3dInputElementDesc.Format = GrafUtilsDX12::GrafToDXGIFormat(grafVertexElementDesc.Format);
				d3dInputElementDesc.InputSlot = (UINT)grafVertexInputDesc.BindingIdx;
				d3dInputElementDesc.AlignedByteOffset = (UINT)grafVertexElementDesc.Offset;
				if (GrafVertexInputType::PerInstance == grafVertexInputDesc.InputType)
				{
					d3dInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
					d3dInputElementDesc.InstanceDataStepRate = 1;
				}
				else
				{
					d3dInputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					d3dInputElementDesc.InstanceDataStepRate = 0;
				}
			}
		}
		d3dPipelineDesc.InputLayout.pInputElementDescs = (d3dInputElementDescs.size() > 0 ? &d3dInputElementDescs.front() : NULL);
		d3dPipelineDesc.InputLayout.NumElements = (UINT)d3dInputElementDescs.size();

		// primitive topology

		d3dPipelineDesc.PrimitiveTopologyType = GrafUtilsDX12::GrafToD3DPrimitiveTopologyType(initParams.PrimitiveTopology);
		this->d3dPrimitiveTopology = GrafUtilsDX12::GrafToD3DPrimitiveTopology(initParams.PrimitiveTopology);

		// create pipeline

		hres = d3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, __uuidof(ID3D12PipelineState), this->d3dPipelineState);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafPipelineDX12: CreateGraphicsPipelineState failed with HRESULT = ") + HResultToString(hres));
		}

		return Result(Success);
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
		this->d3dPipelineState.reset(ur_null);
		this->d3dRootSignature.reset(ur_null);

		return Result(Success);
	}

	Result GrafComputePipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafComputePipeline::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafComputePipelineDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		D3D12_COMPUTE_PIPELINE_STATE_DESC d3dPipelineDesc = {};

		d3dPipelineDesc.NodeMask = 0;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		// initialize root signature

		D3D12_ROOT_SIGNATURE_DESC d3dRootSigDesc = {};
		d3dRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		std::vector<D3D12_ROOT_PARAMETER> d3dRootParameters;
		for (ur_uint descriptorLayoutIdx = 0; descriptorLayoutIdx < initParams.DescriptorTableLayoutCount; ++descriptorLayoutIdx)
		{
			GrafDescriptorTableLayoutDX12* tableLayoutDX12 = static_cast<GrafDescriptorTableLayoutDX12*>(initParams.DescriptorTableLayouts[descriptorLayoutIdx]);
			const D3D12_ROOT_PARAMETER& tableSrvUavCbvD3DRootPatameter = tableLayoutDX12->GetSrvUavCbvTableD3DRootParameter();
			if (tableSrvUavCbvD3DRootPatameter.DescriptorTable.NumDescriptorRanges > 0)
			{
				d3dRootParameters.emplace_back(tableSrvUavCbvD3DRootPatameter);
				d3dRootSigDesc.NumParameters += 1;
			}
			const D3D12_ROOT_PARAMETER& tableSamplerD3DRootPatameter = tableLayoutDX12->GetSamplerTableD3DRootParameter();
			if (tableSamplerD3DRootPatameter.DescriptorTable.NumDescriptorRanges > 0)
			{
				d3dRootParameters.emplace_back(tableSamplerD3DRootPatameter);
				d3dRootSigDesc.NumParameters += 1;
			}
		}
		d3dRootSigDesc.pParameters = &d3dRootParameters.front();

		shared_ref<ID3DBlob> d3dSignatureBlob;
		shared_ref<ID3DBlob> d3dErrorBlob;
		HRESULT hres = D3D12SerializeRootSignature(&d3dRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, d3dSignatureBlob, d3dErrorBlob);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafComputePipelineDX12: D3D12SerializeRootSignature failed with HRESULT = ") + HResultToString(hres));
		}

		hres = d3dDevice->CreateRootSignature(d3dPipelineDesc.NodeMask, d3dSignatureBlob->GetBufferPointer(), d3dSignatureBlob->GetBufferSize(),
			__uuidof(this->d3dRootSignature), this->d3dRootSignature);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafComputePipelineDX12: CreateRootSignature failed with HRESULT = ") + HResultToString(hres));
		}

		d3dPipelineDesc.pRootSignature = this->d3dRootSignature.get();

		// initialize shader

		GrafShaderDX12* grafShaderDX12 = static_cast<GrafShaderDX12*>(initParams.ShaderStage);
		if (grafShaderDX12 != nullptr)
		{
			d3dPipelineDesc.CS.pShaderBytecode = grafShaderDX12->GetByteCodePtr();
			d3dPipelineDesc.CS.BytecodeLength = (SIZE_T)grafShaderDX12->GetByteCodeSize();
		}

		// create pipeline

		hres = d3dDevice->CreateComputePipelineState(&d3dPipelineDesc, __uuidof(ID3D12PipelineState), this->d3dPipelineState);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafComputePipelineDX12: CreateComputePipelineState failed with HRESULT = ") + HResultToString(hres));
		}

		return Result(Success);
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
		this->d3dRootSignature.reset(ur_null);
		this->d3dStateObject.reset(ur_null);

		return Result(Success);
	}

	Result GrafRayTracingPipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafRayTracingPipeline::Initialize(grafDevice, initParams);

		// validate device

		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafRayTracingPipelineDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// shader library

		GrafShaderLibDX12* shaderLibDX12 = static_cast<GrafShaderLibDX12*>(initParams.ShaderLib);
		if (ur_null == shaderLibDX12)
		{
			return ResultError(InvalidArgs, std::string("GrafRayTracingPipelineDX12: failed to initialize, shader lib can not be null"));
		}

		std::vector<std::wstring> shaderLibEntryPoints;
		shaderLibEntryPoints.resize(initParams.ShaderStageCount);
		for (ur_uint ishader = 0; ishader < initParams.ShaderStageCount; ++ishader)
		{
			StringToWstring(initParams.ShaderStages[ishader]->GetEntryPoint(), shaderLibEntryPoints[ishader]);
		}

		std::vector<D3D12_EXPORT_DESC> d3dShaderLibExports;
		d3dShaderLibExports.resize(initParams.ShaderStageCount);
		for (ur_uint ishader = 0; ishader < initParams.ShaderStageCount; ++ishader)
		{
			D3D12_EXPORT_DESC& d3dExportDesc = d3dShaderLibExports[ishader];
			d3dExportDesc = {};
			d3dExportDesc.Name = shaderLibEntryPoints[ishader].c_str();
		}

		D3D12_DXIL_LIBRARY_DESC d3dShaderLibDesc = {};
		d3dShaderLibDesc.NumExports = (UINT)d3dShaderLibExports.size();
		d3dShaderLibDesc.pExports = d3dShaderLibExports.data();
		d3dShaderLibDesc.DXILLibrary.pShaderBytecode = shaderLibDX12->GetByteCodePtr();
		d3dShaderLibDesc.DXILLibrary.BytecodeLength = shaderLibDX12->GetByteCodeSize();

		// shader hit groups

		std::vector<D3D12_HIT_GROUP_DESC> d3dHitGroups;
		std::vector<std::wstring> hitGroupNames;
		char hitGroupScratch[16];
		d3dHitGroups.reserve(initParams.ShaderGroupCount);
		hitGroupNames.reserve(initParams.ShaderGroupCount);
		for (ur_uint igroup = 0; igroup < initParams.ShaderGroupCount; ++igroup)
		{
			const GrafRayTracingShaderGroupDesc& grafGroupDesc = initParams.ShaderGroups[igroup];
			if (GrafRayTracingShaderGroupType::TrianglesHit == grafGroupDesc.Type)
			{
				d3dHitGroups.resize(d3dHitGroups.size() + 1);
				hitGroupNames.resize(hitGroupNames.size() + 1);
				D3D12_HIT_GROUP_DESC& d3dHitGroupDesc = d3dHitGroups.back();
				d3dHitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
				std::snprintf(hitGroupScratch, ur_array_size(hitGroupScratch), "hit_group_%i", igroup);
				StringToWstring(hitGroupScratch, hitGroupNames.back());
				d3dHitGroupDesc.HitGroupExport = hitGroupNames.back().c_str();
				if (grafGroupDesc.AnyHitShaderIdx != GrafRayTracingShaderGroupDesc::UnusedShaderIdx)
					d3dHitGroupDesc.AnyHitShaderImport = shaderLibEntryPoints[grafGroupDesc.AnyHitShaderIdx].c_str();
				if (grafGroupDesc.ClosestHitShaderIdx != GrafRayTracingShaderGroupDesc::UnusedShaderIdx)
					d3dHitGroupDesc.ClosestHitShaderImport = shaderLibEntryPoints[grafGroupDesc.ClosestHitShaderIdx].c_str();
			}
			else if (GrafRayTracingShaderGroupType::ProceduralHit == grafGroupDesc.Type)
			{
				d3dHitGroups.resize(d3dHitGroups.size() + 1);
				hitGroupNames.resize(hitGroupNames.size() + 1);
				D3D12_HIT_GROUP_DESC& d3dHitGroupDesc = d3dHitGroups.back();
				d3dHitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
				std::snprintf(hitGroupScratch, ur_array_size(hitGroupScratch), "hit_group_%i", igroup);
				StringToWstring(hitGroupScratch, hitGroupNames.back());
				d3dHitGroupDesc.HitGroupExport = hitGroupNames.back().c_str();
				if (grafGroupDesc.IntersectionShaderIdx != GrafRayTracingShaderGroupDesc::UnusedShaderIdx)
					d3dHitGroupDesc.IntersectionShaderImport = shaderLibEntryPoints[grafGroupDesc.IntersectionShaderIdx].c_str();
			}
		}

		// shader config

		D3D12_RAYTRACING_SHADER_CONFIG d3dShaderConfig = {};
		d3dShaderConfig.MaxPayloadSizeInBytes = sizeof(ur_float) * 4 + 16; // float4 color + 16 bytes of custom data
		d3dShaderConfig.MaxAttributeSizeInBytes = sizeof(ur_float) * 2; // float2 barycentrics

		// pipeline config

		D3D12_RAYTRACING_PIPELINE_CONFIG d3dPipelineConfig = {};
		d3dPipelineConfig.MaxTraceRecursionDepth = initParams.MaxRecursionDepth;

		// root signature

		D3D12_ROOT_SIGNATURE_DESC d3dRootSigDesc = {};
		d3dRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		std::vector<D3D12_ROOT_PARAMETER> d3dRootParameters;
		for (ur_uint descriptorLayoutIdx = 0; descriptorLayoutIdx < initParams.DescriptorTableLayoutCount; ++descriptorLayoutIdx)
		{
			GrafDescriptorTableLayoutDX12* tableLayoutDX12 = static_cast<GrafDescriptorTableLayoutDX12*>(initParams.DescriptorTableLayouts[descriptorLayoutIdx]);
			const D3D12_ROOT_PARAMETER& tableSrvUavCbvD3DRootPatameter = tableLayoutDX12->GetSrvUavCbvTableD3DRootParameter();
			if (tableSrvUavCbvD3DRootPatameter.DescriptorTable.NumDescriptorRanges > 0)
			{
				d3dRootParameters.emplace_back(tableSrvUavCbvD3DRootPatameter);
				d3dRootSigDesc.NumParameters += 1;
			}
			const D3D12_ROOT_PARAMETER& tableSamplerD3DRootPatameter = tableLayoutDX12->GetSamplerTableD3DRootParameter();
			if (tableSamplerD3DRootPatameter.DescriptorTable.NumDescriptorRanges > 0)
			{
				d3dRootParameters.emplace_back(tableSamplerD3DRootPatameter);
				d3dRootSigDesc.NumParameters += 1;
			}
		}
		d3dRootSigDesc.pParameters = &d3dRootParameters.front();

		shared_ref<ID3DBlob> d3dSignatureBlob;
		shared_ref<ID3DBlob> d3dErrorBlob;
		HRESULT hres = D3D12SerializeRootSignature(&d3dRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, d3dSignatureBlob, d3dErrorBlob);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRayTracingPipelineDX12: D3D12SerializeRootSignature failed with HRESULT = ") + HResultToString(hres));
		}

		UINT nodeMask = 0;
		hres = d3dDevice->CreateRootSignature(nodeMask, d3dSignatureBlob->GetBufferPointer(), d3dSignatureBlob->GetBufferSize(),
			__uuidof(this->d3dRootSignature), this->d3dRootSignature);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRayTracingPipelineDX12: CreateRootSignature failed with HRESULT = ") + HResultToString(hres));
		}

		D3D12_GLOBAL_ROOT_SIGNATURE d3dRootSignature = {};
		d3dRootSignature.pGlobalRootSignature = this->d3dRootSignature.get();

		// state object

		std::vector<D3D12_STATE_SUBOBJECT> d3dStateSubobjects;
		d3dStateSubobjects.reserve(4 + d3dHitGroups.size());
		d3dStateSubobjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &d3dRootSignature);
		d3dStateSubobjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &d3dPipelineConfig);
		d3dStateSubobjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &d3dShaderLibDesc);
		d3dStateSubobjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &d3dShaderConfig);
		for (D3D12_HIT_GROUP_DESC& d3dHitGroup : d3dHitGroups)
		{
			d3dStateSubobjects.emplace_back(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &d3dHitGroup);
		}

		D3D12_STATE_OBJECT_DESC d3dStateDesc = {};
		d3dStateDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		d3dStateDesc.pSubobjects = d3dStateSubobjects.data();
		d3dStateDesc.NumSubobjects = (UINT)d3dStateSubobjects.size();

		hres = d3dDevice->CreateStateObject(&d3dStateDesc, __uuidof(ID3D12StateObject), this->d3dStateObject);
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRayTracingPipelineDX12: CreateStateObject failed with HRESULT = ") + HResultToString(hres));
		}

		return Result(Success);
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
		this->grafScratchBuffer.reset();
		this->grafStorageBuffer.reset();
		this->d3dPrebuildInfo = {};
		this->structureType = GrafAccelerationStructureType::Undefined;
		this->structureBuildFlags = GrafAccelerationStructureBuildFlags(0);
		this->structureDeviceAddress = 0;

		return Result(Success);
	}

	Result GrafAccelerationStructureDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafAccelerationStructure::Initialize(grafDevice, initParams);

		// validate device

		GrafSystemDX12& grafSystemDX12 = static_cast<GrafSystemDX12&>(this->GetGrafSystem());
		GrafDeviceDX12* grafDeviceDX12 = static_cast<GrafDeviceDX12*>(grafDevice);
		if (ur_null == grafDeviceDX12 || ur_null == grafDeviceDX12->GetD3DDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafAccelerationStructureDX12: failed to initialize, invalid GrafDevice"));
		}
		ID3D12Device5* d3dDevice = grafDeviceDX12->GetD3DDevice();

		// request prebuild info

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS d3dBuildInputs = {};
		d3dBuildInputs.Type = GrafUtilsDX12::GrafToD3DAccelerationStructureType(initParams.StructureType);
		d3dBuildInputs.Flags = GrafUtilsDX12::GrafToD3DAccelerationStructureBuildFlags(initParams.BuildFlags);
		d3dBuildInputs.NumDescs = initParams.GeometryCount;
		d3dBuildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> d3dGeometryDescArray;
		if (GrafAccelerationStructureType::TopLevel == initParams.StructureType)
		{
			d3dBuildInputs.InstanceDescs = D3D12_GPU_VIRTUAL_ADDRESS(0); // not required for prebuild
		}
		else if (GrafAccelerationStructureType::BottomLevel == initParams.StructureType)
		{
			d3dGeometryDescArray.resize(initParams.GeometryCount);
			for (ur_uint32 igeom = 0; igeom < initParams.GeometryCount; ++igeom)
			{
				GrafAccelerationStructureGeometryDesc& grafGeometryDesc = initParams.Geometry[igeom];
				D3D12_RAYTRACING_GEOMETRY_DESC& d3dGeometryDesc = d3dGeometryDescArray[igeom];
				d3dGeometryDesc = {};
				d3dGeometryDesc.Type = GrafUtilsDX12::GrafToD3DAccelerationStructureGeometryType(grafGeometryDesc.GeometryType);
				d3dGeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
				if (GrafAccelerationStructureGeometryType::Triangles == grafGeometryDesc.GeometryType)
				{
					D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& d3dGeometryTriangles = d3dGeometryDesc.Triangles;
					d3dGeometryTriangles.IndexFormat = GrafUtilsDX12::GrafToDXGIFormat(grafGeometryDesc.IndexType);
					d3dGeometryTriangles.VertexFormat = GrafUtilsDX12::GrafToDXGIFormat(grafGeometryDesc.VertexFormat);
					d3dGeometryTriangles.IndexCount = grafGeometryDesc.PrimitiveCountMax * 3;
					d3dGeometryTriangles.VertexCount = grafGeometryDesc.VertexCountMax;
					d3dGeometryTriangles.VertexBuffer.StrideInBytes = grafGeometryDesc.VertexStride;
					d3dGeometryTriangles.Transform3x4 = D3D12_GPU_VIRTUAL_ADDRESS(0); // not required for prebuild
					d3dGeometryTriangles.IndexBuffer = D3D12_GPU_VIRTUAL_ADDRESS(0);
					d3dGeometryTriangles.VertexBuffer.StartAddress = D3D12_GPU_VIRTUAL_ADDRESS(0);
				}
				else if (GrafAccelerationStructureGeometryType::AABBs == grafGeometryDesc.GeometryType)
				{
					D3D12_RAYTRACING_GEOMETRY_AABBS_DESC& d3dGeometryAabbs = d3dGeometryDesc.AABBs;
					d3dGeometryAabbs.AABBCount = grafGeometryDesc.PrimitiveCountMax;
					d3dGeometryAabbs.AABBs.StrideInBytes = 0;
					d3dGeometryAabbs.AABBs.StartAddress = D3D12_GPU_VIRTUAL_ADDRESS(0); // not required for prebuild
				}
			}
			d3dBuildInputs.pGeometryDescs = d3dGeometryDescArray.data();
		}

		d3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(&d3dBuildInputs, &this->d3dPrebuildInfo);

		// storage buffer for acceleration structure

		Result res = grafSystemDX12.CreateBuffer(this->grafStorageBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams storageBufferParams = {};
			storageBufferParams.BufferDesc.Usage = GrafBufferUsageFlags(GrafBufferUsageFlag::AccelerationStructure) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress) | ur_uint(GrafBufferUsageFlag::StorageBuffer);
			storageBufferParams.BufferDesc.MemoryType = GrafDeviceMemoryFlags(GrafDeviceMemoryFlag::GpuLocal);
			storageBufferParams.BufferDesc.SizeInBytes = this->d3dPrebuildInfo.ResultDataMaxSizeInBytes;

			res = this->grafStorageBuffer->Initialize(grafDevice, storageBufferParams);
		}
		if (Failed(res))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafAccelerationStructureDX12: failed to create storage buffer");
		}
		this->structureDeviceAddress = this->grafStorageBuffer->GetDeviceAddress();

		// scratch buffer

		res = grafSystemDX12.CreateBuffer(this->grafScratchBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams scrathBufferParams = {};
			scrathBufferParams.BufferDesc.Usage = GrafBufferUsageFlags(ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress) | ur_uint(GrafBufferUsageFlag::StorageBuffer));
			scrathBufferParams.BufferDesc.MemoryType = GrafDeviceMemoryFlags(GrafDeviceMemoryFlag::GpuLocal);
			scrathBufferParams.BufferDesc.SizeInBytes = std::max(this->d3dPrebuildInfo.ScratchDataSizeInBytes, this->d3dPrebuildInfo.UpdateScratchDataSizeInBytes);

			res = this->grafScratchBuffer->Initialize(grafDeviceDX12, scrathBufferParams);
		}
		if (Failed(res))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafAccelerationStructureDX12: failed to create scratch buffer");
		}

		return Result(Success);
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
		case GrafImageState::ColorClear:
			d3dState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			break;
		case GrafImageState::ColorWrite:
			d3dState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			break;
		case GrafImageState::DepthStencilClear:
			d3dState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
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

	D3D12_RESOURCE_DIMENSION GrafUtilsDX12::GrafToD3DResDimension(GrafImageType imageType)
	{
		D3D12_RESOURCE_DIMENSION d3dResDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		switch (imageType)
		{
		case GrafImageType::Tex1D: d3dResDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
		case GrafImageType::Tex2D: d3dResDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
		case GrafImageType::Tex3D: d3dResDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
		};
		return d3dResDimension;
	}

	D3D12_RESOURCE_FLAGS GrafUtilsDX12::GrafToD3DResourceFlags(GrafImageUsageFlags imageUsageFlags)
	{
		D3D12_RESOURCE_FLAGS d3dResFlags = D3D12_RESOURCE_FLAG_NONE;
		if (imageUsageFlags & ur_uint(GrafImageUsageFlag::ColorRenderTarget))
			d3dResFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if (imageUsageFlags & ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget))
			d3dResFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if (imageUsageFlags & ur_uint(GrafImageUsageFlag::ShaderReadWrite))
			d3dResFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		return d3dResFlags;
	}

	D3D12_RTV_DIMENSION GrafUtilsDX12::GrafToD3DRTVDimension(GrafImageType imageType)
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

	D3D12_DSV_DIMENSION GrafUtilsDX12::GrafToD3DDSVDimension(GrafImageType imageType)
	{
		D3D12_DSV_DIMENSION d3dDSVDimension = D3D12_DSV_DIMENSION_UNKNOWN;
		switch (imageType)
		{
		case GrafImageType::Tex1D: d3dDSVDimension = D3D12_DSV_DIMENSION_TEXTURE1D; break;
		case GrafImageType::Tex2D: d3dDSVDimension = D3D12_DSV_DIMENSION_TEXTURE2D; break;
		};
		return d3dDSVDimension;
	}

	D3D12_SRV_DIMENSION GrafUtilsDX12::GrafToD3DSRVDimension(GrafImageType imageType)
	{
		D3D12_SRV_DIMENSION d3dSRVDimension = D3D12_SRV_DIMENSION_UNKNOWN;
		switch (imageType)
		{
		case GrafImageType::Tex1D: d3dSRVDimension = D3D12_SRV_DIMENSION_TEXTURE1D; break;
		case GrafImageType::Tex2D: d3dSRVDimension = D3D12_SRV_DIMENSION_TEXTURE2D; break;
		case GrafImageType::Tex3D: d3dSRVDimension = D3D12_SRV_DIMENSION_TEXTURE3D; break;
		};
		return d3dSRVDimension;
	}

	D3D12_UAV_DIMENSION GrafUtilsDX12::GrafToD3DUAVDimension(GrafImageType imageType)
	{
		D3D12_UAV_DIMENSION d3dUAVDimension = D3D12_UAV_DIMENSION_UNKNOWN;
		switch (imageType)
		{
		case GrafImageType::Tex1D: d3dUAVDimension = D3D12_UAV_DIMENSION_TEXTURE1D; break;
		case GrafImageType::Tex2D: d3dUAVDimension = D3D12_UAV_DIMENSION_TEXTURE2D; break;
		case GrafImageType::Tex3D: d3dUAVDimension = D3D12_UAV_DIMENSION_TEXTURE3D; break;
		};
		return d3dUAVDimension;
	}

	D3D12_HEAP_TYPE GrafUtilsDX12::GrafToD3DBufferHeapType(GrafBufferUsageFlags bufferUsage, GrafDeviceMemoryFlags memoryFlags)
	{
		D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (memoryFlags & ur_uint(GrafDeviceMemoryFlag::CpuVisible))
		{
			// by default CPU visible memory is considered to be used for upload,
			// readback is expected to be done from a copy destination resource (TransferDst)
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferDst))
				d3dHeapType = D3D12_HEAP_TYPE_READBACK;
			else
				d3dHeapType = D3D12_HEAP_TYPE_UPLOAD;
		}
		return d3dHeapType;
	}

	D3D12_RESOURCE_STATES GrafUtilsDX12::GrafToD3DBufferInitialState(GrafBufferUsageFlags bufferUsage, GrafDeviceMemoryFlags memoryFlags)
	{
		D3D12_RESOURCE_STATES d3dStates = D3D12_RESOURCE_STATE_COMMON;
		// by default CPU visible memory is considered to be used for upload,
		// upload heap resource must be created generic read state
		if (memoryFlags & ur_uint(GrafDeviceMemoryFlag::CpuVisible))
		{
			d3dStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else
		{
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::AccelerationStructure))
				d3dStates = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			else
				d3dStates = D3D12_RESOURCE_STATE_COMMON; // D3D12: Buffers are effectively created in state D3D12_RESOURCE_STATE_COMMON
			/*if (bufferUsage & ur_uint(GrafBufferUsageFlag::VertexBuffer))
				d3dStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::IndexBuffer))
				d3dStates = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::ConstantBuffer))
				d3dStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::StorageBuffer))
				d3dStates = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::AccelerationStructure))
				d3dStates = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferSrc))
				d3dStates = D3D12_RESOURCE_STATE_COPY_SOURCE;
			if (bufferUsage & ur_uint(GrafBufferUsageFlag::TransferDst))
				d3dStates = D3D12_RESOURCE_STATE_COPY_DEST;*/
		}
		return d3dStates;
	}

	D3D12_HEAP_TYPE GrafUtilsDX12::GrafToD3DImageHeapType(GrafImageUsageFlags imageUsage, GrafDeviceMemoryFlags memoryFlags)
	{
		D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE_DEFAULT;
		// D3D12: A texture resource cannot be created on a D3D12_HEAP_TYPE_UPLOAD or D3D12_HEAP_TYPE_READBACK heap.
		return d3dHeapType;
	}

	D3D12_RESOURCE_STATES GrafUtilsDX12::GrafToD3DImageInitialState(GrafImageUsageFlags imageUsage, GrafDeviceMemoryFlags memoryFlags)
	{
		D3D12_RESOURCE_STATES d3dStates = D3D12_RESOURCE_STATE_COMMON;
		if (imageUsage & ur_uint(GrafImageUsageFlag::TransferSrc))
			d3dStates = D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (imageUsage & ur_uint(GrafImageUsageFlag::TransferDst))
			d3dStates = D3D12_RESOURCE_STATE_COPY_DEST;
		if (imageUsage & ur_uint(GrafImageUsageFlag::ColorRenderTarget))
			d3dStates = D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (imageUsage & ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget))
			d3dStates = D3D12_RESOURCE_STATE_DEPTH_WRITE;
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

	D3D12_BLEND_OP GrafUtilsDX12::GrafToD3DBlendOp(GrafBlendOp blendOp)
	{
		D3D12_BLEND_OP d3dBlendOp = D3D12_BLEND_OP(0);
		switch (blendOp)
		{
		case GrafBlendOp::Add: d3dBlendOp = D3D12_BLEND_OP_ADD; break;
		case GrafBlendOp::Subtract: d3dBlendOp = D3D12_BLEND_OP_SUBTRACT; break;
		case GrafBlendOp::ReverseSubtract: d3dBlendOp = D3D12_BLEND_OP_REV_SUBTRACT; break;
		case GrafBlendOp::Min: d3dBlendOp = D3D12_BLEND_OP_MIN; break;
		case GrafBlendOp::Max: d3dBlendOp = D3D12_BLEND_OP_MAX; break;
		};
		return d3dBlendOp;
	}

	D3D12_BLEND GrafUtilsDX12::GrafToD3DBlendFactor(GrafBlendFactor blendFactor)
	{
		D3D12_BLEND d3dBlendFactor = D3D12_BLEND(0);
		switch (blendFactor)
		{
		case GrafBlendFactor::Zero: d3dBlendFactor = D3D12_BLEND_ZERO; break;
		case GrafBlendFactor::One: d3dBlendFactor = D3D12_BLEND_ONE; break;
		case GrafBlendFactor::SrcColor: d3dBlendFactor = D3D12_BLEND_SRC_COLOR; break;
		case GrafBlendFactor::InvSrcColor: d3dBlendFactor = D3D12_BLEND_INV_SRC_COLOR; break;
		case GrafBlendFactor::DstColor: d3dBlendFactor = D3D12_BLEND_DEST_COLOR; break;
		case GrafBlendFactor::InvDstColor: d3dBlendFactor = D3D12_BLEND_INV_DEST_COLOR; break;
		case GrafBlendFactor::SrcAlpha: d3dBlendFactor = D3D12_BLEND_SRC_ALPHA; break;
		case GrafBlendFactor::InvSrcAlpha: d3dBlendFactor = D3D12_BLEND_INV_SRC_ALPHA; break;
		case GrafBlendFactor::DstAlpha: d3dBlendFactor = D3D12_BLEND_DEST_ALPHA; break;
		case GrafBlendFactor::InvDstAlpha: d3dBlendFactor = D3D12_BLEND_INV_DEST_ALPHA; break;
		};
		return d3dBlendFactor;
	}

	D3D12_CULL_MODE GrafUtilsDX12::GrafToD3DCullMode(GrafCullMode cullMode)
	{
		D3D12_CULL_MODE d3dCullMode = D3D12_CULL_MODE(0);
		switch (cullMode)
		{
		case GrafCullMode::None: d3dCullMode = D3D12_CULL_MODE_NONE; break;
		case GrafCullMode::Front: d3dCullMode = D3D12_CULL_MODE_FRONT; break;
		case GrafCullMode::Back: d3dCullMode = D3D12_CULL_MODE_BACK; break;
		};
		return d3dCullMode;
	}

	D3D12_COMPARISON_FUNC GrafUtilsDX12::GrafToD3DCompareOp(GrafCompareOp compareOp)
	{
		D3D12_COMPARISON_FUNC d3dCompareOp = D3D12_COMPARISON_FUNC(0);
		switch (compareOp)
		{
		case GrafCompareOp::Never: d3dCompareOp = D3D12_COMPARISON_FUNC_NEVER; break;
		case GrafCompareOp::Less: d3dCompareOp = D3D12_COMPARISON_FUNC_LESS; break;
		case GrafCompareOp::Equal: d3dCompareOp = D3D12_COMPARISON_FUNC_EQUAL; break;
		case GrafCompareOp::LessOrEqual: d3dCompareOp = D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
		case GrafCompareOp::Greater: d3dCompareOp = D3D12_COMPARISON_FUNC_GREATER; break;
		case GrafCompareOp::NotEqual: d3dCompareOp = D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
		case GrafCompareOp::GreaterOrEqual: d3dCompareOp = D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
		case GrafCompareOp::Always: d3dCompareOp = D3D12_COMPARISON_FUNC_ALWAYS; break;
		};
		return d3dCompareOp;
	}

	D3D12_STENCIL_OP GrafUtilsDX12::GrafToD3DStencilOp(GrafStencilOp stencilOp)
	{
		D3D12_STENCIL_OP d3dStencilOp = D3D12_STENCIL_OP(0);
		switch (stencilOp)
		{
		case GrafStencilOp::Keep: d3dStencilOp = D3D12_STENCIL_OP_KEEP; break;
		case GrafStencilOp::Zero: d3dStencilOp = D3D12_STENCIL_OP_ZERO; break;
		case GrafStencilOp::Replace: d3dStencilOp = D3D12_STENCIL_OP_REPLACE; break;
		};
		return d3dStencilOp;
	}

	D3D12_PRIMITIVE_TOPOLOGY GrafUtilsDX12::GrafToD3DPrimitiveTopology(GrafPrimitiveTopology topology)
	{
		D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		switch (topology)
		{
		case GrafPrimitiveTopology::PointList: d3dTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
		case GrafPrimitiveTopology::LineList: d3dTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
		case GrafPrimitiveTopology::LineStrip: d3dTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
		case GrafPrimitiveTopology::TriangleList: d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
		case GrafPrimitiveTopology::TriangleStrip: d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
		case GrafPrimitiveTopology::TriangleFan: d3dTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED; break;
		};
		return d3dTopology;
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE GrafUtilsDX12::GrafToD3DPrimitiveTopologyType(GrafPrimitiveTopology topology)
	{
		D3D12_PRIMITIVE_TOPOLOGY_TYPE d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		switch (topology)
		{
		case GrafPrimitiveTopology::PointList: d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; break;
		case GrafPrimitiveTopology::LineList: d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
		case GrafPrimitiveTopology::LineStrip: d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
		case GrafPrimitiveTopology::TriangleList: d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
		case GrafPrimitiveTopology::TriangleStrip: d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
		case GrafPrimitiveTopology::TriangleFan: d3dTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
		};
		return d3dTopologyType;
	}

	const char* GrafUtilsDX12::ParseVertexElementSemantic(const std::string& semantic, std::string& semanticName, ur_uint& semanticIdx)
	{
		semanticIdx = 0;
		const char* Digits = "0123456789";
		size_t idxPosFirst = semantic.find_first_of(Digits);
		if (idxPosFirst != std::string::npos)
		{
			size_t idxPosLast = semantic.find_first_not_of(Digits, idxPosFirst);
			semanticIdx = std::stoi(semantic.substr(idxPosFirst, (idxPosLast != std::string::npos ? idxPosLast - idxPosFirst : idxPosLast)));
			semanticName = (idxPosFirst > 0 ? semantic.substr(0, idxPosFirst) : "");
		}
		else
		{
			semanticName = semantic;
		}
		return semanticName.c_str();
	}

	D3D12_RAYTRACING_GEOMETRY_TYPE GrafUtilsDX12::GrafToD3DAccelerationStructureGeometryType(GrafAccelerationStructureGeometryType geometryType)
	{
		D3D12_RAYTRACING_GEOMETRY_TYPE d3dGeometryType = D3D12_RAYTRACING_GEOMETRY_TYPE(-1);
		switch (geometryType)
		{
		case GrafAccelerationStructureGeometryType::Triangles: d3dGeometryType = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES; break;
		case GrafAccelerationStructureGeometryType::AABBs: d3dGeometryType = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS; break;
		};
		return d3dGeometryType;
	}

	D3D12_RAYTRACING_GEOMETRY_FLAGS GrafUtilsDX12::GrafToD3DAccelerationStructureGeometryFlags(GrafAccelerationStructureGeometryFlags geometryFlags)
	{
		D3D12_RAYTRACING_GEOMETRY_FLAGS d3dGeometryFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		if (ur_uint(GrafAccelerationStructureGeometryFlag::Opaque) & geometryFlags)
			d3dGeometryFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		if (ur_uint(GrafAccelerationStructureGeometryFlag::NoDuplicateAnyHit) & geometryFlags)
			d3dGeometryFlags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
		return d3dGeometryFlags;
	}

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE GrafUtilsDX12::GrafToD3DAccelerationStructureType(GrafAccelerationStructureType structureType)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE d3dAccelStructType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE(-1);
		switch (structureType)
		{
		case GrafAccelerationStructureType::TopLevel: d3dAccelStructType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL; break;
		case GrafAccelerationStructureType::BottomLevel: d3dAccelStructType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL; break;
		};
		return d3dAccelStructType;
	}

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS GrafUtilsDX12::GrafToD3DAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlags buildFlags)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS d3dAccelStructBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		if (ur_uint(GrafAccelerationStructureBuildFlag::AllowUpdate) & buildFlags)
			d3dAccelStructBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		if (ur_uint(GrafAccelerationStructureBuildFlag::AllowCompaction) & buildFlags)
			d3dAccelStructBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		if (ur_uint(GrafAccelerationStructureBuildFlag::PreferFastTrace) & buildFlags)
			d3dAccelStructBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		if (ur_uint(GrafAccelerationStructureBuildFlag::PreferFastBuild) & buildFlags)
			d3dAccelStructBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		if (ur_uint(GrafAccelerationStructureBuildFlag::MinimizeMemory) & buildFlags)
			d3dAccelStructBuildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		return d3dAccelStructBuildFlags;
	}

	DXGI_FORMAT GrafUtilsDX12::GrafToDXGIFormat(GrafIndexType indexType)
	{
		DXGI_FORMAT dxgiIndexFormat = DXGI_FORMAT_UNKNOWN;
		switch (indexType)
		{
		case GrafIndexType::UINT16: dxgiIndexFormat = DXGI_FORMAT_R16_UINT; break;
		case GrafIndexType::UINT32: dxgiIndexFormat = DXGI_FORMAT_R32_UINT; break;
		};
		return dxgiIndexFormat;
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