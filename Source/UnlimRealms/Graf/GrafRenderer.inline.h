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

	inline ur_uint GrafRenderer::GetRecordedFrameCount() const
	{
		return this->frameCount;
	}

	inline ur_uint GrafRenderer::GetCurrentFrameId() const
	{
		return this->frameIdx;
	}

	inline GrafBuffer* GrafRenderer::GetDynamicUploadBuffer() const
	{
		return this->grafDynamicUploadBuffer.get();
	}

	inline GrafBuffer* GrafRenderer::GetDynamicConstantBuffer() const
	{
		return this->grafDynamicConstantBuffer.get();
	}

	inline Allocation GrafRenderer::GetDynamicConstantBufferAllocation(ur_size size)
	{
		return this->constantBufferAllocator.Allocate(size);
	}

} // end namespace UnlimRealmscs