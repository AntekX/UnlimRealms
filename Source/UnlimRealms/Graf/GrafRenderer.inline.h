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

	inline GrafCommandList* GrafRenderer::GetTransientCommandList()
	{
		GrafCommandList* crntCmdList = ur_null;
		this->GetOrCreateCommandListForCurrentThread(crntCmdList);
		return crntCmdList;
	}

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

	inline GrafRenderer& GrafRendererEntity::GetGrafRenderer() const
	{
		return this->grafRenderer;
	}

	template <class TGrafObject>
	GrafRendererManagedEntity<TGrafObject>::GrafRendererManagedEntity(GrafRenderer& grafRenderer) :
		GrafRendererEntity(grafRenderer)
	{
	}

	template <class TGrafObject>
	GrafRendererManagedEntity<TGrafObject>::~GrafRendererManagedEntity()
	{
		this->Deinitialize();
	}

	template <class TGrafObject>
	Result GrafRendererManagedEntity<TGrafObject>::Initialize()
	{
		this->grafObjectPerFrame.resize(this->GetGrafRenderer().GetRecordedFrameCount());
		return Result(Success);
	}

	template <class TGrafObject>
	Result GrafRendererManagedEntity<TGrafObject>::Deinitialize()
	{
		this->grafObjectPerFrame.clear();
		return Result(Success);
	}

	template <class TGrafObject>
	inline TGrafObject* GrafRendererManagedEntity<TGrafObject>::GetFrameObject(ur_uint frameId) const
	{
		if (frameId >= this->GetGrafRenderer().GetRecordedFrameCount())
			frameId = this->GetGrafRenderer().GetCurrentFrameId();
		return (frameId < this->grafObjectPerFrame.size() ? static_cast<TGrafObject*>(this->grafObjectPerFrame[frameId].get()) : ur_null);
	}

} // end namespace UnlimRealmscs