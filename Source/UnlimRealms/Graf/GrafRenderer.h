///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF Based Renderer
// Provides higher level of control over rendering pipeline
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GrafSystem.h"
#include "Core/Memory.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafCallbackContext
	{
		void *DataPtr;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<Result(GrafCallbackContext& ctx)> GrafCommandListCallback;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderer : public RealmEntity
	{
	public:

		struct UR_DECL InitParams
		{
			ur_uint DeviceId;
			GrafCanvas::InitParams CanvasParams;
			ur_uint DynamicUploadBufferSize;
			ur_uint DynamicConstantBufferSize;

			static const InitParams Default;
			static const ur_uint RecommendedDeviceId;
		};

		GrafRenderer(Realm &realm);

		virtual ~GrafRenderer();

		virtual Result Initialize(std::unique_ptr<GrafSystem>& grafSystem, const InitParams& initParams);

		virtual Result Deinitialize();

		virtual Result BeginFrame();

		virtual Result EndFrameAndPresent();

		virtual Result AddCommandListCallback(GrafCommandList *executionCmdList, GrafCallbackContext ctx, GrafCommandListCallback callback);

		virtual Result Upload(ur_byte *dataPtr, ur_size dataSize, GrafImage* dstImage, GrafImageState dstImageState);

		virtual Result Upload(GrafBuffer *srcBuffer, ur_size srcOffset, GrafImage* dstImage, GrafImageState dstImageState);

		virtual Result ShowImgui();

		inline GrafSystem* GetGrafSystem() const;

		inline GrafCanvas* GetGrafCanvas() const;

		inline GrafDevice* GetGrafDevice() const;

		inline GrafRenderPass* GetCanvasRenderPass() const;

		inline GrafRenderTarget* GetCanvasRenderTarget() const;

		inline ur_uint GetRecordedFrameCount() const;

		inline ur_uint GetCurrentFrameId() const;

		inline ur_uint GetPrevFrameId() const;

		inline GrafBuffer* GetDynamicUploadBuffer() const;

		inline GrafBuffer* GetDynamicConstantBuffer() const;

		inline Allocation GetDynamicConstantBufferAllocation(ur_size size);

	protected:

		struct UR_DECL PendingCommandListCallbackData
		{
			GrafCommandList *cmdList;
			GrafCommandListCallback callback;
			GrafCallbackContext context;
		};

		Result InitializeCanvasRenderTargets();

		Result ProcessPendingCommandListCallbacks();

		std::unique_ptr<GrafSystem> grafSystem;
		std::unique_ptr<GrafDevice> grafDevice;
		std::unique_ptr<GrafCanvas> grafCanvas;
		std::unique_ptr<GrafRenderPass> grafCanvasRenderPass;
		std::vector<std::unique_ptr<GrafRenderTarget>> grafCanvasRenderTarget;
		std::unique_ptr<GrafBuffer> grafDynamicUploadBuffer;
		LinearAllocator uploadBufferAllocator;
		std::unique_ptr<GrafBuffer> grafDynamicConstantBuffer;
		LinearAllocator constantBufferAllocator;
		ur_uint frameCount;
		ur_uint frameIdx;
		GrafCanvas::InitParams grafCanvasParams;
		std::vector<std::unique_ptr<PendingCommandListCallbackData>> pendingCommandListCallbacks;
	};

} // end namespace UnlimRealms

#include "GrafRenderer.inline.h"