///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Graf/GrafRenderer.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "Sys/JobSystem.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const GrafRenderer::InitParams GrafRenderer::InitParams::Default = {
		GrafRenderer::InitParams::RecommendedDeviceId,
		GrafCanvas::InitParams::Default,
		64 * (1 << 20), // DynamicUploadBufferSize
		16 * (1 << 20), // DynamicConstantBufferSize
	};

	const ur_uint GrafRenderer::InitParams::RecommendedDeviceId = ur_uint(-1);

	GrafRenderer::GrafRenderer(Realm &realm) :
		RealmEntity(realm)
	{
	}

	GrafRenderer::~GrafRenderer()
	{
		this->Deinitialize();
	}

	Result GrafRenderer::Initialize(std::unique_ptr<GrafSystem> grafSystem, const InitParams& initParams)
	{
		this->grafSystem.reset(grafSystem.release());
		if (ur_null == this->grafSystem.get())
			return ResultError(InvalidArgs, "GrafRenderer: failed to initialize, invalid GrafSystem");

		Result res(Success);
		const char* crntStageLogName = "";
		do
		{
			// graphics device
			crntStageLogName = "GrafDevice";
			res = this->grafSystem->CreateDevice(this->grafDevice);
			if (Failed(res)) break;
			ur_uint deviceId = (InitParams::RecommendedDeviceId == initParams.DeviceId ? this->grafSystem->GetRecommendedDeviceId() : initParams.DeviceId);
			res = this->grafDevice->Initialize(deviceId);
			if (Failed(res)) break;

			// presentation canvas (swapchain)
			crntStageLogName = "GrafCanvas";
			res = this->grafSystem->CreateCanvas(this->grafCanvas);
			if (Failed(res)) break;
			this->grafCanvasParams = initParams.CanvasParams;
			res = this->grafCanvas->Initialize(this->grafDevice.get(), initParams.CanvasParams);
			if (Failed(res)) break;

			// number of recorded (in flight) frames
			// can differ from swap chain size
			this->recordedFrameCount = std::max(ur_uint(2), this->grafCanvas->GetSwapChainImageCount() - 1);
			this->recordedFrameIdx = 0;
			this->rendererFrameIdx = 0;
			//this->pendingCommandListCallbacksIdx = 0;

			// swap chain render pass
			// used to render into swap chain render target(s)
			crntStageLogName = "swap chain render pass";
			res = this->grafSystem->CreateRenderPass(this->grafCanvasRenderPass);
			if (Failed(res)) break;
			GrafRenderPassImageDesc grafCanvasRenderPassImages[] = {
				{
					initParams.CanvasParams.Format,
					GrafImageState::ColorWrite,
					GrafImageState::ColorWrite,
					GrafRenderPassDataOp::Load,
					GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::DontCare,
					GrafRenderPassDataOp::DontCare
				}
			};
			GrafRenderPassDesc grafCanvasRenderPassDesc = {
				grafCanvasRenderPassImages, ur_array_size(grafCanvasRenderPassImages)
			};
			res = this->grafCanvasRenderPass->Initialize(this->grafDevice.get(), { grafCanvasRenderPassDesc });
			if (Failed(res)) break;

			// swap chain image(s) render target(s)
			crntStageLogName = "swap chain render target(s)";
			res = this->InitializeCanvasRenderTargets();
			if (Failed(res)) break;

			// dynamic upload buffer
			crntStageLogName = "dynamic upload buffer";
			res = this->grafSystem->CreateBuffer(this->grafDynamicUploadBuffer);
			if (Failed(res)) break;
			GrafBufferDesc uploadBufferDesc = {};
			uploadBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::TransferSrc;
			uploadBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			uploadBufferDesc.SizeInBytes = initParams.DynamicUploadBufferSize;
			res = this->grafDynamicUploadBuffer->Initialize(this->grafDevice.get(), { uploadBufferDesc });
			if (Failed(res)) break;
			this->uploadBufferAllocator.Init(uploadBufferDesc.SizeInBytes, this->grafDevice->GetPhysicalDeviceDesc()->ImageDataPlacementAlignment, true);

			// dynamic constant buffer
			crntStageLogName = "dynamic constant buffer";
			res = this->grafSystem->CreateBuffer(this->grafDynamicConstantBuffer);
			if (Failed(res)) break;
			GrafBufferDesc constantBufferDesc = {};
			constantBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::ConstantBuffer;
			constantBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			constantBufferDesc.SizeInBytes = initParams.DynamicConstantBufferSize;
			res = this->grafDynamicConstantBuffer->Initialize(this->grafDevice.get(), { constantBufferDesc });
			if (Failed(res)) break;
			this->constantBufferAllocator.Init(constantBufferDesc.SizeInBytes, this->grafDevice->GetPhysicalDeviceDesc()->ConstantBufferOffsetAlignment, true);

		} while (false);

		if (Failed(res))
		{
			this->Deinitialize();
			LogError(std::string("GrafRenderer: failed to initialize ") + crntStageLogName);
		}

		return res;
	}

	Result GrafRenderer::InitializeCanvasRenderTargets()
	{
		Result res = NotInitialized;

		// RT count must be equal to swap chain size (one RT wraps on swap chain image)
		this->grafCanvasRenderTarget.clear();
		this->grafCanvasRenderTarget.resize(this->grafCanvas->GetSwapChainImageCount());
		for (ur_uint imageIdx = 0; imageIdx < this->grafCanvas->GetSwapChainImageCount(); ++imageIdx)
		{
			res = this->grafSystem->CreateRenderTarget(this->grafCanvasRenderTarget[imageIdx]);
			if (Failed(res)) break;
			GrafImage* renderTargetImages[] = {
				this->grafCanvas->GetSwapChainImage(imageIdx)
			};
			GrafRenderTarget::InitParams renderTargetParams = {};
			renderTargetParams.RenderPass = this->grafCanvasRenderPass.get();
			renderTargetParams.Images = renderTargetImages;
			renderTargetParams.ImageCount = ur_array_size(renderTargetImages);
			res = this->grafCanvasRenderTarget[imageIdx]->Initialize(this->grafDevice.get(), renderTargetParams);
			if (Failed(res)) break;
		}

		return res;
	}

	Result GrafRenderer::GetOrCreateCommandListForCurrentThread(GrafCommandList*& grafCommandList)
	{
		Result res(Success);

		// get or create cache for this thread

		this->grafCommandListCacheMutex.lock();
		std::thread::id thisThreadId = std::this_thread::get_id();
		auto threadCacheIter = this->grafCommandListCache.find(thisThreadId);
		if (this->grafCommandListCache.end() == threadCacheIter)
		{
			std::unique_ptr<CommandListCache> cmdListCache(new CommandListCache);
			this->grafCommandListCache[thisThreadId] = std::move(cmdListCache);
			threadCacheIter = this->grafCommandListCache.find(thisThreadId);
		}
		CommandListCache& cmdListCache = *threadCacheIter->second.get();
		this->grafCommandListCacheMutex.unlock();

		// acquire available command list (recycle) or create a new one

		std::lock_guard<std::mutex> lockCmdLists(cmdListCache.cmdListsMutex);

		std::unique_ptr<GrafCommandList> acquiredCmdList;
		if (!cmdListCache.availableCmdLists.empty())
		{
			acquiredCmdList = std::move(cmdListCache.availableCmdLists.back());
			ur_assert(acquiredCmdList->Wait(0) == Success);
			cmdListCache.availableCmdLists.pop_back();
		}
		else
		{
			res = this->grafSystem->CreateCommandList(acquiredCmdList);
			if (Failed(res)) return res;
			res = acquiredCmdList->Initialize(this->grafDevice.get());
			if (Failed(res)) return res;
		}
		
		grafCommandList = acquiredCmdList.get();
		grafCommandList->Begin(); // return command list in open state to ensure UpdateCommandListCache does not reuse it in other thread right after unlock
		cmdListCache.acquiredCmdLists.emplace_back(std::move(acquiredCmdList));
		
		return res;
	}

	Result GrafRenderer::Deinitialize()
	{
		// order matters!

		// make sure there are no resources still used on gpu before destroying
		if (this->grafDevice != ur_null)
		{
			this->grafDevice->Submit();
			this->grafDevice->WaitIdle();
			this->UpdateCommandListCache(true);
			this->ProcessPendingCommandListCallbacks(true);
		}

		// destroy objects
		this->grafCommandListCache.clear();
		this->grafDynamicConstantBuffer.reset();
		this->grafDynamicUploadBuffer.reset();
		this->grafCanvasRenderTarget.clear();
		this->grafCanvasRenderPass.reset();
		this->grafCanvas.reset();
		this->grafDevice.reset();
		this->grafSystem.reset();

		return Result(Success);
	}

	Result GrafRenderer::BeginFrame()
	{
		if (ur_null == this->grafCanvas)
			return Result(NotInitialized);

		// update swapchain resolution
		ur_uint swapchainWidth = 0;
		ur_uint swapchainHeight = 0;
		if (this->grafCanvas->GetCurrentImage() != ur_null)
		{
			swapchainWidth = this->grafCanvas->GetCurrentImage()->GetDesc().Size.x;
			swapchainHeight = this->grafCanvas->GetCurrentImage()->GetDesc().Size.y;
		}
		if (swapchainWidth != this->GetRealm().GetCanvas()->GetClientBound().Width() ||
			swapchainHeight != this->GetRealm().GetCanvas()->GetClientBound().Height())
		{
			if (this->GetRealm().GetCanvas()->GetClientBound().Area() > 0)
			{
				Result res = this->grafCanvas->Initialize(this->grafDevice.get(), this->grafCanvasParams);
				if (Succeeded(res))
				{
					res = this->InitializeCanvasRenderTargets();
				}
				if (Failed(res))
				{
					return ResultError(res.Code, "GrafRenderer: failed to resize swap chain");
				}
			}
		}

		return Result(Success);
	}

	Result GrafRenderer::EndFrameAndPresent()
	{
		Result res(Success);

		// submit command list(s) to device execution queue
		res &= this->grafDevice->Submit();

		// process pending callbacks
		res &= this->ProcessPendingCommandListCallbacks(false);

		// update command lists cache
		res &= this->UpdateCommandListCache(false);

		// present current swap chain image
		ur_bool canvasValid = (this->GetRealm().GetCanvas()->GetClientBound().Area() > 0); // canvas area can be zero if window is minimized
		if (canvasValid)
		{
			res = this->grafCanvas->Present();
		}

		// move to next frame
		++this->rendererFrameIdx;
		this->recordedFrameIdx = ur_uint(this->rendererFrameIdx % this->recordedFrameCount);

		return res;
	}

	Result GrafRenderer::AddCommandListCallback(GrafCommandList *executionCmdList, GrafCallbackContext ctx, GrafCommandListCallback callback)
	{
		if (ur_null == executionCmdList)
			return Result(InvalidArgs);

		std::unique_ptr<PendingCommandListCallbackData> pendingCallback(new PendingCommandListCallbackData());
		pendingCallback->cmdList = executionCmdList;
		pendingCallback->callback = callback;
		pendingCallback->context = ctx;
		this->pendingCommandListMutex.lock();
		this->pendingCommandListCallbacks.push_back(std::move(pendingCallback));
		this->pendingCommandListMutex.unlock();

		return Result(Success);
	}

	Result GrafRenderer::SafeDelete(GrafEntity* grafEnity, GrafCommandList *grafSyncCmdList, ur_bool deleteCmdList)
	{
		if (ur_null == grafEnity)
			return Result(InvalidArgs);

		if (ur_null == grafSyncCmdList)
		{
			#if (0)

			// create synchronization command list
			std::unique_ptr<GrafCommandList> newSyncCmdList;
			Result res = this->grafSystem->CreateCommandList(newSyncCmdList);
			if (Succeeded(res))
			{
				res = newSyncCmdList->Initialize(this->grafDevice.get());
			}
			if (Failed(res))
				return ResultError(Failure, "GrafRenderer: failed to create upload command list");

			// record empty list (we are interesed in submission fence only)
			newSyncCmdList->Begin();
			newSyncCmdList->End();
			this->grafDevice->Record(newSyncCmdList.get());

			grafSyncCmdList = newSyncCmdList.release();
			deleteCmdList = true;
			
			#else
			
			// create or recycle transient command list
			GrafCommandList* transientCmdList = this->GetTransientCommandList();

			// record empty list (we are interesed in submission fence only)
			transientCmdList->Begin();
			transientCmdList->End();
			this->grafDevice->Record(transientCmdList);

			grafSyncCmdList = transientCmdList;
			deleteCmdList = false;

			#endif
		}

		// destroy object when fence is signaled
		this->AddCommandListCallback(grafSyncCmdList, {}, [grafEnity, grafSyncCmdList, deleteCmdList](GrafCallbackContext& ctx) -> Result
		{
			delete grafEnity;
			if (deleteCmdList)
			{
				delete grafSyncCmdList;
			}
			return Result(Success);
		});

		return Result(Success);
	}

	Result GrafRenderer::UpdateCommandListCache(ur_bool forceWait)
	{
		Result res(Success);

		ur_uint64 waitTimeout = (forceWait ? ur_uint64(-1) : 0);

		std::lock_guard<std::mutex> lockCaches(this->grafCommandListCacheMutex);
		for (auto& cmdListCache : this->grafCommandListCache)
		{
			std::lock_guard<std::mutex> lockCmdLists(cmdListCache.second->cmdListsMutex);
			auto cmdListIter = cmdListCache.second->acquiredCmdLists.begin();
			while (cmdListIter != cmdListCache.second->acquiredCmdLists.end())
			{
				auto cmdListNextIter = cmdListIter;
				cmdListNextIter++;
				if (cmdListIter->get()->Wait(waitTimeout) == Success)
				{
					// command list is no longer in use
					std::unique_ptr<GrafCommandList> recycledCmdList = std::move(*cmdListIter);
					cmdListCache.second->availableCmdLists.emplace_back(std::move(recycledCmdList));
					cmdListCache.second->acquiredCmdLists.erase(cmdListIter);
				}
				cmdListIter = cmdListNextIter;
			}
		}

		return res;
	}

	Result GrafRenderer::ProcessPendingCommandListCallbacks(ur_bool immediateMode)
	{
		Result res(Success);

		#if (1)
		ur_bool previousCallbackProcessed = (ur_null == this->finishedCommandListCallbacksJob || this->finishedCommandListCallbacksJob->Finished());
		if (!previousCallbackProcessed && immediateMode)
		{
			this->finishedCommandListCallbacksJob->Wait();
			previousCallbackProcessed = true;
		}
		if (previousCallbackProcessed)
		{
			// check pending command lists and prepare a list of callbacks for background processing
			this->pendingCommandListMutex.lock();
			this->finishedCommandListCallbacks.clear();
			this->finishedCommandListCallbacks.reserve(this->pendingCommandListCallbacks.size());
			ur_size processedItems = 0;
			ur_size processedItemsMax = (immediateMode ? ur_size(-1) : 1024);
			ur_uint64 waitTimeout = (immediateMode ? ur_uint64(-1) : 0);
			ur_size idx = 0;
			while (idx < this->pendingCommandListCallbacks.size() && processedItems < processedItemsMax)
			{
				auto& pendingCallback = this->pendingCommandListCallbacks[idx];
				if (pendingCallback->cmdList->Wait(waitTimeout) == Success)
				{
					// command list is finished, add to processing list
					this->finishedCommandListCallbacks.push_back(std::move(pendingCallback));
					this->pendingCommandListCallbacks[idx] = std::move(this->pendingCommandListCallbacks.back());
					this->pendingCommandListCallbacks.pop_back();
				}
				else
				{
					// still pending
					++idx;
				}
				++processedItems;
			}
			this->pendingCommandListMutex.unlock();

			// execute callbacks
			auto executeCallbacksJobFunc = [](Job::Context& ctx) -> void
			{
				typedef std::vector<std::unique_ptr<PendingCommandListCallbackData>> CommandListCallbacks;
				CommandListCallbacks& commandListCallbacks = *reinterpret_cast<CommandListCallbacks*>(ctx.data);
				Result res(Success);
				for (auto& callbackData : commandListCallbacks)
				{
					res &= callbackData->callback(callbackData->context);
				}
				ctx.resultCode = res;
			};
			if (!this->finishedCommandListCallbacks.empty())
			{
				if (immediateMode)
				{
					Job immediateJob(this->GetRealm().GetJobSystem(), &this->finishedCommandListCallbacks, executeCallbacksJobFunc);
					immediateJob.Execute();
				}
				else
				{
					this->finishedCommandListCallbacksJob = this->GetRealm().GetJobSystem().Add(JobPriority::High, &this->finishedCommandListCallbacks, executeCallbacksJobFunc);
				}
			}
		}
		#else
		ur_bool previousCallbackProcessed = (ur_null == this->processingCommandListCallbacksJob || this->processingCommandListCallbacksJob->Finished());
		if (!previousCallbackProcessed && immediateMode) this->processingCommandListCallbacksJob->Wait();
		if (previousCallbackProcessed)
		{
			this->pendingCommandListMutex.lock();
			auto accumulatedCommandListCallbacks = &this->pendingCommandListCallbacks[this->pendingCommandListCallbacksIdx];
			this->pendingCommandListCallbacksIdx = !this->pendingCommandListCallbacksIdx;
			this->pendingCommandListMutex.unlock();

			// execute accumulated callbacks
			auto& executeCallbacksJobFunc = [](Job::Context& ctx) -> void
			{
				typedef std::vector<std::unique_ptr<PendingCommandListCallbackData>> CommandListCallbacks;
				CommandListCallbacks& commandListCallbacks = *reinterpret_cast<CommandListCallbacks*>(ctx.data);
				Result res(Success);
				ur_size idx = 0;
				while (idx < commandListCallbacks.size())
				{
					auto& pendingCallback = commandListCallbacks[idx];
					if (pendingCallback->cmdList->Wait(0) == Success)
					{
						// command list is finished, execute a callback and remove from the list
						pendingCallback->callback(pendingCallback->context);
						commandListCallbacks[idx] = std::move(commandListCallbacks.back());
						commandListCallbacks.pop_back();
					}
					else
					{
						// still pending
						++idx;
					}
				}
				ctx.resultCode = res;
			};
			if (immediateMode)
			{
				for (ur_uint ilist = 0; ilist < 2; ++ilist)
				{
					Job immediateJob(this->GetRealm().GetJobSystem(), &this->pendingCommandListCallbacks[ilist], executeCallbacksJobFunc);
					immediateJob.Execute();
				}
			}
			else
			{
				this->processingCommandListCallbacksJob = this->GetRealm().GetJobSystem().Add(JobPriority::High, accumulatedCommandListCallbacks, executeCallbacksJobFunc);
			}
			
		}
		#endif

		return res;
	}

	Result GrafRenderer::Upload(ur_byte *dataPtr, GrafBuffer* dstBuffer, ur_size dataSize, ur_size dstOffset)
	{
		if (ur_null == dataPtr || 0 == dataSize || ur_null == dstBuffer)
			return Result(InvalidArgs);
		
		Result res(Success);
		if (ur_uint(GrafDeviceMemoryFlag::CpuVisible) & dstBuffer->GetDesc().MemoryType)
		{
			// write data directly to destination buffer
			res = dstBuffer->Write(dataPtr, dataSize, 0, dstOffset);
		}
		else
		{
			// allocate
			this->uploadBufferMutex.lock();
			Allocation uploadBufferAlloc = this->uploadBufferAllocator.Allocate(dataSize);
			this->uploadBufferMutex.unlock();
			if (0 == uploadBufferAlloc.Size)
				return Result(OutOfMemory);

			// write data to cpu visible dynamic upload buffer
			res = this->grafDynamicUploadBuffer->Write(dataPtr, dataSize, 0, uploadBufferAlloc.Offset);
			if (Failed(res))
				return ResultError(Failure, "GrafRenderer: failed to write to upload buffer");

			res = this->Upload(this->grafDynamicUploadBuffer.get(), dstBuffer, dataSize, uploadBufferAlloc.Offset, dstOffset);
		}

		return res;
	}

	Result GrafRenderer::Upload(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		// TODO: cached command list from GetTransientCommandList() should be used here
		// simultaneous Wait() threading issue must be fixed first (a call in cmd list Begin() and another in UpdateCommandListCache())

		// create upload command list
		std::unique_ptr<GrafCommandList> grafUploadCmdList;
		Result res = this->grafSystem->CreateCommandList(grafUploadCmdList);
		if (Succeeded(res))
		{
			res = grafUploadCmdList->Initialize(this->grafDevice.get());
		}
		if (Failed(res))
			return ResultError(Failure, "GrafRenderer: failed to create upload command list");

		// copy dynamic buffer region to destination buffer
		grafUploadCmdList->Begin();
		grafUploadCmdList->Copy(srcBuffer, dstBuffer, dataSize, srcOffset, dstOffset);
		grafUploadCmdList->End();
		this->grafDevice->Record(grafUploadCmdList.get());

		// destroy upload command list when finished
		GrafCommandList* uploadCmdListPtr = grafUploadCmdList.release();
		this->AddCommandListCallback(uploadCmdListPtr, { uploadCmdListPtr }, [](GrafCallbackContext& ctx) -> Result
		{
			GrafCommandList* finishedUploadCmdList = reinterpret_cast<GrafCommandList*>(ctx.DataPtr);
			delete finishedUploadCmdList;
			return Result(Success);
		});

		return Result(Success);
	}

	Result GrafRenderer::Upload(ur_byte *dataPtr, ur_size dataSize, GrafImage* dstImage, GrafImageState dstImageState)
	{
		if (ur_null == dataPtr || 0 == dataSize || ur_null == dstImage)
			return Result(InvalidArgs);

		// allocate
		this->uploadBufferMutex.lock();
		Allocation uploadBufferAlloc = this->uploadBufferAllocator.Allocate(dataSize);
		this->uploadBufferMutex.unlock();
		if (0 == uploadBufferAlloc.Size)
			return Result(OutOfMemory);

		// write data to cpu visible dynamic upload buffer
		Result res = this->grafDynamicUploadBuffer->Write(dataPtr, dataSize, 0, uploadBufferAlloc.Offset);
		if (Failed(res))
			return ResultError(Failure, "GrafRenderer: failed to write to upload buffer");

		return this->Upload(this->grafDynamicUploadBuffer.get(), uploadBufferAlloc.Offset, dstImage, dstImageState);
	}

	Result GrafRenderer::Upload(GrafBuffer *srcBuffer, ur_size srcOffset, GrafImage* dstImage, GrafImageState dstImageState)
	{
		// create upload command list
		std::unique_ptr<GrafCommandList> grafUploadCmdList;
		Result res = this->grafSystem->CreateCommandList(grafUploadCmdList);
		if (Succeeded(res))
		{
			res = grafUploadCmdList->Initialize(this->grafDevice.get());
		}
		if (Failed(res))
			return ResultError(Failure, "GrafRenderer: failed to create upload command list");

		// copy dynamic buffer region to destination image
		grafUploadCmdList->Begin();
		grafUploadCmdList->ImageMemoryBarrier(dstImage, GrafImageState::Current, GrafImageState::TransferDst);
		grafUploadCmdList->Copy(srcBuffer, dstImage, srcOffset);
		grafUploadCmdList->ImageMemoryBarrier(dstImage, GrafImageState::Current, dstImageState);
		grafUploadCmdList->End();
		this->grafDevice->Record(grafUploadCmdList.get());

		// destroy upload command list when finished
		GrafCommandList* uploadCmdListPtr = grafUploadCmdList.release();
		this->AddCommandListCallback(uploadCmdListPtr, { uploadCmdListPtr }, [](GrafCallbackContext& ctx) -> Result
		{
			GrafCommandList* finishedUploadCmdList = reinterpret_cast<GrafCommandList*>(ctx.DataPtr);
			delete finishedUploadCmdList;
			return Result(Success);
		});

		return Result(Success);
	}

	Result GrafRenderer::ShowImgui()
	{
		if (ur_null == this->grafSystem)
			return Result(NotInitialized);

		const RectI& canvasRect = this->GetRealm().GetCanvas()->GetClientBound();
		const ImVec2 windowSize(400.0f, (float)canvasRect.Height());
		const bool treeNodesOpenFirestTime = false;
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
		ImGui::SetNextWindowPos({ canvasRect.Width() - windowSize.x, 0.0f }, ImGuiCond_Once);
		ImGui::Begin("GrafRenderer");
		
		ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
		if (ImGui::TreeNode("GrafSystem"))
		{
			ImGui::Text("Implementation: %s", typeid(*this->grafSystem.get()).name());
			if (this->grafDevice != ur_null)
			{
				ImGui::Text("Device used: %s", this->grafDevice->GetPhysicalDeviceDesc()->Description.c_str());
			}
			
			ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
			if (ImGui::TreeNode("Devices Available"))
			{
				for (ur_uint idevice = 0; idevice < this->grafSystem->GetPhysicalDeviceCount(); ++idevice)
				{
					const GrafPhysicalDeviceDesc* deviceDesc = this->grafSystem->GetPhysicalDeviceDesc(idevice);
					ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
					if (ImGui::TreeNode("PhysicalDeviceNode", "%s", deviceDesc->Description.c_str()))
					{
						ImGui::Text("Local Memory (Mb): %u", deviceDesc->LocalMemory / (1 << 20));
						ImGui::Text("	Exclusive (Mb): %u", deviceDesc->LocalMemoryExclusive / (1 << 20));
						ImGui::Text("	Host Visible (Mb): %u", deviceDesc->LocalMemoryHostVisible / (1 << 20));
						ImGui::Text("System Memory (Mb): %u", deviceDesc->SystemMemory / (1 << 20));
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}

			if (this->grafCanvas != ur_null)
			{
				ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
				if (ImGui::TreeNode("Swap Chain"))
				{
					ImGui::Text("Image count: %i", this->grafCanvas->GetSwapChainImageCount());
					if (this->grafCanvas->GetSwapChainImageCount() > 0)
					{
						const GrafImageDesc& imageDesc = this->grafCanvas->GetCurrentImage()->GetDesc();
						ImGui::Text("Image Size: %i x %i", imageDesc.Size.x, imageDesc.Size.y);
					}
					ImGui::Text("Recorded frame count: %i", this->recordedFrameCount);
					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

		if (this->grafDynamicUploadBuffer)
		{
			ur_size crntAllocatorOffset = 0;
			ur_size allocatorSize = 0;
			{
				std::lock_guard<std::mutex> lock(this->uploadBufferMutex);
				crntAllocatorOffset = this->uploadBufferAllocator.GetOffset();
				allocatorSize = this->uploadBufferAllocator.GetSize();
			}
			static ur_size prevAllocatorOffset = 0;
			static ur_size allocatorOffsetDeltaMax = 0;
			ur_size allocatorOffsetDelta = crntAllocatorOffset + (crntAllocatorOffset < prevAllocatorOffset ? this->uploadBufferAllocator.GetSize() : 0) - prevAllocatorOffset;
			allocatorOffsetDeltaMax = std::max(allocatorOffsetDelta, allocatorOffsetDeltaMax);
			prevAllocatorOffset = crntAllocatorOffset;
			ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
			if (ImGui::TreeNode("Dynamic Upload Buffer"))
			{
				ImGui::Text("Size: %i", this->grafDynamicUploadBuffer->GetDesc().SizeInBytes);
				ImGui::Text("Maximal usage per frame: %i", allocatorOffsetDeltaMax);
				ImGui::Text("Current frame usage: %i", allocatorOffsetDelta);
				ImGui::Text("Current ofs: %i", this->uploadBufferAllocator.GetOffset());
				ImGui::TreePop();
			}
		}

		if (this->grafDynamicConstantBuffer)
		{
			ur_size crntAllocatorOffset = 0;
			ur_size allocatorSize = 0;
			{
				std::lock_guard<std::mutex> lock(this->constantBufferMutex);
				crntAllocatorOffset = this->constantBufferAllocator.GetOffset();
				allocatorSize = this->constantBufferAllocator.GetSize();
			}
			static ur_size prevAllocatorOffset = 0;
			static ur_size allocatorOffsetDeltaMax = 0;
			ur_size allocatorOffsetDelta = crntAllocatorOffset + (crntAllocatorOffset < prevAllocatorOffset ? allocatorSize : 0) - prevAllocatorOffset;
			allocatorOffsetDeltaMax = std::max(allocatorOffsetDelta, allocatorOffsetDeltaMax);
			prevAllocatorOffset = crntAllocatorOffset;
			ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
			if (ImGui::TreeNode("Dynamic Constant Buffer"))
			{
				ImGui::Text("Size: %i", this->grafDynamicConstantBuffer->GetDesc().SizeInBytes);
				ImGui::Text("Maximal usage per frame: %i", allocatorOffsetDeltaMax);
				ImGui::Text("Current frame usage: %i", allocatorOffsetDelta);
				ImGui::Text("Current ofs (bytes): %i", crntAllocatorOffset);
				ImGui::TreePop();
			}
		}

		ImGui::SetNextItemOpen(treeNodesOpenFirestTime, ImGuiCond_Once);
		if (ImGui::TreeNode("Command List Cache"))
		{
			ur_size inflightCount = 0;
			ur_size availableCount = 0;
			if (ImGui::TreeNode("per thread"))
			{
				std::lock_guard<std::mutex> lock(this->grafCommandListCacheMutex);
				ur_uint32 cacheIdx = 0;
				for (auto& cmdListCache : this->grafCommandListCache)
				{
					std::lock_guard<std::mutex> lockCmdLists(cmdListCache.second->cmdListsMutex);
					availableCount += cmdListCache.second->availableCmdLists.size();
					inflightCount += cmdListCache.second->acquiredCmdLists.size();
					ImGui::Text("cache %i: %i", cacheIdx, cmdListCache.second->availableCmdLists.size());
					++cacheIdx;
				}
				ImGui::TreePop();
			}
			ImGui::Text("Available: %i", availableCount);
			ImGui::Text("In flight: %i", inflightCount);
			ImGui::TreePop();
		}

		{
			std::lock_guard<std::mutex> lock(this->pendingCommandListMutex);
			ImGui::Text("PendingCallbacks: %i", this->pendingCommandListCallbacks.size());
		}

		ImGui::End();

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRendererEntity::GrafRendererEntity(GrafRenderer& grafRenderer) :
		RealmEntity(grafRenderer.GetRealm()),
		grafRenderer(grafRenderer)
	{
	}

	GrafRendererEntity::~GrafRendererEntity()
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafManagedCommandList::GrafManagedCommandList(GrafRenderer& grafRenderer) :
		GrafRendererManagedEntity<GrafCommandList>(grafRenderer)
	{
	}

	GrafManagedCommandList::~GrafManagedCommandList()
	{
	}

	Result GrafManagedCommandList::Initialize()
	{
		this->Deinitialize();

		GrafSystem* grafSystem = this->GetGrafRenderer().GetGrafSystem();
		GrafDevice* grafDevice = this->GetGrafRenderer().GetGrafDevice();
		if (ur_null == grafSystem || ur_null == grafDevice)
		{
			LogError(std::string("GrafManagedCommandList: failed to initialize, invalid args"));
			return Result(InvalidArgs);
		}

		GrafRendererManagedEntity<GrafCommandList>::Initialize();

		for (std::unique_ptr<GrafCommandList>& grafCommandList : this->grafObjectPerFrame)
		{
			Result res = grafSystem->CreateCommandList(grafCommandList);
			if (Succeeded(res))
			{
				res = grafCommandList->Initialize(grafDevice);
			}
			if (Failed(res))
			{
				this->Deinitialize();
				LogError(std::string("GrafManagedCommandList: failed to initialize command list(s)"));
				return Result(res);
			}
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafManagedDescriptorTable::GrafManagedDescriptorTable(GrafRenderer& grafRenderer) :
		GrafRendererManagedEntity<GrafDescriptorTable>(grafRenderer)
	{
	}

	GrafManagedDescriptorTable::~GrafManagedDescriptorTable()
	{
	}

	Result GrafManagedDescriptorTable::Initialize(const GrafDescriptorTable::InitParams& initParams)
	{
		this->Deinitialize();

		GrafSystem* grafSystem = this->GetGrafRenderer().GetGrafSystem();
		GrafDevice* grafDevice = this->GetGrafRenderer().GetGrafDevice();
		if (ur_null == grafSystem || ur_null == grafDevice || ur_null == initParams.Layout)
		{
			LogError(std::string("GrafManagedDescriptorTable: failed to initialize, invalid args"));
			return Result(InvalidArgs);
		}

		GrafRendererManagedEntity<GrafDescriptorTable>::Initialize();

		for (std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable : this->grafObjectPerFrame)
		{
			Result res = grafSystem->CreateDescriptorTable(grafDescriptorTable);
			if (Succeeded(res))
			{
				res = grafDescriptorTable->Initialize(grafDevice, initParams);
			}
			if (Failed(res))
			{
				this->Deinitialize();
				LogError(std::string("GrafManagedDescriptorTable: failed to initialize descriptor table(s)"));
				return Result(res);
			}
		}

		return Result(Success);
	}

} // end namespace UnlimRealms