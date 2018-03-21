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
		this->commandListsId = 0;
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
					break;
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

	Result GfxSystemD3D12::SetFrame(ur_uint frameIndex)
	{
		// add signal command to the current frame's queue
		ur_uint frameFenceValue = this->frameFenceValues[frameIndex];
		this->d3dCommandQueue->Signal(this->d3dFrameFence, frameFenceValue);

		// setup new frame
		this->frameIndex = frameIndex;
		if (!this->IsFrameComplete(this->frameIndex))
		{
			this->WaitFrame(this->frameIndex);
		}
		this->frameFenceValues[frameIndex] = frameFenceValue + 1;

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

	Result GfxSystemD3D12::CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader)
	{
		gfxVertexShader.reset(new GfxVertexShaderD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader)
	{
		gfxPixelShader.reset(new GfxPixelShaderD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout)
	{
		gfxInputLayout.reset(new GfxInputLayoutD3D12(*this));
		return Result(Success);
	}

	Result GfxSystemD3D12::CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState)
	{
		gfxPipelineState.reset(new GfxPipelineStateD3D12(*this));
		return Result(Success);
	}

	GfxSystemD3D12::Descriptor::Descriptor(DescriptorHeap* heap) :
		heap(heap)
	{
		this->cpuHandle.ptr = 0;
		this->gpuHandle.ptr = 0;
		this->heapIdx = ur_size(-1);
	}

	GfxSystemD3D12::Descriptor::~Descriptor()
	{
		if (this->heap != ur_null)
		{
			this->heap->ReleaseDescriptor(*this);
		}
	}

	GfxSystemD3D12::DescriptorHeap::DescriptorHeap(GfxSystemD3D12& gfxSystem, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType) :
		GfxEntity(gfxSystem),
		d3dHeapType(d3dHeapType)
	{
		this->d3dDescriptorSize = 0;
		this->currentPageIdx = ur_size(-1);
	}
	
	GfxSystemD3D12::DescriptorHeap::~DescriptorHeap()
	{
		// detach extrenal links
		for (auto& descriptor : this->descriptors)
		{
			descriptor->heap = ur_null;
		}
	}

	Result GfxSystemD3D12::DescriptorHeap::AcquireDescriptor(std::unique_ptr<Descriptor>& descriptor)
	{
		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxSystemD3D12::DescriptorHeap::AcquireDescriptor: failed, device unavailable");

		std::lock_guard<std::mutex> lockModify(this->modifyMutex);

		if (this->pagePool.empty() ||
			this->pagePool[currentPageIdx]->freeRanges.empty())
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
			this->currentPageIdx = this->pagePool.size() - 1;
			
			if (0 == this->d3dDescriptorSize)
			{
				this->d3dDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(this->d3dHeapType);
			}
		}

		// grab some page memory
		Page& currentPage = *this->pagePool[this->currentPageIdx].get();
		Range& currentRange = currentPage.freeRanges.back();
		ur_size currentOffset = currentRange.first;
		if (currentRange.first < currentRange.second)
		{
			// move to next record
			currentRange.first += 1;
		}
		else
		{
			// release exhausted range
			currentPage.freeRanges.pop_back();
		}

		// initialize descriptor
		ID3D12DescriptorHeap* currentHeap = currentPage.d3dHeap;
		SIZE_T currentHeapCPUStart = currentHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		UINT64 currentHeapGPUStart = currentHeap->GetGPUDescriptorHandleForHeapStart().ptr;
		descriptor.reset( new Descriptor(this));
		descriptor->cpuHandle.ptr = currentHeapCPUStart + this->d3dDescriptorSize * currentOffset;
		descriptor->gpuHandle.ptr = currentHeapGPUStart + this->d3dDescriptorSize * currentOffset;
		descriptor->heapIdx = currentOffset + this->currentPageIdx * DescriptorsPerHeap;
		this->descriptors.push_back(descriptor.get());

		return Result(Success);
	}

	Result GfxSystemD3D12::DescriptorHeap::ReleaseDescriptor(Descriptor& descriptor)
	{
		std::lock_guard<std::mutex> lockModify(this->modifyMutex);
		
		ur_size pageIdx = descriptor.heapIdx / DescriptorsPerHeap;
		ur_size offset = descriptor.heapIdx % DescriptorsPerHeap;
		Page& page = *this->pagePool[pageIdx].get();
		
		// rearrange free ranges: try adding released item to exiting range, merge ranges or create new range
		for (ur_size ir = 0; ir < page.freeRanges.size(); ++ir)
		{
			Range& range = page.freeRanges[ir];
			if (offset + 1 < range.first)
			{
				page.freeRanges.insert(page.freeRanges.begin() + ir, Range(offset,offset));
				break;
			}
			if (offset + 1 == range.first)
			{
				range.first = offset;
				Range* prevRange = (ir > 0 ? &page.freeRanges[ir - 1] : ur_null);
				if (prevRange && prevRange->second + 1 == offset)
				{
					range.first = prevRange->first;
					page.freeRanges.erase(page.freeRanges.begin() + ir - 1);
				}
				break;
			}
			if (range.second + 1 == offset)
			{
				range.second = offset;
				Range* nextRange = (ir + 1 < page.freeRanges.size() ? &page.freeRanges[ir + 1] : ur_null);
				if (nextRange && nextRange->first == offset + 1)
				{
					range.second = nextRange->second;
					page.freeRanges.erase(page.freeRanges.begin() + ir + 1);
				}
				break;
			}
			if (ir + 1 == page.freeRanges.size())
			{
				page.freeRanges.insert(page.freeRanges.begin() + ir, Range(offset, offset));
				break;
			}
		}

		// released empty page
		if (page.freeRanges.front().second - page.freeRanges.front().first + 1 == DescriptorsPerHeap)
		{
			this->pagePool.erase(this->pagePool.begin() + pageIdx);
		}

		return Result(Success);
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
		return NotImplemented;
	}

	Result GfxContextD3D12::SetViewPort(const GfxViewPort *viewPort)
	{
		return NotImplemented;
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

	Result GfxContextD3D12::SetPipelineState(GfxPipelineState *state)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetTexture(GfxTexture *texture, ur_uint slot)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetConstantBuffer(GfxBuffer *buffer, ur_uint slot)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetVertexBuffer(GfxBuffer *buffer, ur_uint slot, ur_uint stride, ur_uint offset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetIndexBuffer(GfxBuffer *buffer, ur_uint bitsPerIndex, ur_uint offset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetVertexShader(GfxVertexShader *shader)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::SetPixelShader(GfxPixelShader *shader)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		return NotImplemented;
	}

	Result GfxContextD3D12::UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxTextureD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxTextureD3D12::GfxTextureD3D12(GfxSystem &gfxSystem) :
		GfxTexture(gfxSystem),
		initializedFromD3DRes(false)
	{
	}

	GfxTextureD3D12::~GfxTextureD3D12()
	{
	}

	Result GfxTextureD3D12::Initialize(const GfxTextureDesc &desc, shared_ref<ID3D12Resource> &d3dTexture)
	{
		this->initializedFromD3DRes = true; // setting this flag to skip OnInitialize
		GfxTexture::Initialize(desc, ur_null); // store desc in the base class, 
		this->initializedFromD3DRes = false;

		this->d3dResource.reset(ur_null);
		this->srvDescriptor.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxTextureD3D12: failed to initialize, device unavailable");

		this->d3dResource.reset(d3dTexture);

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
		
			d3dDevice->CreateShaderResourceView(d3dTexture.get(), &srvDesc, this->srvDescriptor->CpuHandle());
		}

		return Result(Success);
	}

	Result GfxTextureD3D12::OnInitialize(const GfxResourceData *data)
	{
		// if texture is being initialized from d3d resource - all the stuff is done in GfxTextureD3D12::Initialize
		// so skip this function as its results will be overriden anyway
		if (this->initializedFromD3DRes)
			return Result(Success);

		this->d3dResource.reset(ur_null);
		this->srvDescriptor.reset(ur_null);

		GfxSystemD3D12 &d3dSystem = static_cast<GfxSystemD3D12&>(this->GetGfxSystem());
		ID3D12Device *d3dDevice = d3dSystem.GetD3DDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxTextureD3D12::OnInitialize: failed, device unavailable");

		// todo: init sub resource data here

		D3D12_RESOURCE_DESC d3dResDesc = GfxTextureDescToD3D12ResDesc(this->GetDesc());
		d3dResDesc.Format = GfxFormatToDXGI(this->GetDesc().Format, GfxFormatView::Typeless);

		D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
		d3dHeapProperties.Type = GfxUsageToD3D12HeapType(this->GetDesc().Usage);
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		D3D12_RESOURCE_STATES d3dResStates = GfxBindFlagsAndUsageToD3D12ResState(this->GetDesc().BindFlags, this->GetDesc().Usage);
		
		HRESULT hr = d3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResDesc, d3dResStates, ur_null,
			__uuidof(ID3D12Resource), this->d3dResource);
		if (FAILED(hr))
			return ResultError(Failure, "GfxTextureD3D12::OnInitialize: failed at CreateCommittedResource");

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
			
			d3dDevice->CreateRenderTargetView(d3dTargetBuffer->GetD3DResource(), &rtvDesc, this->rtvDescriptor->CpuHandle());
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

			d3dDevice->CreateDepthStencilView(d3dDepthStencilBuffer->GetD3DResource(), &dsvDesc, this->dsvDescriptor->CpuHandle());
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSwapChainD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSwapChainD3D12::GfxSwapChainD3D12(GfxSystem &gfxSystem) :
		GfxSwapChain(gfxSystem)
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
		this->d3dCommandList.reset(ur_null);
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

		// create command list to execute resource transitions
		hr = d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3dSystem.GetD3DCommandAllocator(), ur_null,
			__uuidof(ID3D12GraphicsCommandList), this->d3dCommandList);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSwapChainD3D12::Initialize: failed to create command list");
		
		// start in non-recording state
		this->d3dCommandList->Close();

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
		ID3D12Resource* currentFrameBuffer = static_cast<GfxTextureD3D12*>(currentFrameRT->GetTargetBuffer())->GetD3DResource();
		D3D12_RESOURCE_BARRIER d3dResBarrier = {};
		d3dResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResBarrier.Transition.pResource = currentFrameBuffer;
		d3dResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		d3dResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		HRESULT hr = this->d3dCommandList->Reset(d3dSystem.GetD3DCommandAllocator(), ur_null);
		this->d3dCommandList->ResourceBarrier(1, &d3dResBarrier);
		hr = this->d3dCommandList->Close();
		d3dSystem.AddCommandList(shared_ref<ID3D12CommandList>(this->d3dCommandList.get()));
		d3dSystem.Render();
		
		if (FAILED(this->dxgiSwapChain->Present(0, 0)))
			return ResultError(Failure, "GfxSwapChainD3D12::Present: failed");

		// move to next frame

		this->backBufferIndex = (ur_uint)this->dxgiSwapChain->GetCurrentBackBufferIndex();
		d3dSystem.SetFrame(this->backBufferIndex);

		// transition barrier: present -> render target
		GfxRenderTargetD3D12 *newFrameRT = static_cast<GfxRenderTargetD3D12*>(this->GetTargetBuffer());
		ID3D12Resource* newFrameBuffer = static_cast<GfxTextureD3D12*>(newFrameRT->GetTargetBuffer())->GetD3DResource();
		d3dResBarrier.Transition.pResource = newFrameBuffer;
		d3dResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		d3dResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		hr = this->d3dCommandList->Reset(d3dSystem.GetD3DCommandAllocator(), ur_null);
		this->d3dCommandList->ResourceBarrier(1, &d3dResBarrier);
		hr = this->d3dCommandList->Close();
		d3dSystem.AddCommandList(shared_ref<ID3D12CommandList>(this->d3dCommandList.get()));

		return Result(Success);
	}

	GfxRenderTarget* GfxSwapChainD3D12::GetTargetBuffer()
	{
		if (this->backBufferIndex >= (ur_uint)this->backBuffers.size())
			return ur_null;

		return this->backBuffers[this->backBufferIndex].get();
	}

	GfxSwapChainD3D12::BackBuffer::BackBuffer(GfxSystem &gfxSystem, shared_ref<ID3D12Resource> &dxgiSwapChainBuffer) :
		GfxRenderTargetD3D12(gfxSystem),
		dxgiSwapChainBuffer(dxgiSwapChainBuffer)
	{
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
		GfxBuffer(gfxSystem)
	{
	}

	GfxBufferD3D12::~GfxBufferD3D12()
	{
	}

	Result GfxBufferD3D12::OnInitialize(const GfxResourceData *data)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxVertexShaderD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxVertexShaderD3D12::GfxVertexShaderD3D12(GfxSystem &gfxSystem) :
		GfxVertexShader(gfxSystem)
	{
	}

	GfxVertexShaderD3D12::~GfxVertexShaderD3D12()
	{
	}

	Result GfxVertexShaderD3D12::OnInitialize()
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPixelShaderD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxPixelShaderD3D12::GfxPixelShaderD3D12(GfxSystem &gfxSystem) :
		GfxPixelShader(gfxSystem)
	{
	}

	GfxPixelShaderD3D12::~GfxPixelShaderD3D12()
	{
	}

	Result GfxPixelShaderD3D12::OnInitialize()
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxInputLayoutD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxInputLayoutD3D12::GfxInputLayoutD3D12(GfxSystem &gfxSystem) :
		GfxInputLayout(gfxSystem)
	{
	}

	GfxInputLayoutD3D12::~GfxInputLayoutD3D12()
	{
	}

	Result GfxInputLayoutD3D12::OnInitialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPipelineStateD3D12
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxPipelineStateD3D12::GfxPipelineStateD3D12(GfxSystem &gfxSystem) :
		GfxPipelineState(gfxSystem)
	{
	}

	GfxPipelineStateD3D12::~GfxPipelineStateD3D12()
	{
	}

	Result GfxPipelineStateD3D12::OnSetRenderState(const GfxRenderState &renderState)
	{
		return NotImplemented;
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

} // end namespace UnlimRealms