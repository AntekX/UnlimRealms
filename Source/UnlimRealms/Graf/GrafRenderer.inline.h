///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline GrafSystem* GrafRenderer::GetGrafSystem() const
	{
		return this->grafSystem.get();
	}

	inline GrafCanvas* GrafRenderer::GetGrafCanvas() const
	{
		return this->grafCanvas.get();
	}

	inline GrafDevice* GrafRenderer::GetGrafDevice() const
	{
		return this->grafDevice.get();
	}

	inline GrafRenderPass* GrafRenderer::GetCanvasRenderPass() const
	{
		return this->grafCanvasRenderPass.get();
	}

	inline GrafRenderTarget* GrafRenderer::GetCanvasRenderTarget() const
	{
		return this->grafCanvasRenderTarget[this->grafCanvas->GetCurrentImageId()].get();
	}

	/*inline GrafCommandList* GrafRenderer::GetCommandList(ur_uint frameId)
	{
		GrafCommandList* crntCmdList = ur_null;
		GetOrCreateCommandListForCurrentThread(crntCmdList, frameId);
		return crntCmdList;
	}*/

	inline ur_uint GrafRenderer::GetRecordedFrameCount() const
	{
		return this->frameCount;
	}

	inline ur_uint GrafRenderer::GetCurrentFrameId() const
	{
		return this->frameIdx;
	}
	
	inline ur_uint GrafRenderer::GetPrevFrameId() const
	{
		return (this->frameIdx > 0 ? this->frameIdx - 1 : this->frameCount - 1);
	}

	inline GrafBuffer* GrafRenderer::GetDynamicUploadBuffer() const
	{
		return this->grafDynamicUploadBuffer.get();
	}

	inline Allocation GrafRenderer::GetDynamicUploadBufferAllocation(ur_size size)
	{
		std::lock_guard<std::mutex> lock(this->uploadBufferMutex);
		return this->uploadBufferAllocator.Allocate(size);
	}

	inline GrafBuffer* GrafRenderer::GetDynamicConstantBuffer() const
	{
		return this->grafDynamicConstantBuffer.get();
	}

	inline Allocation GrafRenderer::GetDynamicConstantBufferAllocation(ur_size size)
	{
		std::lock_guard<std::mutex> lock(this->constantBufferMutex);
		return this->constantBufferAllocator.Allocate(size);
	}

} // end namespace UnlimRealmscs