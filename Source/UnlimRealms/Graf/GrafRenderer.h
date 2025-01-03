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
	class Job;

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

		static const ur_uint CurrentFrameId = ur_uint(-1);

		GrafRenderer(Realm &realm);

		virtual ~GrafRenderer();

		virtual Result Initialize(std::unique_ptr<GrafSystem> grafSystem, const InitParams& initParams);

		virtual Result Deinitialize();

		virtual Result BeginFrame();

		virtual Result EndFrameAndPresent();

		virtual Result AddCommandListCallback(GrafCommandList *executionCmdList, GrafCallbackContext ctx, GrafCommandListCallback callback);

		virtual Result SafeDelete(GrafEntity* grafEnity, GrafCommandList *grafSyncCmdList = ur_null, ur_bool deleteCmdList = false);

		virtual Result Upload(ur_byte *dataPtr, GrafBuffer* dstBuffer, ur_size dataSize, ur_size dstOffset = 0);

		virtual Result Upload(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Upload(ur_byte *dataPtr, ur_size dataSize, GrafImage* dstImage, GrafImageState dstImageState);

		virtual Result Upload(GrafBuffer *srcBuffer, ur_size srcOffset, GrafImage* dstImage, GrafImageState dstImageState);

		virtual Result DisplayImgui();

		inline GrafSystem* GetGrafSystem() const;

		inline GrafCanvas* GetGrafCanvas() const;

		inline GrafDevice* GetGrafDevice() const;

		inline GrafRenderPass* GetCanvasRenderPass() const;

		inline GrafRenderTarget* GetCanvasRenderTarget() const;

		inline GrafCommandList* GetTransientCommandList();
		
		inline ur_uint64 GetFrameIdx() const;

		inline ur_uint GetRecordedFrameCount() const;

		inline ur_uint GetRecordedFrameIdx() const;

		inline ur_uint GetPrevRecordedFrameIdx() const;

		inline GrafBuffer* GetDynamicUploadBuffer() const;

		inline Allocation GetDynamicUploadBufferAllocation(ur_size size);

		inline GrafBuffer* GetDynamicConstantBuffer() const;

		inline Allocation GetDynamicConstantBufferAllocation(ur_size size);

	private:

		struct UR_DECL PendingCommandListCallbackData
		{
			GrafCommandList *cmdList;
			GrafCommandListCallback callback;
			GrafCallbackContext context;
		};

		struct UR_DECL CommandListCache
		{
			std::mutex cmdListsMutex;
			std::list<std::unique_ptr<GrafCommandList>> availableCmdLists;
			std::list<std::unique_ptr<GrafCommandList>> acquiredCmdLists;
		};

		Result InitializeCanvasRenderTargets();

		Result GetOrCreateCommandListForCurrentThread(GrafCommandList*& grafCommandList);

		Result UpdateCommandListCache(ur_bool forceWait);

		Result ProcessPendingCommandListCallbacks(ur_bool immediateMode);

		GrafCanvas::InitParams grafCanvasParams;
		std::unique_ptr<GrafSystem> grafSystem;
		std::unique_ptr<GrafDevice> grafDevice;
		std::unique_ptr<GrafCanvas> grafCanvas;
		std::unique_ptr<GrafRenderPass> grafCanvasRenderPass;
		std::vector<std::unique_ptr<GrafRenderTarget>> grafCanvasRenderTarget;
		std::unique_ptr<GrafBuffer> grafDynamicUploadBuffer;
		LinearAllocator uploadBufferAllocator;
		std::mutex uploadBufferMutex;
		std::unique_ptr<GrafBuffer> grafDynamicConstantBuffer;
		LinearAllocator constantBufferAllocator;
		std::mutex constantBufferMutex;
		ur_uint64 rendererFrameIdx;
		ur_uint recordedFrameIdx;
		ur_uint recordedFrameCount;
		std::map<std::thread::id, std::unique_ptr<CommandListCache>> grafCommandListCache;
		std::mutex grafCommandListCacheMutex;
		std::vector<std::unique_ptr<PendingCommandListCallbackData>> pendingCommandListCallbacks;
		std::vector<std::unique_ptr<PendingCommandListCallbackData>> finishedCommandListCallbacks;
		std::shared_ptr<Job> finishedCommandListCallbacksJob;
		//std::shared_ptr<Job> processingCommandListCallbacksJob;
		//ur_uint pendingCommandListCallbacksIdx;
		std::mutex pendingCommandListMutex;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRendererEntity : public RealmEntity
	{
	public:

		GrafRendererEntity(GrafRenderer& grafRenderer);

		virtual ~GrafRendererEntity();

		inline GrafRenderer& GetGrafRenderer() const;

	private:

		GrafRenderer& grafRenderer;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class TGrafObject>
	class GrafRendererManagedEntity : public GrafRendererEntity
	{
	public:

		GrafRendererManagedEntity(GrafRenderer& grafRenderer);

		virtual ~GrafRendererManagedEntity();

		Result Initialize();

		Result Deinitialize();

		inline TGrafObject* GetFrameObject(ur_uint frameId = ur_uint(-1)) const;

	protected:

		std::vector<std::unique_ptr<TGrafObject>> grafObjectPerFrame;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafManagedCommandList : public GrafRendererManagedEntity<GrafCommandList>
	{
	public:

		GrafManagedCommandList(GrafRenderer& grafRenderer);

		virtual ~GrafManagedCommandList();

		Result Initialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafManagedDescriptorTable : public GrafRendererManagedEntity<GrafDescriptorTable>
	{
	public:

		GrafManagedDescriptorTable(GrafRenderer& grafRenderer);

		virtual ~GrafManagedDescriptorTable();

		Result Initialize(const GrafDescriptorTable::InitParams& initParams);
	};

} // end namespace UnlimRealms

#include "GrafRenderer.inline.h"