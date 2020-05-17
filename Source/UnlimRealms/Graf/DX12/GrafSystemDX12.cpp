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
		// TODO
		return (FAILED(res) ? "FAILED" : "SUCCEEDED");
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
			grafDeviceDesc.DedicatedVideoMemory = (ur_size)dxgiAdapterDesc.DedicatedVideoMemory;
			grafDeviceDesc.SharedSystemMemory = (ur_size)dxgiAdapterDesc.SharedSystemMemory;
			grafDeviceDesc.DedicatedSystemMemory = (ur_size)dxgiAdapterDesc.DedicatedSystemMemory;
			grafDeviceDesc.ConstantBufferOffsetAlignment = 256;

			#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
			LogNoteGrafDbg(std::string("GrafSystemDX12: device available ") + grafDeviceDesc.Description +
				", VRAM = " + std::to_string(grafDeviceDesc.DedicatedVideoMemory >> 20) + " Mb");
			#endif

			// create temporary device object to check features support

			shared_ref<ID3D12Device5> d3dDevice;
			hres = D3D12CreateDevice(dxgiAdapters[iadapter], D3D_FEATURE_LEVEL_11_0, __uuidof(d3dDevice), d3dDevice);
			if (FAILED(hres))
				continue;

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3dOptions5;
			hres = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &d3dOptions5, sizeof(d3dOptions5));
			if (FAILED(hres))
				continue;

			grafDeviceDesc.RayTracing.RayTraceSupported = (ur_bool)(d3dOptions5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
			grafDeviceDesc.RayTracing.RayQuerySupported = (ur_bool)false;
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

		// create common descriptor heap(s)

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

	Result GrafCommandListDX12::BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::SetFenceState(GrafFence* grafFence, GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
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

	Result GrafCommandListDX12::Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion, BoxI dstRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindComputePipeline(GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
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

		return Result(NotImplemented);
	}

	Result GrafCanvasDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
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

	Result GrafDescriptorTableDX12::SetSampler(ur_uint bindingIdx, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetRWImage(ur_uint bindingIdx, GrafImage* image)
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

} // end namespace UnlimRealms