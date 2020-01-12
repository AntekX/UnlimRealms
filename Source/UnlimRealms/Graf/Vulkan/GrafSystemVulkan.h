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

#include "Graf/GrafSystem.h"
#include "vulkan/vulkan.h"
#if defined(_WINDOWS)
#include "vulkan/vulkan_win32.h"
#endif
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

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateSampler(std::unique_ptr<GrafSampler>& grafSampler);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);

		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

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

		inline ur_uint GetVkDeviceGraphicsQueueId() const;

		inline ur_uint GetVkDeviceComputeQueueId() const;

		inline ur_uint GetVkDeviceTransferQueueId() const;

		struct UR_DECL ThreadCommandPool
		{
			VkCommandPool vkCommandPool;
			std::mutex accessMutex;
		};

		ThreadCommandPool* GetVkGraphicsCommandPool();

		ThreadCommandPool* GetVkComputeCommandPool();

		ThreadCommandPool* GetVkTransferCommandPool();

	private:

		Result Deinitialize();

		VkDevice vkDevice;
		VmaAllocator vmaAllocator;
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

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

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

		virtual Result Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

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

		virtual Result Write(ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		Result InitializeFromVkImage(GrafDevice *grafDevice, const InitParams& initParams, VkImage vkImage);

		inline VkImage GetVkImage() const;

		inline VkImageView GetVkImageView() const;

	private:

		Result Deinitialize();

		Result CreateVkImageViews();

		friend class GrafCommandListVulkan;
		inline void SetState(GrafImageState& state);

		ur_bool imageExternalHandle;
		VkImage vkImage;
		VkImageView vkImageView;
		VkDeviceMemory vkDeviceMemory;
		VkDeviceSize vkDeviceMemoryOffset;
		VkDeviceSize vkDeviceMemorySize;
		VkDeviceSize vkDeviceMemoryAlignment;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafBufferVulkan : public GrafBuffer
	{
	public:

		GrafBufferVulkan(GrafSystem &grafSystem);

		~GrafBufferVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline VkBuffer GetVkBuffer() const;

	private:

		Result Deinitialize();

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

		inline VkShaderModule GetVkShaderModule() const;

	private:

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

		virtual Result SetSampler(ur_uint bindingIdx, GrafSampler* sampler);

		virtual Result SetImage(ur_uint bindingIdx, GrafImage* image);

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
		static inline VkShaderStageFlagBits GrafToVkShaderStage(GrafShaderType shaderType);
		static inline VkShaderStageFlags GrafToVkShaderStage(GrafShaderStageFlags shaderStages);
		static inline VkDescriptorType GrafToVkDescriptorType(GrafDescriptorType descriptorType);
		static inline ur_uint32 GrafToVkDescriptorBindingOffset(GrafDescriptorType descriptorType);
		static inline VkPrimitiveTopology GrafToVkPrimitiveTopology(GrafPrimitiveTopology topology);
		static inline VkFrontFace GrafToVkFrontFace(GrafFrontFaceOrder frontFaceOrder);
		static inline VkCullModeFlags GrafToVkCullModeFlags(GrafCullMode cullMode);
		static inline VkCompareOp GrafToVkCompareOp(GrafCompareOp compareOp);
		static inline VkFilter GrafToVkFilter(GrafFilterType filter);
		static inline VkSamplerAddressMode GrafToVkAddressMode(GrafAddressMode address);
		static inline VkIndexType GrafToVkIndexType(GrafIndexType indexType);
		static inline VkAttachmentLoadOp GrafToVkLoadOp(GrafRenderPassDataOp dataOp);
		static inline VkAttachmentStoreOp GrafToVkStoreOp(GrafRenderPassDataOp dataOp);
		static inline VkFormat GrafToVkFormat(GrafFormat grafFormat);
		static inline GrafFormat VkToGrafFormat(VkFormat vkFormat);
	};

} // end namespace UnlimRealms

#include "GrafSystemVulkan.inline.h"