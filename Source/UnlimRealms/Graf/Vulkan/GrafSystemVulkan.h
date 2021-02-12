///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: VULKAN IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define UR_GRAF_VULKAN_RAY_TRACING_KHR 0 // enables KHR ray tracing support
#define UR_GRAF_VULKAN_RAY_TRACING_KHR_PROV 0 // TEMP: enbale provisionary version
#if (UR_GRAF_VULKAN_RAY_TRACING_KHR_PROV)
#define VK_ENABLE_BETA_EXTENSIONS
#endif

#include "Graf/GrafSystem.h"
#include "vulkan/vulkan.h"
#include "3rdParty/VulkanMemoryAllocator/vk_mem_alloc.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSystemVulkan : public GrafSystem
	{
	public:

		GrafSystemVulkan(Realm &realm);

		virtual ~GrafSystemVulkan();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList);

		virtual Result CreateFence(std::unique_ptr<GrafFence>& grafFence);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);

		virtual Result CreateImage(std::unique_ptr<GrafImage>& grafImage);

		virtual Result CreateImageSubresource(std::unique_ptr<GrafImageSubresource>& grafImageSubresource);

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateSampler(std::unique_ptr<GrafSampler>& grafSampler);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateShaderLib(std::unique_ptr<GrafShaderLib>& grafShaderLib);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);

		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		virtual Result CreateComputePipeline(std::unique_ptr<GrafComputePipeline>& grafComputePipeline);

		virtual Result CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline);

		virtual Result CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct);

		inline VkInstance GetVkInstance() const;

		inline VkPhysicalDevice GetVkPhysicalDevice(ur_uint deviceId) const;

	private:

		Result Deinitialize();
		
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT				messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
			void*										pUserData);

		VkInstance vkInstance;
		std::vector<VkPhysicalDevice> vkPhysicalDevices;
		VkDebugUtilsMessengerEXT vkDebugUtilsMessenger;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDeviceVulkan : public GrafDevice
	{
	public:

		GrafDeviceVulkan(GrafSystem &grafSystem);

		~GrafDeviceVulkan();

		virtual Result Initialize(ur_uint deviceId);

		virtual Result Record(GrafCommandList* grafCommandList);

		virtual Result Submit();

		virtual Result WaitIdle();

		inline VkDevice GetVkDevice() const;

		inline VmaAllocator GetVmaAllocator() const;

		inline VkDescriptorPool GetVkDescriptorPool() const;

		inline ur_uint GetVkDeviceGraphicsQueueId() const;

		inline ur_uint GetVkDeviceComputeQueueId() const;

		inline ur_uint GetVkDeviceTransferQueueId() const;

		struct UR_DECL ThreadCommandPool
		{
			VkCommandPool vkCommandPool;
			std::recursive_mutex accessMutex;
		};

		ThreadCommandPool* GetVkGraphicsCommandPool();

		ThreadCommandPool* GetVkComputeCommandPool();

		ThreadCommandPool* GetVkTransferCommandPool();

	private:

		Result Deinitialize();

		VkDevice vkDevice;
		VmaAllocator vmaAllocator;
		VkDescriptorPool vkDescriptorPool;
		ur_uint deviceGraphicsQueueId;
		ur_uint deviceComputeQueueId;
		ur_uint deviceTransferQueueId;
		std::unordered_map<std::thread::id, std::unique_ptr<ThreadCommandPool>> graphicsCommandPools;
		std::unordered_map<std::thread::id, std::unique_ptr<ThreadCommandPool>> computeCommandPools;
		std::unordered_map<std::thread::id, std::unique_ptr<ThreadCommandPool>> transferCommandPools;
		std::mutex graphicsCommandPoolsMutex;
		std::mutex computeCommandPoolsMutex;
		std::mutex transferCommandPoolsMutex;
		std::vector<GrafCommandList*> graphicsCommandLists;
		std::mutex graphicsCommandListsMutex;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCommandListVulkan : public GrafCommandList
	{
	public:

		GrafCommandListVulkan(GrafSystem &grafSystem);

		~GrafCommandListVulkan();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result Begin();

		virtual Result End();

		virtual Result Wait(ur_uint64 timeout = ur_uint64(-1));

		virtual Result InsertDebugLabel(const char* name, const ur_float4& color = ur_float4::Zero);

		virtual Result BeginDebugLabel(const char* name, const ur_float4& color = ur_float4::Zero);

		virtual Result EndDebugLabel();

		virtual Result BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState);

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result ImageMemoryBarrier(GrafImageSubresource* grafImageSubresource, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result ClearColorImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue);

		virtual Result ClearDepthStencilImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result ClearDepthStencilImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue);

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues = ur_null);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset = 0);

		virtual Result BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset = 0);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImageSubresource* dstImageSubresource, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImageSubresource* srcImageSubresource, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result Copy(GrafImageSubresource* srcImageSubresource, GrafImageSubresource* dstImageSubresource, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result BindComputePipeline(GrafComputePipeline* grafPipeline);

		virtual Result BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline);

		virtual Result Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ);

		virtual Result BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount);

		virtual Result BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline);

		virtual Result BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline);

		virtual Result DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
			const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
			const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable);

		inline VkCommandBuffer GetVkCommandBuffer() const;

		inline VkFence GetVkSubmitFence() const;

	private:

		Result Deinitialize();

		GrafDeviceVulkan::ThreadCommandPool* commandPool;
		VkCommandBuffer vkCommandBuffer;
		VkFence vkSubmitFence;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafFenceVulkan : public GrafFence
	{
	public:

		GrafFenceVulkan(GrafSystem &grafSystem);

		~GrafFenceVulkan();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result SetState(GrafFenceState state);

		virtual Result GetState(GrafFenceState& state);

		virtual Result WaitSignaled();

		inline VkEvent GetVkEvent() const;

	private:

		Result Deinitialize();

		VkEvent vkEvent;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCanvasVulkan : public GrafCanvas
	{
	public:

		GrafCanvasVulkan(GrafSystem &grafSystem);

		~GrafCanvasVulkan();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

		virtual Result Present();

		virtual GrafImage* GetCurrentImage();

		virtual GrafImage* GetSwapChainImage(ur_uint imageId);

	private:

		Result Deinitialize();

		Result AcquireNextImage();

		ur_uint32 vkDevicePresentQueueId;
		VkSurfaceKHR vkSurface;
		VkSwapchainKHR vkSwapChain;
		std::vector<std::unique_ptr<GrafImage>> swapChainImages;

		// per frame data
		ur_uint32 frameCount;
		ur_uint32 frameIdx;
		std::vector<std::unique_ptr<GrafCommandList>> imageTransitionCmdListBegin;
		std::vector<std::unique_ptr<GrafCommandList>> imageTransitionCmdListEnd;
		std::vector<VkSemaphore> vkSemaphoreImageAcquired;
		std::vector<VkSemaphore> vkSemaphoreRenderFinished;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImageVulkan : public GrafImage
	{
	public:

		GrafImageVulkan(GrafSystem &grafSystem);

		~GrafImageVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		Result InitializeFromVkImage(GrafDevice *grafDevice, const InitParams& initParams, VkImage vkImage);

		inline VkImage GetVkImage() const;

		inline VkImageView GetVkImageView() const;

		inline GrafImageSubresource* GetDefaultSubresource() const;

	private:

		Result Deinitialize();

		Result CreateDefaultSubresource();

		friend class GrafCommandListVulkan;
		inline void SetState(GrafImageState& state);

		ur_bool imageExternalHandle;
		VkImage vkImage;
		VkDeviceMemory vkDeviceMemory;
		VkDeviceSize vkDeviceMemoryOffset;
		VkDeviceSize vkDeviceMemorySize;
		VkDeviceSize vkDeviceMemoryAlignment;
		VmaAllocation vmaAllocation;
		std::unique_ptr<GrafImageSubresource> grafDefaultSubresource;
		VkImageView vkImageView;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImageSubresourceVulkan : public GrafImageSubresource
	{
	public:

		GrafImageSubresourceVulkan(GrafSystem &grafSystem);

		~GrafImageSubresourceVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkImageView GetVkImageView() const;

	protected:

		Result Deinitialize();

		Result CreateVkImageView();

		friend class GrafCommandListVulkan;
		inline void SetState(GrafImageState& state);

		VkImageView vkImageView;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafBufferVulkan : public GrafBuffer
	{
	public:

		GrafBufferVulkan(GrafSystem &grafSystem);

		~GrafBufferVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline VkBuffer GetVkBuffer() const;

	private:

		Result Deinitialize();

		friend class GrafCommandListVulkan;
		inline void SetState(GrafBufferState& state);

		VkBuffer vkBuffer;
		VkDeviceMemory vkDeviceMemory;
		VkDeviceSize vkDeviceMemoryOffset;
		VkDeviceSize vkDeviceMemorySize;
		VkDeviceSize vkDeviceMemoryAlignment;
		VmaAllocation vmaAllocation;
		void *mappedMemoryDataPtr;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSamplerVulkan : public GrafSampler
	{
	public:

		GrafSamplerVulkan(GrafSystem &grafSystem);

		~GrafSamplerVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkSampler GetVkSampler() const;

	protected:

		Result Deinitialize();

		VkSampler vkSampler;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShaderVulkan : public GrafShader
	{
	public:

		GrafShaderVulkan(GrafSystem &grafSystem);

		~GrafShaderVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		Result InitializeFromVkShaderModule(GrafDevice *grafDevice, const InitParams& initParams, VkShaderModule vkShaderModule);

		inline VkShaderModule GetVkShaderModule() const;

	private:

		Result Deinitialize();

		VkShaderModule vkShaderModule;
		ur_bool moduleOwner;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShaderLibVulkan : public GrafShaderLib
	{
	public:

		GrafShaderLibVulkan(GrafSystem &grafSystem);

		~GrafShaderLibVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	protected:

		Result Deinitialize();

		VkShaderModule vkShaderModule;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderPassVulkan : public GrafRenderPass
	{
	public:

		GrafRenderPassVulkan(GrafSystem& grafSystem);

		~GrafRenderPassVulkan();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

		inline VkRenderPass GetVkRenderPass() const;

	private:

		Result Deinitialize();

		VkRenderPass vkRenderPass;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderTargetVulkan : public GrafRenderTarget
	{
	public:

		GrafRenderTargetVulkan(GrafSystem &grafSystem);

		~GrafRenderTargetVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkFramebuffer GetVkFramebuffer() const;

	private:

		Result Deinitialize();

		VkFramebuffer vkFramebuffer;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableLayoutVulkan : public GrafDescriptorTableLayout
	{
	public:

		GrafDescriptorTableLayoutVulkan(GrafSystem &grafSystem);

		~GrafDescriptorTableLayoutVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkDescriptorSetLayout GetVkDescriptorSetLayout() const;

	private:

		Result Deinitialize();

		VkDescriptorSetLayout vkDescriptorSetLayout;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableVulkan : public GrafDescriptorTable
	{
	public:

		GrafDescriptorTableVulkan(GrafSystem &grafSystem);

		~GrafDescriptorTableVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs = 0, ur_size bufferRange = 0);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource, GrafSampler* sampler);

		virtual Result SetSampler(ur_uint bindingIdx, GrafSampler* sampler);

		virtual Result SetImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource);

		virtual Result SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource);

		virtual Result SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure);

		inline VkDescriptorSet GetVkDescriptorSet() const;

	private:

		Result Deinitialize();

		VkDescriptorSet vkDescriptorSet;
		VkDescriptorPool vkDescriptorPool;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafPipelineVulkan : public GrafPipeline
	{
	public:

		GrafPipelineVulkan(GrafSystem &grafSystem);

		~GrafPipelineVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkPipeline GetVkPipeline() const;

		inline VkPipelineLayout GetVkPipelineLayout() const;

	protected:

		Result Deinitialize();

		VkPipeline vkPipeline;
		VkPipelineLayout vkPipelineLayout;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafComputePipelineVulkan : public GrafComputePipeline
	{
	public:

		GrafComputePipelineVulkan(GrafSystem &grafSystem);

		~GrafComputePipelineVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkPipeline GetVkPipeline() const;

		inline VkPipelineLayout GetVkPipelineLayout() const;

	protected:

		Result Deinitialize();

		VkPipeline vkPipeline;
		VkPipelineLayout vkPipelineLayout;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRayTracingPipelineVulkan : public GrafRayTracingPipeline
	{
	public:

		GrafRayTracingPipelineVulkan(GrafSystem &grafSystem);

		~GrafRayTracingPipelineVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr);

		inline VkPipeline GetVkPipeline() const;

		inline VkPipelineLayout GetVkPipelineLayout() const;

	protected:

		Result Deinitialize();

		VkPipeline vkPipeline;
		VkPipelineLayout vkPipelineLayout;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafAccelerationStructureVulkan : public GrafAccelerationStructure
	{
	public:

		GrafAccelerationStructureVulkan(GrafSystem &grafSystem);

		~GrafAccelerationStructureVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		inline VkAccelerationStructureKHR GetVkAccelerationStructure() const;
	#endif

		inline GrafBuffer* GetScratchBuffer() const;

	protected:

		Result Deinitialize();

	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		VkAccelerationStructureKHR vkAccelerationStructure;
	#endif
		VmaAllocation vmaAllocation;
		VmaAllocationInfo vmaAllocationInfo;
		std::unique_ptr<GrafBuffer> grafScratchBuffer;
		std::unique_ptr<GrafBuffer> grafStorageBuffer;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafUtilsVulkan
	{
	public:

		static inline VkImageUsageFlags GrafToVkImageUsage(GrafImageUsageFlags usage);
		static inline GrafImageUsageFlags VkToGrafImageUsage(VkImageUsageFlags usage);
		static inline VkImageType GrafToVkImageType(GrafImageType imageType);
		static inline VkImageAspectFlags GrafToVkImageUsageAspect(GrafImageUsageFlags usage);
		static inline VkImageLayout GrafToVkImageLayout(GrafImageState imageState);
		static inline VkBufferUsageFlags GrafToVkBufferUsage(GrafBufferUsageFlags usage);
		static inline VkMemoryPropertyFlags GrafToVkMemoryProperties(GrafDeviceMemoryFlags memoryType);
		static inline VkShaderStageFlags GrafToVkShaderStage(GrafShaderStageFlags shaderStages);
		static inline VkShaderStageFlagBits GrafToVkShaderStage(GrafShaderType shaderType);
		static inline VkDescriptorType GrafToVkDescriptorType(GrafDescriptorType descriptorType);
		static inline ur_uint32 GrafToVkDescriptorBindingOffset(GrafDescriptorType descriptorType);
		static inline VkPrimitiveTopology GrafToVkPrimitiveTopology(GrafPrimitiveTopology topology);
		static inline VkFrontFace GrafToVkFrontFace(GrafFrontFaceOrder frontFaceOrder);
		static inline VkCullModeFlags GrafToVkCullModeFlags(GrafCullMode cullMode);
		static inline VkCompareOp GrafToVkCompareOp(GrafCompareOp compareOp);
		static inline VkStencilOp GrafToVkStencilOp(GrafStencilOp stencilOp);
		static inline VkBlendOp GrafToVkBlendOp(GrafBlendOp blendOp);
		static inline VkBlendFactor GrafToVkBlendFactor(GrafBlendFactor blendFactor);
		static inline VkFilter GrafToVkFilter(GrafFilterType filter);
		static inline VkSamplerAddressMode GrafToVkAddressMode(GrafAddressMode address);
		static inline VkIndexType GrafToVkIndexType(GrafIndexType indexType);
		static inline VkAttachmentLoadOp GrafToVkLoadOp(GrafRenderPassDataOp dataOp);
		static inline VkAttachmentStoreOp GrafToVkStoreOp(GrafRenderPassDataOp dataOp);
		static inline VkFormat GrafToVkFormat(GrafFormat grafFormat);
		static inline GrafFormat VkToGrafFormat(VkFormat vkFormat);
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		static inline VkRayTracingShaderGroupTypeKHR GrafToVkRayTracingShaderGroupType(GrafRayTracingShaderGroupType shaderGroupType);
		static inline VkGeometryTypeKHR GrafToVkAccelerationStructureGeometryType(GrafAccelerationStructureGeometryType geometryType);
		static inline VkGeometryFlagsKHR GrafToVkAccelerationStructureGeometryFlags(GrafAccelerationStructureGeometryFlags geometryFlags);
		static inline VkAccelerationStructureTypeKHR GrafToVkAccelerationStructureType(GrafAccelerationStructureType structureType);
		static inline VkBuildAccelerationStructureFlagsKHR GrafToVkAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlags buildFlags);
		#endif
	};

} // end namespace UnlimRealms

#include "GrafSystemVulkan.inline.h"