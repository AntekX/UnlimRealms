///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Graf/GrafRenderer.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	const GrafRenderer::InitParams GrafRenderer::InitParams::Default = {
		GrafRenderer::InitParams::RecommendedDeviceId,
		GrafCanvas::InitParams::Default,
		20 * (1 << 20), // DynamicUploadBufferSize
		20 * (1 << 20), // DynamicConstantBufferSize
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

	Result GrafRenderer::Initialize(std::unique_ptr<GrafSystem>& grafSystem, const InitParams& initParams)
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
			this->frameCount = std::max(ur_uint(2), this->grafCanvas->GetSwapChainImageCount() - 1);
			this->frameIdx = 0;

			// swap chain render pass
			// used to render into swap chain render target(s)
			crntStageLogName = "swap chain render pass";
			res = this->grafSystem->CreateRenderPass(this->grafCanvasRenderPass);
			if (Failed(res)) break;
			res = this->grafCanvasRenderPass->Initialize(this->grafDevice.get());
			if (Failed(res)) break;

			// swap chain image(s) render target(s)
			crntStageLogName = "swap chain render target(s)";
			res = this->InitializeCanvasRenderTargets();
			if (Failed(res)) break;

			// dynamic upload buffer
			crntStageLogName = "dynamic upload buffer";
			res = this->grafSystem->CreateBuffer(this->grafDynamicUploadBuffer);
			if (Failed(res)) break;
			GrafBufferDesc uploadBufferDesc;
			uploadBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::TransferSrc;
			uploadBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			uploadBufferDesc.SizeInBytes = initParams.DynamicUploadBufferSize;
			res = this->grafDynamicUploadBuffer->Initialize(this->grafDevice.get(), { uploadBufferDesc });
			if (Failed(res)) break;
			this->uploadBufferAllocator.Init(uploadBufferDesc.SizeInBytes);

			// dynamic constant buffer
			crntStageLogName = "dynamic constant buffer";
			res = this->grafSystem->CreateBuffer(this->grafDynamicConstantBuffer);
			if (Failed(res)) break;
			GrafBufferDesc constantBufferDesc;
			constantBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::ConstantBuffer;
			constantBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			constantBufferDesc.SizeInBytes = initParams.DynamicConstantBufferSize;
			res = this->grafDynamicConstantBuffer->Initialize(this->grafDevice.get(), { constantBufferDesc });
			if (Failed(res)) break;
			this->constantBufferAllocator.Init(constantBufferDesc.SizeInBytes, this->grafDevice->GetPhysicalDeviceDesc()->ConstantBufferOffsetAlignment);

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

	Result GrafRenderer::Deinitialize()
	{
		// order matters!

		// make sure there are no resources still used on gpu before destroying
		if (this->grafDevice != ur_null)
		{
			this->grafDevice->WaitIdle();
		}

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
		// process pending callbacks
		this->ProcessPendingCommandListCallbacks();

		// present current swap chain image
		Result res = this->grafCanvas->Present();

		// move to next frame
		this->frameIdx = (this->frameIdx + 1) % this->frameCount;

		return res;
	}

	Result GrafRenderer::AddCommandListCallback(GrafCommandList *executionCmdList, GrafCallbackContext ctx, GrafCommandListCallback callback)
	{
		if (ur_null == executionCmdList)
			return Result(InvalidArgs);

		if (executionCmdList->Wait(0) == Success)
		{
			// command list is not in pending state, call back right now
			callback(ctx);
		}
		else
		{
			// command list fence is not signaled, add to pending list
			std::unique_ptr<PendingCommandListCallbackData> pendingCallback(new PendingCommandListCallbackData());
			pendingCallback->cmdList = executionCmdList;
			pendingCallback->callback = callback;
			pendingCallback->context = ctx;
			this->pendingCommandListCallbacks.push_back(std::move(pendingCallback));
		}

		return Result(Success);
	}

	Result GrafRenderer::ProcessPendingCommandListCallbacks()
	{
		Result res(Success);

		ur_size idx = 0;
		while (idx < this->pendingCommandListCallbacks.size())
		{
			auto& pendingCallback = this->pendingCommandListCallbacks[idx];
			if (pendingCallback->cmdList->Wait(0) == Success)
			{
				// command list is finished
				res &= pendingCallback->callback(pendingCallback->context);
				this->pendingCommandListCallbacks.erase(this->pendingCommandListCallbacks.begin() + idx);
			}
			else
			{
				// still pending
				++idx;
			}
		}

		return res;
	}

	Result GrafRenderer::Upload(ur_byte *dataPtr, ur_size dataSize, GrafImage* dstImage, GrafImageState dstImageState)
	{
		if (ur_null == dataPtr || 0 == dataSize || ur_null == dstImage)
			return Result(InvalidArgs);

		// allocate
		Allocation uploadBufferAlloc = this->uploadBufferAllocator.Allocate(dataSize);
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
			return ResultError(Failure, "ImguiRender::Init: failed to create upload command list");

		// copy dynamic buffer region to destination image
		grafUploadCmdList->Begin();
		grafUploadCmdList->ImageMemoryBarrier(dstImage, GrafImageState::Current, GrafImageState::TransferDst);
		grafUploadCmdList->Copy(srcBuffer, dstImage, srcOffset);
		grafUploadCmdList->ImageMemoryBarrier(dstImage, GrafImageState::Current, dstImageState);
		grafUploadCmdList->End();
		this->grafDevice->Submit(grafUploadCmdList.get());

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
		ImGui::SetNextWindowSize(windowSize, ImGuiSetCond_Once);
		ImGui::SetNextWindowPos({ canvasRect.Width() - windowSize.x, 0.0f }, ImGuiSetCond_Once);
		ImGui::Begin("GrafRenderer");
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
		if (ImGui::TreeNode("GrafSystem"))
		{
			ImGui::Text("Implementation: %s", typeid(*this->grafSystem.get()).name());
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::TreeNode("Devices Available:"))
			{
				for (ur_uint idevice = 0; idevice < this->grafSystem->GetPhysicalDeviceCount(); ++idevice)
				{
					const GrafPhysicalDeviceDesc* deviceDesc = this->grafSystem->GetPhysicalDeviceDesc(idevice);
					ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
					if (ImGui::TreeNode("PhysicalDeviceNode", "%s", deviceDesc->Description.c_str()))
					{
						ImGui::Text("Dedicated Memory (Mb): %u", deviceDesc->DedicatedVideoMemory / (1 << 20));
						ImGui::Text("Shared Memory (Mb): %u", deviceDesc->SharedSystemMemory / (1 << 20));
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			if (this->grafDevice != ur_null)
			{
				ImGui::Text("Device used: %s", this->grafDevice->GetPhysicalDeviceDesc()->Description.c_str());
			}
			if (this->grafCanvas != ur_null)
			{
				ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
				if (ImGui::TreeNode("Swap Chain:"))
				{
					ImGui::Text("Image count: %i", this->grafCanvas->GetSwapChainImageCount());
					if (this->grafCanvas->GetSwapChainImageCount() > 0)
					{
						const GrafImageDesc& imageDesc = this->grafCanvas->GetCurrentImage()->GetDesc();
						ImGui::Text("Image Size: %i x %i", imageDesc.Size.x, imageDesc.Size.y);
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (this->grafDynamicUploadBuffer)
		{
			static ur_size prevAllocatorOffset = 0;
			static ur_size allocatorOffsetDeltaMax = 0;
			ur_size crntAllocatorOffset = this->uploadBufferAllocator.GetOffset();
			ur_size allocatorOffsetDelta = crntAllocatorOffset + (crntAllocatorOffset < prevAllocatorOffset ? this->uploadBufferAllocator.GetSize() : 0) - prevAllocatorOffset;
			allocatorOffsetDeltaMax = std::max(allocatorOffsetDelta, allocatorOffsetDeltaMax);
			prevAllocatorOffset = crntAllocatorOffset;
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
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
			static ur_size prevAllocatorOffset = 0;
			static ur_size allocatorOffsetDeltaMax = 0;
			ur_size crntAllocatorOffset = this->constantBufferAllocator.GetOffset();
			ur_size allocatorOffsetDelta = crntAllocatorOffset + (crntAllocatorOffset < prevAllocatorOffset ? this->constantBufferAllocator.GetSize() : 0) - prevAllocatorOffset;
			allocatorOffsetDeltaMax = std::max(allocatorOffsetDelta, allocatorOffsetDeltaMax);
			prevAllocatorOffset = crntAllocatorOffset;
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::TreeNode("Dynamic Constant Buffer"))
			{
				ImGui::Text("Size: %i", this->grafDynamicConstantBuffer->GetDesc().SizeInBytes);
				ImGui::Text("Maximal usage per frame: %i", allocatorOffsetDeltaMax);
				ImGui::Text("Current frame usage: %i", allocatorOffsetDelta);
				ImGui::Text("Current ofs (bytes): %i", this->constantBufferAllocator.GetOffset());
				ImGui::TreePop();
			}
		}
		ImGui::Text("Recorded frame count: %i", this->frameCount);
		ImGui::End();

		return Result(Success);
	}

} // end namespace UnlimRealms