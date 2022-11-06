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

	// command buffer synchronisation policy
	#define UR_GRAF_DX12_COMMAND_LIST_SYNC_DESTROY	1
	#define UR_GRAF_DX12_COMMAND_LIST_SYNC_RESET	1
	#define UR_GRAF_DX12_SLEEPZERO_WHILE_WAIT		0

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

		// create descriptor heaps

		// TODO

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
				ID3D12CommandList* d3dCommandList = grafCommandListDX12->GetD3DCommandList();
				
				this->graphicsQueue->d3dQueue->ExecuteCommandLists(1, &d3dCommandList);
				
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

		GrafImageDX12* grafImageVulkan = static_cast<GrafImageDX12*>(grafImage);
		Result res = this->ImageMemoryBarrier(grafImageVulkan->GetDefaultSubresource(), srcState, dstState);
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
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset)
	{
		return Result(NotImplemented);
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
			return ResultError(InvalidArgs, std::string("GrafCanvasVulkan: failed to initialize, invalid WinCanvas"));
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
		LogNoteGrafDbg("GrafCanvasVulkan: VkSurfaceKHR created");

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
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::InitializeFromD3DResource(GrafDevice *grafDevice, const InitParams& initParams, ID3D12Resource* d3dResource)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::CreateDefaultSubresource()
	{
		return Result(NotImplemented);
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
		return Result(NotImplemented);
	}

	Result GrafImageSubresourceDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafImageSubresourceDX12::CreateD3DImageView()
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafBufferDX12::GrafBufferDX12(GrafSystem &grafSystem) :
		GrafBuffer(grafSystem)
	{
	}

	GrafBufferDX12::~GrafBufferDX12()
	{
		this->Deinitialize();
	}

	Result GrafBufferDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
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
		return Result(NotImplemented);
	}

	Result GrafSamplerDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
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
		return Result(NotImplemented);
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
		return Result(NotImplemented);
	}

	Result GrafRenderTargetDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
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
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableLayoutDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableDX12::GrafDescriptorTableDX12(GrafSystem &grafSystem) :
		GrafDescriptorTable(grafSystem)
	{
	}

	GrafDescriptorTableDX12::~GrafDescriptorTableDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs, ur_size bufferRange)
	{
		return Result(NotImplemented);
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
		return Result(NotImplemented);
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
		return Result(NotImplemented);
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
		return Result(NotImplemented);
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
		return Result(NotImplemented);
	}

	Result GrafRenderPassDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
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