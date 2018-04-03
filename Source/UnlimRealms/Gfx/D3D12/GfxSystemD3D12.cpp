///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Gfx/D3D12/GfxSystemD3D12.h"
#include "Gfx/D3D12/d3dx12.h"
#include "Gfx/DXGIUtils/DXGIUtils.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSystemD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSystemD3D12::GfxSystemD3D12(Realm &realm) :
		GfxSystem(realm)
	{
		this->winCanvas = ur_null;
		this->frameIndex = 0;
		this->framesCount = 0;
		this->frameFenceEvent = CreateEvent(ur_null, false, false, ur_null);
	}

	GfxSystemD3D12::~GfxSystemD3D12()
	{
		this->WaitGPU();
		CloseHandle(this->frameFenceEvent);
	}

	Result GfxSystemD3D12::InitializeDXGI()
	{
		this->ReleaseDXGIObjects();

		UINT dxgiFactoryFlags = 0;
		#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			shared_ref<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), debugController)))
			{
				debugController->EnableDebugLayer();

				// Enable additional debug layers.
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
		#endif

		HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(&this->dxgiFactory));
		if (FAILED(hr))
			return ResultError(Failure, "GfxSystemD3D12: failed to create DXGI factory");

		UINT adapterId = 0;
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		do
		{
			shared_ref<IDXGIAdapter1> dxgiAdapter;
			hr = this->dxgiFactory->EnumAdapters1(adapterId++, dxgiAdapter);
			if (SUCCEEDED(hr))
			{
				dxgiAdapter->GetDesc1(&dxgiAdapterDesc);
				this->dxgiAdapters.push_back(dxgiAdapter);
				this->gfxAdapters.push_back(DXGIAdapterDescToGfx(dxgiAdapterDesc));
			}
		} while (SUCCEEDED(hr));

		return (!this->dxgiAdapters.empty() ?
			Result(Success) :
			ResultError(Failure, "GfxSystemD3D12: failed to enumerate adapaters"));
	}

	Result GfxSystemD3D12::InitializeDevice(IDXGIAdapter *pAdapter, const D3D_FEATURE_LEVEL minimalFeatureLevel)
	{
		this->ReleaseDeviceObjects();

		if (ur_null == pAdapter)
		{
			this->gfxAdapterIdx = 0;
			// try to find high performance GPU
			// let's consider that HP GPU is the one with the highest amount of dedicated VRAM on board
			DXGI_ADAPTER_DESC1 adapterDesc;
			ur_size dedicatedMemoryMax = 0;
			for (ur_size idx = 0; idx < this->dxgiAdapters.size(); ++idx)
			{
				this->dxgiAdapters[idx]->GetDesc1(&adapterDesc);
				ur_size dedicatedMemoryMb = adapterDesc.DedicatedVideoMemory / (1 << 20);
				if (dedicatedMemoryMb > dedicatedMemoryMax)
				{
					dedicatedMemoryMax = dedicatedMemoryMb;
					this->gfxAdapterIdx = ur_uint(idx);
				}
			}
			pAdapter = *(this->dxgiAdapters.begin() + this->gfxAdapterIdx);
		}

		// initialize D3D device

		HRESULT hr = D3D12CreateDevice(pAdapter, minimalFeatureLevel, __uuidof(ID3D12Device), this->d3dDevice);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSystemD3D12: failed to initialize Direct3D device");

		#if defined(_PROFILE)
		this->d3dDevice->SetStablePowerState(true);
		#endif

		// initialize direct command queue

		D3D12_COMMAND_QUEUE_DESC queueDesc;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;
		
		hr = this->d3dDevice->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), this->d3dCommandQueue);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSystemD3D12: failed to create direct command queue");

		return Result(Success);
	}

	Result GfxSystemD3D12::InitializeFrameData(ur_uint framesCount)
	{
		this->WaitGPU();

		this->frameIndex = 0;
		this->framesCount = std::max(framesCount, ur_uint(1));
		this->d3dCommandAllocators.resize(framesCount);
		this->d3dCommandAllocators.shrink_to_fit();

		for (ur_size iframe = 0; iframe < framesCount; ++iframe)
		{
			HRESULT hr = this->d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), this->d3dCommandAllocators[iframe]);
			if (FAILED(hr))
				return ResultError(Failure, "GfxSystemD3D12::InitializeFrameData: failed to create direct command allocator");
		}

		this->frameFenceValues.resize(framesCount);
		memset(this->frameFenceValues.data(), 0, this->frameFenceValues.size() * sizeof(ur_uint));
		this->d3dDevice->CreateFence(this->frameFenceValues[this->frameIndex], D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), this->d3dFrameFence);
		++this->frameFenceValues[this->frameIndex];

		return Result(Success);
	}

	Result GfxSystemD3D12::SetFrame(ur_uint newFrameIndex)
	{
		// add signal command to the current frame's queue
		ur_uint frameFenceValue = this->frameFenceValues[this->frameIndex];
		this->d3dCommandQueue->Signal(this->d3dFrameFence, frameFenceValue);

		// setup new frame
		this->frameIndex = newFrameIndex;
		if (!this->IsFrameComplete(this->frameIndex))
		{
			this->WaitFrame(this->frameIndex);
		}
		this->frameFenceValues[this->frameIndex] = frameFenceValue + 1;

		// prepare frame resources
		this->d3dCommandAllocators[this->frameIndex]->Reset();

		return Result(Success);
	}

	Result GfxSystemD3D12::SetNextFrame()
	{
		ur_uint nextFrameIndex = (this->frameIndex + 1) % this->framesCount;
		return this->SetFrame(nextFrameIndex);
	}

	Result GfxSystemD3D12::WaitFrame(ur_uint frameIndex)
	{
		this->Render(); // force execute any pending command lists

		ur_uint &frameFenceValue = this->frameFenceValues[frameIndex];
		this->d3dCommandQueue->Signal(this->d3dFrameFence, frameFenceValue);
		this->d3dFrameFence->SetEventOnCompletion(frameFenceValue, this->frameFenceEvent);
		WaitForSingleObjectEx(this->frameFenceEvent, INFINITE, false);
		++frameFenceValue;

		return Result(Success);
	}

	Result GfxSystemD3D12::WaitCurrentFrame()
	{
		return WaitFrame(this->frameIndex);
	}

	Result GfxSystemD3D12::WaitGPU()
	{
		// wait all frames
		Result res = Success;
		for (ur_uint iframe = 0; iframe < this->framesCount; ++iframe)
		{
			res &= WaitFrame(iframe);
		}
		return res;
	}

	void GfxSystemD3D12::ReleaseDXGIObjects()
	{
		this->gfxAdapters.clear();
		this->dxgiAdapters.clear();
		this->dxgiFactory.reset(ur_null);
	}

	void GfxSystemD3D12::ReleaseDeviceObjects()
	{
		this->d3dCommandLists.clear();
		this->d3dCommandAllocators.clear();
		this->d3dCommandQueue.reset(ur_null);
		this->d3dDevice.reset(ur_null);
	}

	Result GfxSystemD3D12::Initialize(Canvas *canvas)
	{
		if (ur_null == canvas)
			return ResultError(InvalidArgs, "GfxSystemD3D12: failed to initialize, target canvas is null");

		if (typeid(*canvas).hash_code() != typeid(WinCanvas).hash_code())
			return ResultError(InvalidArgs, "GfxSystemD3D12: failed to initialize, target canvas type is unsupported");

		this->winCanvas = static_cast<WinCanvas*>(canvas);

		// initialize DXGI objects
		Result res = this->InitializeDXGI();
		if (Failed(res))
			return ResultError(res.Code, "GfxSystemD3D12: failed to initialize DXGI objects");

		// initialize D3D device objects
		res = this->InitializeDevice(ur_null, D3D_FEATURE_LEVEL_11_0);
		if (Failed(res))
			return ResultError(res.Code, "GfxSystemD3D12: failed to initialize D3D device objects");

		// initialize descriptor heaps
		for (ur_int heapType = 0; heapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++heapType)
		{
			std::unique_ptr<DescriptorHeap> newHeap(new DescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE(heapType)));
			this->descriptorHeaps.push_back(std::move(newHeap));
		}

		// initialize frame(s) data
		const ur_uint defaultFramesCount = 1;
		res = InitializeFrameData(defaultFramesCount);
		if (Failed(res))
			return ResultError(Failure, "GfxSystemD3D12: failed to initialize frame data");

		// initialize resource context
		this->resourceContext.reset(new GfxContextD3D12(*this));
		this->resourceContext->Initialize();

		return Result(Success);
	}

	Result GfxSystemD3D12::AddCommandList(shared_ref<ID3D12CommandList> &d3dCommandList)
	{
		std::lock_guard<std::mutex> lockCommandLists(this->commandListsMutex);

		this->d3dCommandLists.push_back(d3dCommandList);

		return Result(Success);
	}

	Result GfxSystemD3D12::Render()
	{
		// execute command lists

		std::lock_guard<std::mutex> lockCommandLists(this->commandListsMutex);
		
		std::vector<ID3D12CommandList*> executionList;
		executionList.reserve(this->d3dCommandLists.size());
		for (auto &d3dCommandList : this->d3dCommandLists)
		{
			executionList.push_back(d3dCommandList.get());
		}
		this->d3dCommandQueue->ExecuteCommandLists((UINT)executionList.size(), executionList.data());
		
		this->d3dCommandLists.clear();

		return Result(Success);
	}

	Result GfxSystemD3D12::CreateContext(std::unique_ptr<GfxContext> &gfxContext)
	{
		gfxContext.reset(new GfxContextD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture)
	{
		gfxTexture.reset(new GfxTextureD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT)
	{
		gfxRT.reset(new GfxRenderTargetD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain)
	{
		gfxSwapChain.reset(new GfxSwapChainD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer)
	{
		gfxBuffer.reset(new GfxBufferD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateResourceBinding(std::unique_ptr<GfxResourceBinding> &gfxBinding)
	{
		gfxBinding.reset(new GfxResourceBindingD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreatePipelineStateObject(std::unique_ptr<GfxPipelineStateObject> &gfxPipelineState)
	{
		gfxPipelineState.reset(new GfxPipelineStateObjectD3D12(*this));
		return Result(Success);
	}

	GfxSystemD3D12::DescriptorHeap::DescriptorHeap(GfxSystemD3D12& gfxSystem, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType) :
		GfxEntity(gfxSystem),
		d3dHeapType(d3dHeapType)
	{
		this->d3dDescriptorSize = 0;
		this->firstFreePageIdx = 0;
	}
	
	GfxSystemD3D12::DescriptorHeap::~DescriptorHeap()
	{
		// detach extrenal handles
		for (auto& descriptorHandle : this->descriptorSets)
		{
			descriptorHandle->heap = ur_null;
		}
	}

	Result GfxSystemD3D12::DescriptorHeap::AcquireDescriptor(std::unique_ptr<Descriptor>& descriptor)
	{
		std::unique_ptr<DescriptorSet> descriptorSet(new Descriptor());
		Result res = this->AcquireDescriptors(*descriptorSet, 1);
		descriptor.reset(static_cast<Descriptor*>(descriptorSet.release()));
		return res;
	}

	Result GfxSystemD3D12::DescriptorHeap::AcquireDescriptors(std::unique_ptr<DescriptorSet>& descriptorSet, ur_size descriptorsCount)
	{
		descriptorSet.reset(new DescriptorSet());
		return this->AcquireDescriptors(*descriptorSet, 1);
	}

	Result GfxSystemD3D12::DescriptorHeap::AcquireDescriptors(DescriptorSet& descriptorSet, ur_size descriptorsCount)
	{
		if (0 == descriptorsCount || descriptorsCount > DescriptorsPerHeap)
			return Result(InvalidArgs);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxSystemD3D12::DescriptorHeap::AcquireDescriptor: failed, device unavailable");

		std::lock_guard<std::mutex> lockModify(this->modifyMutex);

		ur_size currentPageIdx = ur_size(-1);
		ur_size currentRangeIdx = ur_size(-1);
		if (this->firstFreePageIdx < this->pagePool.size())
		{
			for (ur_size pageIdx = this->firstFreePageIdx; pageIdx < this->pagePool.size(); ++pageIdx)
			{
				auto& pageRanges = this->pagePool[pageIdx]->freeRanges;
				if (!pageRanges.empty())
				{
					// try finding a free range of sufficient size in current page
					for (ur_size rangeIdx = 0; rangeIdx < pageRanges.size(); ++rangeIdx)
					{
						if (pageRanges[rangeIdx].size() >= descriptorsCount)
						{
							currentPageIdx = pageIdx;
							currentRangeIdx = rangeIdx;
							break; // found
						}
					}
				}
				if (currentRangeIdx != ur_size(-1))
					break;
			}
			if (currentRangeIdx == ur_size(-1))
			{
				// new page is required
				this->firstFreePageIdx = this->pagePool.size();
			}
		}

		if (this->firstFreePageIdx >= this->pagePool.size())
		{
			// create new D3D heap
			
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
			heapDesc.Type = this->d3dHeapType;
			heapDesc.NumDescriptors = (UINT)DescriptorsPerHeap;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			switch (this->d3dHeapType)
			{
			case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				break;
			}
			heapDesc.NodeMask = 0;

			shared_ref<ID3D12DescriptorHeap> d3dHeap;
			HRESULT hr = d3dDevice->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), d3dHeap);
			if (FAILED(hr))
				return ResultError(NotInitialized, "GfxSystemD3D12::DescriptorHeap::AcquireDescriptor: failed to create ID3D12DescriptorHeap");
			
			std::unique_ptr<Page> newPage(new Page());
			newPage->d3dHeap = d3dHeap;
			newPage->freeRanges.push_back(Range(0, DescriptorsPerHeap - 1));

			this->pagePool.push_back(std::move(newPage));
			this->firstFreePageIdx = this->pagePool.size() - 1;
			currentPageIdx = this->firstFreePageIdx;
			currentRangeIdx = 0;
			
			if (0 == this->d3dDescriptorSize)
			{
				this->d3dDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(this->d3dHeapType);
			}
		}

		// grab some page memory
		Page& currentPage = *this->pagePool[currentPageIdx].get();
		Range& currentRange = currentPage.freeRanges[currentRangeIdx];
		ur_size currentOffset = currentRange.start;
		currentRange.start += descriptorsCount; // decrease range
		assert(currentRange.start <= currentRange.end); // must never be reached
		if (currentRange.start == currentRange.end)
		{
			// release exhausted range
			currentPage.freeRanges.erase(currentPage.freeRanges.begin() + currentRangeIdx);
		}
		if (currentPage.freeRanges.empty())
		{
			// find next page with free range(s) available
			do
			{
				++this->firstFreePageIdx;
			} while (this->firstFreePageIdx < this->pagePool.size() && !this->pagePool[this->firstFreePageIdx]->freeRanges.empty());
		}

		// initialize descriptor
		ID3D12DescriptorHeap* currentHeap = currentPage.d3dHeap;
		SIZE_T currentHeapCPUStart = currentHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		UINT64 currentHeapGPUStart = currentHeap->GetGPUDescriptorHandleForHeapStart().ptr;
		descriptorSet.descriptorCount = descriptorsCount;
		descriptorSet.firstCpuHandle.ptr = currentHeapCPUStart + currentOffset * this->d3dDescriptorSize;
		descriptorSet.firstGpuHandle.ptr = currentHeapGPUStart + currentOffset * this->d3dDescriptorSize;
		descriptorSet.heap = this;
		descriptorSet.pagePoolPos = currentOffset + currentPageIdx * DescriptorsPerHeap;
		descriptorSet.descriptorListPos = this->descriptorSets.size();
		this->descriptorSets.push_back(&descriptorSet);

		return Result(Success);
	}

	Result GfxSystemD3D12::DescriptorHeap::ReleaseDescriptor(Descriptor& descriptor)
	{
		return this->ReleaseDescriptors(static_cast<DescriptorSet&>(descriptor));
	}

	Result GfxSystemD3D12::DescriptorHeap::ReleaseDescriptors(DescriptorSet& descriptorSet)
	{
		std::lock_guard<std::mutex> lockModify(this->modifyMutex);

		// remove from descriptors registry
		if (descriptorSet.descriptorListPos + 1 < this->descriptorSets.size())
		{
			// replace with the last descriptor
			ur_size replacePos = descriptorSet.descriptorListPos;
			this->descriptorSets[replacePos] = this->descriptorSets.back();
			this->descriptorSets[replacePos]->descriptorListPos = replacePos;
		}
		this->descriptorSets.pop_back();
		
		ur_size pageIdx = descriptorSet.pagePoolPos / DescriptorsPerHeap;
		ur_size releaseStart = descriptorSet.pagePoolPos % DescriptorsPerHeap;
		ur_size releaseEnd = releaseStart + descriptorSet.descriptorCount - 1;
		Page& page = *this->pagePool[pageIdx].get();
		
		// rearrange free ranges: try adding released item to exiting range, merge ranges or create new range
		for (ur_size ir = 0; ir < page.freeRanges.size(); ++ir)
		{
			Range& range = page.freeRanges[ir];
			if (releaseEnd + 1 < range.start)
			{
				page.freeRanges.insert(page.freeRanges.begin() + ir, Range(releaseStart, releaseEnd));
				break;
			}
			if (releaseEnd + 1 == range.start)
			{
				range.start = releaseStart;
				Range* prevRange = (ir > 0 ? &page.freeRanges[ir - 1] : ur_null);
				if (prevRange && prevRange->end + 1 == releaseStart)
				{
					range.start = prevRange->start;
					page.freeRanges.erase(page.freeRanges.begin() + ir - 1);
				}
				break;
			}
			if (range.end + 1 == releaseStart)
			{
				range.end = releaseEnd;
				Range* nextRange = (ir + 1 < page.freeRanges.size() ? &page.freeRanges[ir + 1] : ur_null);
				if (nextRange && nextRange->start == releaseEnd + 1)
				{
					range.end = nextRange->end;
					page.freeRanges.erase(page.freeRanges.begin() + ir + 1);
				}
				break;
			}
			if (ir + 1 == page.freeRanges.size())
			{
				page.freeRanges.insert(page.freeRanges.begin() + ir, Range(releaseStart, releaseEnd));
				break;
			}
		}

		// note: do not remove empty page to preserve ordering and indexing (following pages can still be referenced by other descriptor(s));
		// however, d3d heap memory can be released & reacquired on demand
		if (page.freeRanges.front().end - page.freeRanges.front().start + 1 == DescriptorsPerHeap)
		{
			// consider releasing d3d heap here
		}
		this->firstFreePageIdx = std::min(pageIdx, this->firstFreePageIdx);

		return Result(Success);
	}

	GfxSystemD3D12::DescriptorSet::DescriptorSet()
	{
		this->descriptorCount = 0;
		this->firstCpuHandle.ptr = 0;
		this->firstGpuHandle.ptr = 0;
		this->heap = ur_null;
		this->pagePoolPos = ur_size(-1);
		this->descriptorListPos = ur_size(-1);
	}

	GfxSystemD3D12::DescriptorSet::~DescriptorSet()
	{
		if (this->heap != ur_null)
		{
			this->heap->ReleaseDescriptors(*this);
		}
	}

	GfxSystemD3D12::Descriptor::Descriptor()
	{
	}

	GfxSystemD3D12::Descriptor::~Descriptor()
	{
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxContextD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxContextD3D12::GfxContextD3D12(GfxSystem &gfxSystem) :
		GfxContext(gfxSystem)
	{
	}

	GfxContextD3D12::~GfxContextD3D12()
	{
	}

	Result GfxContextD3D12::Initialize()
	{
		this->d3dCommandList.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxContextD3D12: failed to initialize, device unavailable");

		UINT nodeMask = 0;
		HRESULT hr = d3dDevice->CreateCommandList(nodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dSystem.GetD3DCommandAllocator(), ur_null,
			__uuidof(ID3D12GraphicsCommandList), this->d3dCommandList);
		if (FAILED(hr))
			return ResultError(Failure, "GfxContextD3D12: failed to create command list");

		// start in non-recording state
		this->d3dCommandList->Close();

		return Result(Success);
	}

	Result GfxContextD3D12::Begin()
	{
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		
		this->d3dCommandList->Reset(d3dSystem.GetD3DCommandAllocator(), ur_null);
		
		return Result(Success);
	}

	Result GfxContextD3D12::End()
	{
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());

		this->d3dCommandList->Close();
		d3dSystem.AddCommandList(shared_ref<ID3D12CommandList>(this->d3dCommandList.get()));

		return Result(Success);
	}

	Result GfxContextD3D12::SetRenderTarget(GfxRenderTarget *rt)
	{
		return this->SetRenderTarget(rt, rt);
	}

	Result GfxContextD3D12::SetRenderTarget(GfxRenderTarget *rt, GfxRenderTarget *ds)
	{
		// todo: support DS

		if (this->d3dCommandList.empty())
			return ResultError(Failure, "GfxContextD3D12::SetRenderTarget: failed, d3d command list is not initialized");

		GfxRenderTargetD3D12* rtD3D12 = static_cast<GfxRenderTargetD3D12*>(rt);
		UINT numRTs = (rtD3D12 != ur_null && rtD3D12->GetRTVDescriptor() != ur_null ? 1 : 0);
		const D3D12_CPU_DESCRIPTOR_HANDLE d3dRTDescriptors[] = { numRTs > 0 ? rtD3D12->GetRTVDescriptor()->CpuHandle() : D3D12_CPU_DESCRIPTOR_HANDLE() };
		this->d3dCommandList->OMSetRenderTargets(numRTs, d3dRTDescriptors, FALSE, ur_null);

		if (rt != ur_null)
		{
			GfxTexture *rtTex = rt->GetTargetBuffer();
			if (ur_null == rtTex && ds != ur_null) rtTex = ds->GetDepthStencilBuffer();
			if (rtTex != ur_null)
			{
				GfxViewPort gfxViewPort = {
					0.0f, 0.0f, (ur_float)rtTex->GetDesc().Width, (ur_float)rtTex->GetDesc().Height, 0.0f, 1.0f
				};
				this->SetViewPort(&gfxViewPort);
			}
		}

		return Result(Success);
	}

	Result GfxContextD3D12::SetViewPort(const GfxViewPort *viewPort)
	{
		if (this->d3dCommandList.empty())
			return ResultError(Failure, "GfxContextD3D12::SetViewPort: failed, d3d command list is not initialized");

		this->d3dCommandList->RSSetViewports(1, viewPort != ur_null ?
			&GfxViewPortToD3D12(*viewPort) : ur_null);

		//  temp: set scissor rect right here
		if (viewPort != ur_null)
		{
			D3D12_RECT d3dRect = { LONG(viewPort->X), LONG(viewPort->Y), LONG(viewPort->X + viewPort->Width), LONG(viewPort->Y + viewPort->Height) };
			this->d3dCommandList->RSSetScissorRects(1, &d3dRect);
		}

		return Result(Success);
	}

	Result GfxContextD3D12::GetViewPort(GfxViewPort &viewPort)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetScissorRect(const RectI *rect)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::ClearTarget(GfxRenderTarget *rt,
		bool clearColor, const ur_float4 &color,
		bool clearDepth, const ur_float &depth,
		bool clearStencil, const ur_uint &stencil)
	{
		if (ur_null == rt)
			return Result(InvalidArgs);

		GfxRenderTargetD3D12 *rt_d3d12 = static_cast<GfxRenderTargetD3D12*>(rt);

		if (clearColor)
		{
			this->d3dCommandList->ClearRenderTargetView(rt_d3d12->GetRTVDescriptor()->CpuHandle(), &color.x, 0, ur_null);
		}

		// todo: clear depth & stencil

		return Result(Success);
	}

	Result GfxContextD3D12::SetPipelineStateObject(GfxPipelineStateObject *state)
	{
		if (ur_null == state)
			return Result(InvalidArgs);

		if (this->d3dCommandList.empty())
			return ResultError(Failure, "GfxContextD3D12::SetPipelineStateObject: failed, d3d command list is not initialized");

		GfxPipelineStateObjectD3D12 *pipelineStateD3D12 = static_cast<GfxPipelineStateObjectD3D12*>(state);
		this->d3dCommandList->SetPipelineState(pipelineStateD3D12->GetD3DPipelineState());
		this->d3dCommandList->IASetPrimitiveTopology(pipelineStateD3D12->GetD3DPrimitiveTopology());

		return Result(Success);
	}

	Result GfxContextD3D12::SetResourceBinding(GfxResourceBinding *binding)
	{
		if (this->d3dCommandList.empty())
			return ResultError(Failure, "GfxContextD3D12::SetResourceBinding: failed, d3d command list is not initialized");

		GfxResourceBindingD3D12 *resourceBindingD3D12 = static_cast<GfxResourceBindingD3D12*>(binding);
		this->d3dCommandList->SetGraphicsRootSignature(resourceBindingD3D12->GetD3DRootSignature());

		// todo: descriptor tables & corresponding heaps
		//this->d3dCommandList->SetDescriptorHeaps()
		//this->d3dCommandList->SetGraphicsRootDescriptorTable(0, )

		return Result(Success);
	}

	Result GfxContextD3D12::SetVertexBuffer(GfxBuffer *buffer, ur_uint slot, ur_uint stride, ur_uint offset)
	{
		if (this->d3dCommandList.empty())
			return ResultError(Failure, "GfxContextD3D12::SetVertexBuffer: failed, d3d command list is not initialized");

		GfxBufferD3D12* bufferD3D12 = static_cast<GfxBufferD3D12*>(buffer);
		if (bufferD3D12 != ur_null && bufferD3D12->GetResource().GetD3DResource() != ur_null)
		{
			D3D12_VERTEX_BUFFER_VIEW d3dVBV;
			d3dVBV.BufferLocation = bufferD3D12->GetResource().GetD3DResource()->GetGPUVirtualAddress();
			d3dVBV.StrideInBytes = stride;
			d3dVBV.SizeInBytes = buffer->GetDesc().Size;
			this->d3dCommandList->IASetVertexBuffers(slot, 1, &d3dVBV);
		}
		else
		{
			this->d3dCommandList->IASetVertexBuffers(slot, 0, ur_null);
		}

		return Result(Success);
	}

	Result GfxContextD3D12::SetIndexBuffer(GfxBuffer *buffer, ur_uint bitsPerIndex, ur_uint offset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		if (this->d3dCommandList.empty())
			return ResultError(Failure, "GfxContextD3D12::Draw: failed, d3d command list is not initialized");

		this->d3dCommandList->DrawInstanced((UINT)vertexCount, (UINT)instanceCount, (UINT)vertexOffset, (UINT)instanceOffset);

		return Result(Success);
	}

	Result GfxContextD3D12::DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::UpdateResource(GfxResourceD3D12* dstResource, GfxResourceD3D12* srcResource)
	{
		if (ur_null == this->d3dCommandList.get())
			return Result(NotInitialized);

		if (ur_null == dstResource || ur_null == srcResource)
			return Result(InvalidArgs);

		this->d3dCommandList->CopyResource(dstResource->GetD3DResource(), srcResource->GetD3DResource());
		
		return Result(Success);
	}

	Result GfxContextD3D12::ResourceTransition(GfxResourceD3D12 *resource, D3D12_RESOURCE_STATES newState)
	{
		if (ur_null == resource || ur_null == resource->GetD3DResource())
			return Result(InvalidArgs);

		if (resource->d3dCurrentState != newState)
		{
			ID3D12Resource *d3dResource = resource->GetD3DResource();
			D3D12_RESOURCE_BARRIER d3dResBarrier = {};
			d3dResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			d3dResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dResBarrier.Transition.pResource = d3dResource;
			d3dResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			d3dResBarrier.Transition.StateBefore = resource->d3dCurrentState;
			d3dResBarrier.Transition.StateAfter = newState;
			this->d3dCommandList->ResourceBarrier(1, &d3dResBarrier);

			resource->d3dCurrentState = newState;
		}
		
		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxResourceD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxResourceD3D12::GfxResourceD3D12(GfxSystem &gfxSystem) :
		GfxEntity(gfxSystem)
	{
		this->d3dCurrentState = D3D12_RESOURCE_STATES(-1);
	}

	GfxResourceD3D12::~GfxResourceD3D12()
	{
	}

	Result GfxResourceD3D12::Initialize(ID3D12Resource *d3dResource, D3D12_RESOURCE_STATES initialState)
	{
		this->d3dResource.reset(d3dResource);
		this->d3dCurrentState = initialState;
		return Result(Success);
	}

	Result GfxResourceD3D12::Initialize(GfxResourceD3D12& resource)
	{
		this->d3dResource.reset(resource.GetD3DResource());
		this->d3dCurrentState = resource.GetD3DResourceState();
		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxTextureD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxTextureD3D12::GfxTextureD3D12(GfxSystem &gfxSystem) :
		GfxTexture(gfxSystem),
		initializedFromD3DRes(false),
		resource(gfxSystem)
	{
	}

	GfxTextureD3D12::~GfxTextureD3D12()
	{
	}

	Result GfxTextureD3D12::Initialize(const GfxTextureDesc &desc, GfxResourceD3D12 &resource)
	{
		this->initializedFromD3DRes = true; // setting this flag to skip OnInitialize
		GfxTexture::Initialize(desc, ur_null); // store desc in the base class, 
		this->initializedFromD3DRes = false;

		this->resource.Initialize(ur_null);
		this->srvDescriptor.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxTextureD3D12: failed to initialize, device unavailable");

		this->resource.Initialize(resource);

		if (desc.BindFlags & ur_uint(GfxBindFlag::ShaderResource))
		{
			Result res = d3dSystem.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->AcquireDescriptor(this->srvDescriptor);
			if (Failed(res))
				return ResultError(NotInitialized, "GfxTextureD3D12::Initialize: failed to acquire SRV descriptor");

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = GfxFormatToDXGI(this->GetDesc().Format, this->GetDesc().FormatView);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = -1;
			srvDesc.Texture2D.MostDetailedMip = 0;
		
			d3dDevice->CreateShaderResourceView(this->resource.GetD3DResource(), &srvDesc, this->srvDescriptor->CpuHandle());
		}

		return Result(Success);
	}

	Result GfxTextureD3D12::OnInitialize(const GfxResourceData *data)
	{
		// if texture is being initialized from d3d resource - all the stuff is done in GfxTextureD3D12::Initialize
		// so skip this function as its results will be overriden anyway
		if (this->initializedFromD3DRes)
			return Result(Success);

		this->resource.Initialize(ur_null);
		this->srvDescriptor.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxTextureD3D12::OnInitialize: failed, device unavailable");

		// todo: init sub resource data

		D3D12_RESOURCE_DESC d3dResDesc = GfxTextureDescToD3D12ResDesc(this->GetDesc());
		d3dResDesc.Format = GfxFormatToDXGI(this->GetDesc().Format, GfxFormatView::Typeless);

		D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
		d3dHeapProperties.Type = GfxUsageToD3D12HeapType(this->GetDesc().Usage);
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		D3D12_RESOURCE_STATES d3dResStates = GfxBindFlagsAndUsageToD3D12ResState(this->GetDesc().BindFlags, this->GetDesc().Usage);
		
		shared_ref<ID3D12Resource> d3dResource;
		HRESULT hr = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dResStates, ur_null,
			__uuidof(ID3D12Resource), d3dResource);
		if (FAILED(hr))
			return ResultError(Failure, "GfxTextureD3D12::OnInitialize: failed at CreateCommittedResource");

		Result res = this->resource.Initialize(d3dResource, d3dResStates);
		if (Failed(res))
			return ResultError(Failure, "GfxTextureD3D12::OnInitialize: failed to initialize GfxResourceD3D12");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxRenderTargetD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxRenderTargetD3D12::GfxRenderTargetD3D12(GfxSystem &gfxSystem) :
		GfxRenderTarget(gfxSystem)
	{
	}

	GfxRenderTargetD3D12::~GfxRenderTargetD3D12()
	{
	}

	Result GfxRenderTargetD3D12::OnInitialize()
	{
		this->rtvDescriptor.reset(ur_null);
		this->dsvDescriptor.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxRenderTargetD3D12: failed to initialize, device unavailable");

		GfxTextureD3D12 *d3dTargetBuffer = static_cast<GfxTextureD3D12*>(this->GetTargetBuffer());
		if (d3dTargetBuffer != ur_null)
		{
			Result res = d3dSystem.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->AcquireDescriptor(this->rtvDescriptor);
			if (Failed(res))
				return ResultError(NotInitialized, "GfxRenderTargetD3D12::OnInitialize: failed to acquire RTV descriptor");

			const GfxTextureDesc &bufferDesc = this->GetTargetBuffer()->GetDesc();
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = GfxFormatToDXGI(bufferDesc.Format, bufferDesc.FormatView);
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
			
			d3dDevice->CreateRenderTargetView(d3dTargetBuffer->GetResource().GetD3DResource(), &rtvDesc, this->rtvDescriptor->CpuHandle());
		}

		GfxTextureD3D12 *d3dDepthStencilBuffer = static_cast<GfxTextureD3D12*>(this->GetDepthStencilBuffer());
		if (d3dDepthStencilBuffer != ur_null)
		{
			Result res = d3dSystem.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->AcquireDescriptor(this->dsvDescriptor);
			if (Failed(res))
				return ResultError(NotInitialized, "GfxRenderTargetD3D12::OnInitialize: failed to acquire DSV descriptor");

			const GfxTextureDesc &bufferDesc = this->GetDepthStencilBuffer()->GetDesc();
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			switch (bufferDesc.Format)
			{
			case GfxFormat::R32G8X24:
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				break;
			case GfxFormat::R32:
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				break;
			case GfxFormat::R24G8:
				dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			case GfxFormat::R16:
				dsvDesc.Format = DXGI_FORMAT_D16_UNORM;
				break;
			default:
				dsvDesc.Format = DXGI_FORMAT_UNKNOWN;
			}
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.Texture2D.MipSlice = 0;

			d3dDevice->CreateDepthStencilView(d3dDepthStencilBuffer->GetResource().GetD3DResource(), &dsvDesc, this->dsvDescriptor->CpuHandle());
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSwapChainD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSwapChainD3D12::GfxSwapChainD3D12(GfxSystem &gfxSystem) :
		GfxSwapChain(gfxSystem),
		gfxContext(gfxSystem)
	{
		memset(&this->dxgiChainDesc, 0, sizeof(this->dxgiChainDesc));
		this->backBufferIndex = 0;
	}

	GfxSwapChainD3D12::~GfxSwapChainD3D12()
	{
		// ensure command lists possibly referencing swap chain resources are done before releasing
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		d3dSystem.WaitGPU();
	}

	Result GfxSwapChainD3D12::Initialize(const GfxPresentParams &params)
	{
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());

		// ensure back buffers are no longer used by GPU
		d3dSystem.WaitGPU();

		this->backBuffers.clear();
		this->dxgiSwapChain.reset(ur_null);

		IDXGIFactory4 *dxgiFactory = d3dSystem.GetDXGIFactory();
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		ID3D12CommandQueue *d3dCommandQueue = d3dSystem.GetD3DCommandQueue();
		if (ur_null == d3dSystem.GetWinCanvas())
			return ResultError(NotInitialized, "GfxSwapChainD3D12::Initialize: failed, canvas not initialized");

		this->dxgiChainDesc.Width = params.BufferWidth;
		this->dxgiChainDesc.Height = params.BufferHeight;
		this->dxgiChainDesc.Format = GfxFormatToDXGI(params.BufferFormat, GfxFormatView::Unorm);
		this->dxgiChainDesc.Stereo = false;
		this->dxgiChainDesc.SampleDesc.Count = params.MutisampleCount;
		this->dxgiChainDesc.SampleDesc.Quality = params.MutisampleQuality;
		this->dxgiChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		this->dxgiChainDesc.BufferCount = params.BufferCount;
		this->dxgiChainDesc.Scaling = DXGI_SCALING_STRETCH;
		this->dxgiChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		this->dxgiChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		this->dxgiChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		shared_ref<IDXGISwapChain1> dxgiSwapChain1;
		HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(d3dCommandQueue, d3dSystem.GetWinCanvas()->GetHwnd(),
			&this->dxgiChainDesc, nullptr, nullptr, dxgiSwapChain1);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSwapChainD3D12::Initialize: failed to create DXGI swap chain");

		dxgiSwapChain1->QueryInterface(__uuidof(IDXGISwapChain3), this->dxgiSwapChain);

		GfxTextureDesc desc;
		desc.Width = params.BufferWidth;
		desc.Height = params.BufferHeight;
		desc.Levels = 1;
		desc.Format = params.BufferFormat;
		desc.FormatView = GfxFormatView::Unorm;
		desc.Usage = GfxUsage::Default;
		desc.BindFlags = ur_uint(GfxBindFlag::RenderTarget);
		desc.AccessFlags = ur_uint(0);

		std::unique_ptr<BackBuffer> dummy;
		this->backBuffers.reserve(params.BufferCount);
		for (ur_uint ibuffer = 0; ibuffer < params.BufferCount; ++ibuffer)
		{
			shared_ref<ID3D12Resource> d3dResource;
			hr = this->dxgiSwapChain->GetBuffer(ibuffer, __uuidof(ID3D12Resource), d3dResource);
			if (FAILED(hr))
				return ResultError(Failure, "GfxSwapChainD3D12::Initialize: failed to retrieve back buffer resource");

			std::unique_ptr<GfxSwapChainD3D12::BackBuffer> backBuffer(new BackBuffer(d3dSystem, d3dResource));
			Result res = backBuffer->Initialize(desc, params.DepthStencilEnabled, params.DepthStencilFormat);
			if (Failed(res))
				return ResultError(res.Code, "GfxSwapChainD3D12::Initialize: failed to initialize back buffer");

			this->backBuffers.push_back(std::move(backBuffer));
		}

		this->backBufferIndex = (ur_uint)this->dxgiSwapChain->GetCurrentBackBufferIndex();

		// request GfxSystemD3D12 to initialize per frame data corresponding to the number of back buffers
		d3dSystem.InitializeFrameData(params.BufferCount);
		d3dSystem.SetFrame(this->backBufferIndex);

		// initialize context to execute resource transitions
		this->gfxContext.Initialize();

		// set current frame buffer to render target state
		GfxRenderTargetD3D12 *crntFrameRT = static_cast<GfxRenderTargetD3D12*>(this->GetTargetBuffer());
		GfxResourceD3D12 &crntFrameResource = static_cast<GfxTextureD3D12*>(crntFrameRT->GetTargetBuffer())->GetResource();
		this->gfxContext.Begin();
		this->gfxContext.ResourceTransition(&crntFrameResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
		this->gfxContext.End();
		this->GetGfxSystem().Render();

		return Result(Success);
	}

	Result GfxSwapChainD3D12::Present()
	{
		if (this->dxgiSwapChain.empty())
			return ResultError(NotInitialized, "GfxSwapChainD3D12::Present: DXGI swap chain not initialized");

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();

		// finalize current frame
		// transition barrier: render target -> present
		GfxRenderTargetD3D12 *currentFrameRT = static_cast<GfxRenderTargetD3D12*>(this->GetTargetBuffer());
		GfxResourceD3D12 &currentFrameBuffer = static_cast<GfxTextureD3D12*>(currentFrameRT->GetTargetBuffer())->GetResource();
		this->gfxContext.Begin();
		this->gfxContext.ResourceTransition(&currentFrameBuffer, D3D12_RESOURCE_STATE_PRESENT);
		this->gfxContext.End();
		d3dSystem.Render();

		// present
		if (FAILED(this->dxgiSwapChain->Present(1, 0)))
			return ResultError(Failure, "GfxSwapChainD3D12::Present: failed");

		// move to next frame
		this->backBufferIndex = (ur_uint)this->dxgiSwapChain->GetCurrentBackBufferIndex();
		d3dSystem.SetFrame(this->backBufferIndex);

		// transition barrier: present -> render target
		GfxRenderTargetD3D12 *newFrameRT = static_cast<GfxRenderTargetD3D12*>(this->GetTargetBuffer());
		GfxResourceD3D12 &newFrameBuffer = static_cast<GfxTextureD3D12*>(newFrameRT->GetTargetBuffer())->GetResource();
		this->gfxContext.Begin();
		this->gfxContext.ResourceTransition(&newFrameBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		this->gfxContext.End();

		return Result(Success);
	}

	GfxRenderTarget* GfxSwapChainD3D12::GetTargetBuffer()
	{
		if (this->backBufferIndex >= (ur_uint)this->backBuffers.size())
			return ur_null;

		return this->backBuffers[this->backBufferIndex].get();
	}

	GfxSwapChainD3D12::BackBuffer::BackBuffer(GfxSystem &gfxSystem, shared_ref<ID3D12Resource> d3dSwapChainResource) :
		GfxRenderTargetD3D12(gfxSystem),
		dxgiSwapChainBuffer(gfxSystem)
	{
		this->dxgiSwapChainBuffer.Initialize(d3dSwapChainResource.get(), D3D12_RESOURCE_STATE_COMMON);
	}

	GfxSwapChainD3D12::BackBuffer::~BackBuffer()
	{
	}

	Result GfxSwapChainD3D12::BackBuffer::InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc)
	{
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());

		std::unique_ptr<GfxTextureD3D12> newTargetBuffer(new GfxTextureD3D12(d3dSystem));
		Result res = newTargetBuffer->Initialize(desc, this->dxgiSwapChainBuffer);
		if (Failed(res))
			return ResultError(res.Code, "GfxSwapChainD3D12::BackBuffer::InitializeTargetBuffer: failed");

		resultBuffer = std::move(newTargetBuffer);

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxBufferD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxBufferD3D12::GfxBufferD3D12(GfxSystem &gfxSystem) :
		GfxBuffer(gfxSystem),
		resource(gfxSystem)
	{
	}

	GfxBufferD3D12::~GfxBufferD3D12()
	{
	}

	Result GfxBufferD3D12::OnInitialize(const GfxResourceData *data)
	{
		this->resource.Initialize(ur_null);
		this->uploadResource.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxBufferD3D12::OnInitialize: failed, device unavailable");

		D3D12_RESOURCE_DESC d3dResDesc = {};
		d3dResDesc = GfxBufferDescToD3D12ResDesc(this->GetDesc());

		D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
		d3dHeapProperties.Type = GfxUsageToD3D12HeapType(this->GetDesc().Usage);
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		D3D12_RESOURCE_STATES d3dResStates = GfxBindFlagsAndUsageToD3D12ResState(this->GetDesc().BindFlags, this->GetDesc().Usage);

		shared_ref<ID3D12Resource> d3dResource;
		HRESULT hr = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dResStates, ur_null,
			__uuidof(ID3D12Resource), d3dResource);
		if (FAILED(hr))
			return ResultError(Failure, "GfxBufferD3D12::OnInitialize: failed at CreateCommittedResource");

		Result res = this->resource.Initialize(d3dResource, d3dResStates);
		if (Failed(res))
			return ResultError(Failure, "GfxBufferD3D12::OnInitialize: failed to initialize GfxResourceD3D12");

		if (data != ur_null)
		{
			D3D12_SUBRESOURCE_DATA d3dSubresData = GfxResourceDataToD3D12(*data);

			if (D3D12_HEAP_TYPE_UPLOAD == d3dHeapProperties.Type)
			{
				// if current resource is in Upload heap - fill it with initial data

				hr = FillUploadResource(this->resource.GetD3DResource(), 0, 1, &d3dSubresData);
				if (FAILED(hr))
					return ResultError(Failure, "GfxBufferD3D12::OnInitialize: failed to fill upload resource");
			}
			else
			{
				// create & fill upload resource

				shared_ref<ID3D12Resource> d3dUploadResource;
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
				D3D12_RESOURCE_STATES d3dUploadResState = D3D12_RESOURCE_STATE_GENERIC_READ;
				hr = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dUploadResState, ur_null,
					__uuidof(ID3D12Resource), d3dUploadResource);
				if (FAILED(hr))
					return ResultError(Failure, "GfxBufferD3D12::OnInitialize: failed to create upload resource");
				this->uploadResource.reset(new GfxResourceD3D12(this->GetGfxSystem()));
				this->uploadResource->Initialize(d3dUploadResource, d3dUploadResState);

				hr = FillUploadResource(this->uploadResource->GetD3DResource(), 0, 1, &d3dSubresData);
				if (FAILED(hr))
					return ResultError(Failure, "GfxBufferD3D12::OnInitialize: failed to fill upload resource");

				// schedule a copy to destination resource

				d3dSystem.GetResourceContext()->Begin();
				d3dSystem.GetResourceContext()->ResourceTransition(&this->resource, D3D12_RESOURCE_STATE_COPY_DEST);
				d3dSystem.GetResourceContext()->UpdateResource(&this->resource, this->uploadResource.get());
				d3dSystem.GetResourceContext()->ResourceTransition(&this->resource, d3dResStates);
				d3dSystem.GetResourceContext()->End();
			}
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPipelineStateObjectD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxPipelineStateObjectD3D12::GfxPipelineStateObjectD3D12(GfxSystem &gfxSystem) :
		GfxPipelineStateObject(gfxSystem)
	{
		this->d3dPipelineDesc = {};
		this->d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		this->d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		this->d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		this->d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		this->d3dPipelineDesc.NumRenderTargets = 1;
		this->d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		this->d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		this->d3dPipelineDesc.SampleDesc.Count = 1;
		this->d3dPipelineDesc.SampleDesc.Quality = 0;

		this->d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	GfxPipelineStateObjectD3D12::~GfxPipelineStateObjectD3D12()
	{

	}

	Result GfxPipelineStateObjectD3D12::OnInitialize(const StateFlags& changedStates)
	{
		StateFlags updateFlags = (this->d3dPipelineState.empty() ? ~StateFlags(0) : changedStates);
		if (updateFlags == 0)
			return Result(Success);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxPipelineStateObjectD3D12::OnInitialize: failed, device unavailable");

		if (BindingStateFlag & updateFlags)
		{
			if (this->GetBinding() != ur_null)
			{
				const GfxResourceBindingD3D12* bindingD3D12 = static_cast<const GfxResourceBindingD3D12*>(this->GetBinding());
				this->d3dPipelineDesc.pRootSignature = bindingD3D12->GetD3DRootSignature();
			}
			else
			{
				this->d3dPipelineDesc.pRootSignature = ur_null;
			}
		}
		if (VertexShaderFlag & updateFlags)
		{
			if (this->GetVertexShader() != ur_null)
			{
				this->d3dPipelineDesc.VS.pShaderBytecode = (void*)this->GetVertexShader()->GetByteCode();
				this->d3dPipelineDesc.VS.BytecodeLength = (SIZE_T)this->GetVertexShader()->GetSizeInBytes();
			}
			else
			{
				this->d3dPipelineDesc.VS.pShaderBytecode = ur_null;
				this->d3dPipelineDesc.VS.BytecodeLength = 0;
			}
		}
		if (PixelShaderFlag & updateFlags)
		{
			if (this->GetPixelShader() != ur_null)
			{
				this->d3dPipelineDesc.PS.pShaderBytecode = (void*)this->GetPixelShader()->GetByteCode();
				this->d3dPipelineDesc.PS.BytecodeLength = (SIZE_T)this->GetPixelShader()->GetSizeInBytes();
			}
			else
			{
				this->d3dPipelineDesc.PS.pShaderBytecode = ur_null;
				this->d3dPipelineDesc.PS.BytecodeLength = 0;
			}
		}
		if (BlendStateFlag & updateFlags)
		{
			for (ur_uint irt = 0; irt < MaxRenderTargets; ++irt)
			{
				this->d3dPipelineDesc.BlendState.RenderTarget[irt] = GfxBlendStateToD3D12(this->GetBlendState(irt));
			}
		}
		if (RasterizerStateFlag & updateFlags)
		{
			this->d3dPipelineDesc.RasterizerState = GfxRasterizerStateToD3D12(this->GetRasterizerState());
		}
		if (DepthStencilStateFlag & updateFlags)
		{
			this->d3dPipelineDesc.DepthStencilState = GfxDepthStencilStateToD3D12(this->GetDepthStencilState());
		}
		if (InputLayoutFlag & updateFlags)
		{
			if (this->GetInputLayout() != ur_null)
			{
				this->d3dInputLayoutElements.resize(this->GetInputLayout()->GetElements().size());
				for (ur_size i = 0; i < this->d3dInputLayoutElements.size(); ++i)
				{
					this->d3dInputLayoutElements[i] = GfxInputElementToD3D12(this->GetInputLayout()->GetElements()[i]);
				}
				this->d3dPipelineDesc.InputLayout.pInputElementDescs = this->d3dInputLayoutElements.data();
				this->d3dPipelineDesc.InputLayout.NumElements = (UINT)this->d3dInputLayoutElements.size();
			}
			else
			{
				this->d3dInputLayoutElements.clear();
				this->d3dPipelineDesc.InputLayout.pInputElementDescs = ur_null;
				this->d3dPipelineDesc.InputLayout.NumElements = 0;
			}
		}
		if (PrimitiveTopologyFlag & updateFlags)
		{
			this->d3dPipelineDesc.PrimitiveTopologyType = GfxPrimitiveTopologyToD3D12Type(this->GetPrimitiveTopology());
			this->d3dPrimitiveTopology = GfxPrimitiveTopologyToD3D12(this->GetPrimitiveTopology());
		}
		if (RenderTargetFormatFlag & updateFlags)
		{
			this->d3dPipelineDesc.NumRenderTargets = (UINT)this->GetRenderTargetsNumber();
			for (ur_uint i = 0; i < this->GetRenderTargetsNumber(); ++i)
			{
				this->d3dPipelineDesc.RTVFormats[i] = GfxFormatToDXGI(this->GetRenderTargetFormat(i), GfxFormatView::Unorm);
			}
			this->d3dPipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;//GfxFormatToDXGI(this->GetDepthStencilFormat(), GfxFormatView::Typeless);;
		}

		this->d3dPipelineState.reset(ur_null);
		HRESULT hr = d3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, __uuidof(ID3D12PipelineState), this->d3dPipelineState);
		if (FAILED(hr))
			return ResultError(NotInitialized, "GfxPipelineStateObjectD3D12::OnInitialize: failed to create d3d pipeline state");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxResourceBindingD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxResourceBindingD3D12::GfxResourceBindingD3D12(GfxSystem &gfxSystem) :
		GfxResourceBinding(gfxSystem)
	{
	}

	GfxResourceBindingD3D12::~GfxResourceBindingD3D12()
	{
	}

	template <typename TGfxResource, D3D12_DESCRIPTOR_RANGE_TYPE d3dDescriptorRangeType>
	void InitializeRanges(const std::vector<std::pair<ur_uint, TGfxResource*>>& resourceBindings,
		std::vector<D3D12_DESCRIPTOR_RANGE>& d3dDescriptorRanges)
	{
		if (resourceBindings.empty())
			return;

		// order records by slot index
		std::multimap<ur_uint, ur_size> orderedMap;
		for (ur_size idx = 0; idx < resourceBindings.size(); ++idx)
		{
			orderedMap.insert(std::pair<ur_uint, ur_size>(resourceBindings[idx].first, idx));
		}

		// calculate ranges
		ur_uint baseRegister = ur_uint(-2);
		for (auto& indirectRecord : orderedMap)
		{
			const auto& binding = resourceBindings[indirectRecord.second];
			if (binding.first - baseRegister > 1)
			{
				// start new range
				baseRegister = binding.first;
				D3D12_DESCRIPTOR_RANGE range = CD3DX12_DESCRIPTOR_RANGE(d3dDescriptorRangeType, 0, UINT(baseRegister));
				d3dDescriptorRanges.emplace_back(range);
			}
			++d3dDescriptorRanges.back().NumDescriptors;
		}
	}

	Result GfxResourceBindingD3D12::OnInitialize()
	{
		// todo: check whether layout is changed;
		// if it is - reinitialize root tables and signature object;
		// if not - simply copy new resource(s) decriptor(s) into corresponding table descriptor heap(s)

		// reset

		this->d3dDesriptorRangesCbvSrvUav.clear();
		this->d3dDesriptorRangesSampler.clear();
		this->d3dRootParameters.clear();
		this->d3dRootSignature.reset(ur_null);
		this->d3dSerializedRootSignature.reset(ur_null);

		// initialize descriptor table ranges

		InitializeRanges<GfxBuffer, D3D12_DESCRIPTOR_RANGE_TYPE_CBV>(this->GetBuffers(), this->d3dDesriptorRangesCbvSrvUav);
		InitializeRanges<GfxTexture, D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(this->GetTextures(), this->d3dDesriptorRangesCbvSrvUav);
		InitializeRanges<GfxSamplerState, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER>(this->GetSamplers(), this->d3dDesriptorRangesSampler);

		// initialize root signature

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxResourceBindingD3D12::OnInitialize: failed, device unavailable");

		if (!this->d3dDesriptorRangesCbvSrvUav.empty())
		{
			this->d3dRootParameters.emplace_back(D3D12_ROOT_PARAMETER());
			D3D12_ROOT_PARAMETER& rootParameter = this->d3dRootParameters.back();
			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.DescriptorTable.NumDescriptorRanges = (UINT)this->d3dDesriptorRangesCbvSrvUav.size();
			rootParameter.DescriptorTable.pDescriptorRanges = this->d3dDesriptorRangesCbvSrvUav.data();
			rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		}

		if (!this->d3dDesriptorRangesSampler.empty())
		{
			this->d3dRootParameters.emplace_back(D3D12_ROOT_PARAMETER());
			D3D12_ROOT_PARAMETER& rootParameter = this->d3dRootParameters.back();
			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.DescriptorTable.NumDescriptorRanges = (UINT)this->d3dDesriptorRangesSampler.size();
			rootParameter.DescriptorTable.pDescriptorRanges = this->d3dDesriptorRangesSampler.data();
			rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		}

		D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc = {};
		d3dRootSignatureDesc.NumParameters = (UINT)d3dRootParameters.size();
		d3dRootSignatureDesc.pParameters = d3dRootParameters.data();
		d3dRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		shared_ref<ID3DBlob> d3dErrorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, this->d3dSerializedRootSignature, d3dErrorBlob);
		if (FAILED(hr))
			return ResultError(Failure, "GfxResourceBindingD3D12::OnInitialize: failed to serialize d3d root signature");

		hr = d3dDevice->CreateRootSignature(0, this->d3dSerializedRootSignature->GetBufferPointer(), this->d3dSerializedRootSignature->GetBufferSize(),
			__uuidof(ID3D12RootSignature), this->d3dRootSignature);
		if (FAILED(hr))
			return ResultError(Failure, "GfxResourceBindingD3D12::OnInitialize: failed to create d3d root signature");

		// reserve descriptors per non empty root table;
		// this gpu memory location will be used to store shader visible descriptors in an ordered manner (correspnding to table ranges layout);
		// to be able to bind different resources to the same layout - ID3D12Device::CopyDescriptors is used to copy descriptors from random heap(s) locations into root table heap region;
		this->tableDescriptorSets.resize(this->d3dRootParameters.size());
		for (ur_size tableIdx = 0; tableIdx < this->d3dRootParameters.size(); ++tableIdx)
		{
			auto& tableDescriptorSet = this->tableDescriptorSets[tableIdx];
			const auto& d3dDescriptorTable = this->d3dRootParameters[tableIdx].DescriptorTable;
			D3D12_DESCRIPTOR_HEAP_TYPE d3dDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE(-1);
			ur_size descriptorsCount = 0;
			for (UINT irange = 0; irange < d3dDescriptorTable.NumDescriptorRanges; ++irange)
			{
				descriptorsCount += ur_size(d3dDescriptorTable.pDescriptorRanges[irange].NumDescriptors);
				if (d3dDescriptorTable.pDescriptorRanges[irange].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
					d3dDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				else
					d3dDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			}
			d3dSystem.GetDescriptorHeap(d3dDescriptorHeapType)->AcquireDescriptors(tableDescriptorSet, descriptorsCount);
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	D3D12_RESOURCE_DESC GfxTextureDescToD3D12ResDesc(const GfxTextureDesc &desc)
	{
		D3D12_RESOURCE_DESC d3dDesc = {};
		d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		d3dDesc.Width = desc.Width;
		d3dDesc.Height = desc.Height;
		d3dDesc.DepthOrArraySize = 1;
		d3dDesc.MipLevels = desc.Levels;
		d3dDesc.Format = GfxFormatToDXGI(desc.Format, desc.FormatView);
		d3dDesc.SampleDesc.Count = 1;
		d3dDesc.SampleDesc.Quality = 0;
		d3dDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dDesc.Flags = GfxBindFlagsToD3D12ResFlags(desc.BindFlags);
		return d3dDesc;
	}

	D3D12_RESOURCE_DESC GfxBufferDescToD3D12ResDesc(const GfxBufferDesc &desc)
	{
		D3D12_RESOURCE_DESC d3dDesc = {};
		d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dDesc.Alignment = 0;
		d3dDesc.Width = desc.Size;
		d3dDesc.Height = 1;
		d3dDesc.DepthOrArraySize = 1;
		d3dDesc.MipLevels = 1;
		d3dDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dDesc.SampleDesc.Count = 1;
		d3dDesc.SampleDesc.Quality = 0;
		d3dDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		d3dDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		return d3dDesc;
	}

	D3D12_RESOURCE_FLAGS GfxBindFlagsToD3D12ResFlags(const ur_uint gfxFlags)
	{
		D3D12_RESOURCE_FLAGS d3dFlags = D3D12_RESOURCE_FLAG_NONE;
		if (gfxFlags & ur_uint(GfxBindFlag::RenderTarget))
			d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if (gfxFlags & ur_uint(GfxBindFlag::DepthStencil))
			d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if (gfxFlags & ur_uint(GfxBindFlag::UnorderedAccess))
			d3dFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if (!(gfxFlags & ur_uint(GfxBindFlag::ShaderResource)))
			d3dFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		return d3dFlags;
	}

	D3D12_HEAP_TYPE GfxUsageToD3D12HeapType(const GfxUsage gfxUsage)
	{
		D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE(0);
		switch (gfxUsage)
		{
		case GfxUsage::Default:
		case GfxUsage::Immutable:
			d3dHeapType = D3D12_HEAP_TYPE_DEFAULT;
			break;
		case GfxUsage::Dynamic:
			d3dHeapType = D3D12_HEAP_TYPE_UPLOAD;
			break;
		case GfxUsage::Readback:
			d3dHeapType = D3D12_HEAP_TYPE_READBACK;
			break;
		}
		return d3dHeapType;
	}

	D3D12_RESOURCE_STATES GfxBindFlagsAndUsageToD3D12ResState(ur_uint gfxBindFlags, const GfxUsage gfxUsage)
	{
		D3D12_RESOURCE_STATES d3dStates = D3D12_RESOURCE_STATES(-1);
		if (GfxUsage::Dynamic == gfxUsage)
			d3dStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		else if (GfxUsage::Readback == gfxUsage)
			d3dStates = D3D12_RESOURCE_STATE_COPY_DEST;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::RenderTarget))
			d3dStates = D3D12_RESOURCE_STATE_RENDER_TARGET;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::DepthStencil))
			d3dStates = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::VertexBuffer))
			d3dStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::ConstantBuffer))
			d3dStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::IndexBuffer))
			d3dStates = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::UnorderedAccess))
			d3dStates = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::ShaderResource))
			d3dStates = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		else if (gfxBindFlags & ur_uint(GfxBindFlag::StreamOutput))
			d3dStates = D3D12_RESOURCE_STATE_STREAM_OUT;
		return d3dStates;
	}

	D3D12_SUBRESOURCE_DATA GfxResourceDataToD3D12(const GfxResourceData& gfxData)
	{
		D3D12_SUBRESOURCE_DATA d3dSubResData = {};
		d3dSubResData.pData = gfxData.Ptr;
		d3dSubResData.RowPitch = gfxData.RowPitch;
		d3dSubResData.SlicePitch = gfxData.SlicePitch;
		return d3dSubResData;
	}

	D3D12_RENDER_TARGET_BLEND_DESC GfxBlendStateToD3D12(const GfxBlendState &state)
	{
		D3D12_RENDER_TARGET_BLEND_DESC d3dDesc = {};
		d3dDesc.BlendEnable = (BOOL)state.BlendEnable;
		d3dDesc.SrcBlend = GfxBlendFactorToD3D12(state.SrcBlend);
		d3dDesc.DestBlend = GfxBlendFactorToD3D12(state.DstBlend);
		d3dDesc.SrcBlendAlpha = GfxBlendFactorToD3D12(state.SrcBlendAlpha);
		d3dDesc.DestBlendAlpha = GfxBlendFactorToD3D12(state.DstBlendAlpha);
		d3dDesc.BlendOp = GfxBlendOpToD3D12(state.BlendOp);
		d3dDesc.BlendOpAlpha = GfxBlendOpToD3D12(state.BlendOpAlpha);
		d3dDesc.RenderTargetWriteMask = (UINT8)state.RenderTargetWriteMask;
		d3dDesc.LogicOpEnable = FALSE;
		d3dDesc.LogicOp = D3D12_LOGIC_OP(0);
		return d3dDesc;
	}

	D3D12_BLEND GfxBlendFactorToD3D12(GfxBlendFactor blendFactor)
	{
		switch (blendFactor)
		{
		case GfxBlendFactor::Zero: return D3D12_BLEND_ZERO;
		case GfxBlendFactor::One: return D3D12_BLEND_ONE;
		case GfxBlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
		case GfxBlendFactor::InvSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
		case GfxBlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
		case GfxBlendFactor::InvSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
		case GfxBlendFactor::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
		case GfxBlendFactor::InvDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
		case GfxBlendFactor::DstColor: return D3D12_BLEND_DEST_COLOR;
		case GfxBlendFactor::InvDstColor: return D3D12_BLEND_INV_DEST_COLOR;
		}
		return D3D12_BLEND(0);
	}

	D3D12_BLEND_OP GfxBlendOpToD3D12(GfxBlendOp blendOp)
	{
		switch (blendOp)
		{
		case GfxBlendOp::Add: return D3D12_BLEND_OP_ADD;
		case GfxBlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
		case GfxBlendOp::RevSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
		case GfxBlendOp::Min: return D3D12_BLEND_OP_MIN;
		case GfxBlendOp::Max: return D3D12_BLEND_OP_MAX;
		}
		return D3D12_BLEND_OP(0);
	}

	D3D12_RASTERIZER_DESC GfxRasterizerStateToD3D12(const GfxRasterizerState& state)
	{
		D3D12_RASTERIZER_DESC d3dDesc = {};
		d3dDesc.FillMode = GfxFillModeToD3D12(state.FillMode);
		d3dDesc.CullMode = GfxCullModeToD3D12(state.CullMode);
		d3dDesc.DepthBias = (INT)state.DepthBias;
		d3dDesc.DepthBiasClamp = (FLOAT)state.DepthBiasClamp;
		d3dDesc.SlopeScaledDepthBias = (FLOAT)state.SlopeScaledDepthBias;
		d3dDesc.DepthClipEnable = (BOOL)state.DepthClipEnable;
		d3dDesc.MultisampleEnable = (BOOL)state.MultisampleEnable;
		d3dDesc.AntialiasedLineEnable = FALSE;
		d3dDesc.FrontCounterClockwise = FALSE;
		d3dDesc.ForcedSampleCount = 0;
		d3dDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		return d3dDesc;
	}

	D3D12_FILL_MODE GfxFillModeToD3D12(GfxFillMode mode)
	{
		switch (mode)
		{
		case GfxFillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
		case GfxFillMode::Solid: return D3D12_FILL_MODE_SOLID;
		}
		return D3D12_FILL_MODE(0);
	}

	D3D12_CULL_MODE GfxCullModeToD3D12(GfxCullMode mode)
	{
		switch (mode)
		{
		case GfxCullMode::None: return D3D12_CULL_MODE_NONE;
		case GfxCullMode::CW: return D3D12_CULL_MODE_FRONT;
		case GfxCullMode::CCW: return D3D12_CULL_MODE_BACK;
		}
		return D3D12_CULL_MODE(0);
	}

	D3D12_DEPTH_STENCIL_DESC GfxDepthStencilStateToD3D12(const GfxDepthStencilState &state)
	{
		D3D12_DEPTH_STENCIL_DESC d3dDesc = {};
		d3dDesc.DepthEnable = (BOOL)state.DepthEnable;
		d3dDesc.DepthWriteMask = (state.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO);
		d3dDesc.DepthFunc = GfxCmpFuncToD3D12(state.DepthFunc);
		d3dDesc.StencilEnable = (BOOL)state.StencilEnable;
		d3dDesc.StencilReadMask = (UINT8)state.StencilReadMask;
		d3dDesc.StencilWriteMask = (UINT8)state.StencilWriteMask;
		d3dDesc.FrontFace = GfxDepthStencilOpDescToD3D12(state.FrontFace);
		d3dDesc.BackFace = GfxDepthStencilOpDescToD3D12(state.BackFace);
		return d3dDesc;
	}

	D3D12_COMPARISON_FUNC GfxCmpFuncToD3D12(GfxCmpFunc func)
	{
		switch (func)
		{
		case GfxCmpFunc::Never: return D3D12_COMPARISON_FUNC_NEVER;
		case GfxCmpFunc::Less: return D3D12_COMPARISON_FUNC_LESS;
		case GfxCmpFunc::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
		case GfxCmpFunc::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case GfxCmpFunc::Greater: return D3D12_COMPARISON_FUNC_GREATER;
		case GfxCmpFunc::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case GfxCmpFunc::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case GfxCmpFunc::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
		}
		return D3D12_COMPARISON_FUNC(0);
	}

	D3D12_STENCIL_OP GfxStencilOpToD3D12(GfxStencilOp stencilOp)
	{
		switch (stencilOp)
		{
		case GfxStencilOp::Keep: return D3D12_STENCIL_OP_KEEP;
		case GfxStencilOp::Zero: return D3D12_STENCIL_OP_ZERO;
		case GfxStencilOp::Replace: return D3D12_STENCIL_OP_REPLACE;
		case GfxStencilOp::IncrSat: return D3D12_STENCIL_OP_INCR_SAT;
		case GfxStencilOp::DecrSat: return D3D12_STENCIL_OP_DECR_SAT;
		case GfxStencilOp::Invert: return D3D12_STENCIL_OP_INVERT;
		case GfxStencilOp::Incr: return D3D12_STENCIL_OP_INCR;
		case GfxStencilOp::Decr: return D3D12_STENCIL_OP_DECR;
		}
		return D3D12_STENCIL_OP(0);
	}

	D3D12_DEPTH_STENCILOP_DESC GfxDepthStencilOpDescToD3D12(const GfxDepthStencilOpDesc& desc)
	{
		D3D12_DEPTH_STENCILOP_DESC d3dDesc = {};
		d3dDesc.StencilFailOp = GfxStencilOpToD3D12(desc.StencilFailOp);
		d3dDesc.StencilDepthFailOp = GfxStencilOpToD3D12(desc.StencilDepthFailOp);
		d3dDesc.StencilPassOp = GfxStencilOpToD3D12(desc.StencilPassOp);
		d3dDesc.StencilFunc = GfxCmpFuncToD3D12(desc.StencilFunc);
		return d3dDesc;
	}

	LPCSTR GfxSemanticToD3D12(GfxSemantic semantic)
	{
		switch (semantic)
		{
		case GfxSemantic::Position: return "POSITION";
		case GfxSemantic::TexCoord: return "TEXCOORD";
		case GfxSemantic::Color: return "COLOR";
		case GfxSemantic::Tangtent: return "TANGENT";
		case GfxSemantic::Normal: return "NORMAL";
		case GfxSemantic::Binormal: return "BINORMAL";
		}
		return "UNKNOWN";
	}

	D3D12_INPUT_ELEMENT_DESC GfxInputElementToD3D12(const GfxInputElement &element)
	{
		D3D12_INPUT_ELEMENT_DESC d3dDesc = {};
		d3dDesc.SemanticName = GfxSemanticToD3D12(element.Semantic);
		d3dDesc.SemanticIndex = element.SemanticIdx;
		d3dDesc.Format = GfxFormatToDXGI(element.Format, element.FormatView);
		d3dDesc.InputSlot = element.SlotIdx;
		d3dDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		d3dDesc.InputSlotClass = (element.InstanceStepRate > 0 ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
		d3dDesc.InstanceDataStepRate = element.InstanceStepRate;
		return d3dDesc;
	}

	D3D12_PRIMITIVE_TOPOLOGY GfxPrimitiveTopologyToD3D12(GfxPrimitiveTopology topology)
	{
		switch (topology)
		{
		case GfxPrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case GfxPrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case GfxPrimitiveTopology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case GfxPrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case GfxPrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE GfxPrimitiveTopologyToD3D12Type(GfxPrimitiveTopology topology)
	{
		switch (topology)
		{
		case GfxPrimitiveTopology::PointList:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case GfxPrimitiveTopology::LineList:
		case GfxPrimitiveTopology::LineStrip:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case GfxPrimitiveTopology::TriangleList:
		case GfxPrimitiveTopology::TriangleStrip:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}

	D3D12_VIEWPORT GfxViewPortToD3D12(const GfxViewPort &viewPort)
	{
		D3D12_VIEWPORT d3dViewPort;
		d3dViewPort.TopLeftX = viewPort.X;
		d3dViewPort.TopLeftY = viewPort.Y;
		d3dViewPort.Width = viewPort.Width;
		d3dViewPort.Height = viewPort.Height;
		d3dViewPort.MinDepth = viewPort.DepthMin;
		d3dViewPort.MaxDepth = viewPort.DepthMax;
		return d3dViewPort;
	}

	HRESULT FillUploadResource(ID3D12Resource *uploadResource, ur_uint firstSubresource, ur_uint numSubresources, const D3D12_SUBRESOURCE_DATA *srcData)
	{
		if (ur_null == uploadResource || ur_null == srcData)
			return E_INVALIDARG;

		shared_ref<ID3D12Device> d3dDevice;
		uploadResource->GetDevice(__uuidof(ID3D12Device), d3dDevice);

		// fill descriptive structures

		D3D12_RESOURCE_DESC resourceDesc = uploadResource->GetDesc();
		ur_size descBufferSize = static_cast<ur_size>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * numSubresources;
		std::vector<ur_byte> descBuffer(descBufferSize);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(descBuffer.data());
		UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + numSubresources);
		UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + numSubresources);
		const UINT64 baseOffset = 0;
		UINT64 requiredSize = 0;
		d3dDevice->GetCopyableFootprints(&resourceDesc, firstSubresource, numSubresources, baseOffset, pLayouts, pNumRows, pRowSizesInBytes, &requiredSize);

		// do copy

		ur_byte* pData;
		HRESULT hr = uploadResource->Map(0, NULL, reinterpret_cast<void**>(&pData));
		if (FAILED(hr))
			return hr;

		for (UINT i = 0; i < numSubresources; ++i)
		{
			D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
			MemcpySubresource(&DestData, &srcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
		}
		uploadResource->Unmap(0, NULL);

		return S_OK;
	}

	HRESULT UpdateSubresources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, ID3D12Resource* uploadResource,
		ur_uint dstSubresource, ur_uint srcSubresource, ur_uint numSubresources)
	{
		if (ur_null == commandList || ur_null == dstResource || ur_null == uploadResource)
			return E_INVALIDARG;

		shared_ref<ID3D12Device> d3dDevice;
		commandList->GetDevice(__uuidof(ID3D12Device), d3dDevice);

		// fill descriptive structures

		D3D12_RESOURCE_DESC dstDesc = dstResource->GetDesc();
		ur_size dstMemDescSize = static_cast<ur_size>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * numSubresources;
		std::vector<ur_byte> dstMemDescBuffer(dstMemDescSize);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* dstLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(dstMemDescBuffer.data());
		UINT64* dstRowSizesInBytes = reinterpret_cast<UINT64*>(dstLayouts + numSubresources);
		UINT* dstNumRows = reinterpret_cast<UINT*>(dstRowSizesInBytes + numSubresources);
		const UINT64 dstBaseOffset = 0;
		UINT64 dstRequiredSize = 0;
		d3dDevice->GetCopyableFootprints(&dstDesc, dstSubresource, numSubresources, dstBaseOffset, dstLayouts, dstNumRows, dstRowSizesInBytes, &dstRequiredSize);

		D3D12_RESOURCE_DESC srcDesc = uploadResource->GetDesc();
		ur_size srcMemDescSize = dstMemDescSize;
		std::vector<ur_byte> srcMemDescBuffer(srcMemDescSize);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* srcLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(srcMemDescBuffer.data());
		UINT64* srcRowSizesInBytes = reinterpret_cast<UINT64*>(srcLayouts + numSubresources);
		UINT* srcNumRows = reinterpret_cast<UINT*>(srcRowSizesInBytes + numSubresources);
		const UINT64 srcBaseOffset = 0;
		UINT64 srcRequiredSize = 0;
		d3dDevice->GetCopyableFootprints(&srcDesc, srcSubresource, numSubresources, srcBaseOffset, srcLayouts, srcNumRows, srcRowSizesInBytes, &srcRequiredSize);

		if (srcRequiredSize != dstRequiredSize)
			return E_FAIL;

		// schedule a copy

		if (dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			commandList->CopyBufferRegion(dstResource, 0, uploadResource, dstLayouts[0].Offset, dstLayouts[0].Footprint.Width);
		}
		else
		{
			for (UINT i = 0; i < numSubresources; ++i)
			{
				CD3DX12_TEXTURE_COPY_LOCATION dstLocation(dstResource, i + dstSubresource);
				CD3DX12_TEXTURE_COPY_LOCATION srcLocation(uploadResource, srcLayouts[i]);
				commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
			}
		}

		return S_OK;
	}

} // end namespace UnlimRealms