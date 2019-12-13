#pragma once

class VulkanSandboxApp
{
public:

	int Run();
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NEW GFX ABSTRACTION LAYER WIP
// GRAF: GRaphics Abstraction Front-end
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "UnlimRealms.h"

namespace UnlimRealms
{

	// forward declarations
	class GrafSystem;
	class GrafDevice;
	class GrafCommandList;
	class GrafFence;
	class GrafCanvas;
	class GrafImage;
	class GrafShader;
	class GrafRenderPass;
	class GrafPipeline;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafPhysicalDeviceDesc
	{
		std::string Description;
		ur_uint VendorId;
		ur_uint DeviceId;
		ur_size DedicatedVideoMemory;
		ur_size DedicatedSystemMemory;
		ur_size SharedSystemMemory;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafPresentMode
	{
		Immediate = 0,
		VerticalSync,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*enum GrafDeviceTypeFlags
	{
		GrafDeviceTypeFlag_Undefined = 0,
		GrafDeviceTypeFlag_Graphics = (1 << 0),
		GrafDeviceTypeFlag_Compute = (1 << 1),
		GrafDeviceTypeFlag_Transfer = (1 << 2)
	};*/

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafFenceState
	{
		Undefined = -1,
		Reset,
		Signaled
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafFormat
	{
		Unsupported = -1,
		Undefined = 0,
		R8G8B8_UNORM,
		R8G8B8_SNORM,
		R8G8B8_UINT,
		R8G8B8_SINT,
		R8G8B8_SRGB,
		B8G8R8_UNORM,
		B8G8R8_SNORM,
		B8G8R8_UINT,
		B8G8R8_SINT,
		B8G8R8_SRGB,
		R8G8B8A8_UNORM,
		R8G8B8A8_SNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SINT,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,
		B8G8R8A8_SNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SINT,
		B8G8R8A8_SRGB,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafImageType
	{
		Undefined = 0,
		Tex1D,
		Tex2D,
		Tex3D,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafImageUsageFlag
	{
		Undefined = 0,
		TransferSrc = (1 << 0),
		TransferDst = (1 << 1),
		ColorRenderTarget = (1 << 2),
		DepthStencilRenderTarget = (1 << 3)
	};
	typedef ur_uint GrafImageUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafImageState
	{
		Current = -1,
		Undefined = 0,
		Common,
		ColorWrite,
		DepthStencilWrite,
		DepthStencilRead,
		ShaderRead,
		TransferSrc,
		TransferDst,
		Present
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GrafImageDesc
	{
		GrafImageType Type;
		GrafFormat Format;
		ur_uint3 Size;
		ur_uint MipLevels;
		GrafImageUsageFlags Usage;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GrafClearValue
	{
		union
		{
			ur_float f32[4];
			ur_int i32[4];
			ur_uint u32[4];
		};
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafShaderType
	{
		Undefined = -1,
		Vertex,
		Pixel,
		Compute,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafSystem : public RealmEntity
	{
	public:

		GrafSystem(Realm &realm);

		virtual ~GrafSystem();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList);

		virtual Result CreateFence(std::unique_ptr<GrafFence>& grafFence);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);
		
		virtual Result CreateImage(std::unique_ptr<GrafImage>& grafImage);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);
		
		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		virtual ur_uint GetRecommendedDeviceId();

		inline ur_uint GetPhysicalDeviceCount();

		inline const GrafPhysicalDeviceDesc* GetPhysicalDeviceDesc(ur_uint deviceId);

	protected:

		std::vector<GrafPhysicalDeviceDesc> grafPhysicalDeviceDesc;
	};

	inline ur_uint GrafSystem::GetPhysicalDeviceCount()
	{
		return (ur_uint)grafPhysicalDeviceDesc.size();
	}

	inline const GrafPhysicalDeviceDesc* GrafSystem::GetPhysicalDeviceDesc(ur_uint deviceId)
	{
		return (deviceId < GetPhysicalDeviceCount() ? &grafPhysicalDeviceDesc[deviceId] : ur_null);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafEntity : public RealmEntity
	{
	public:

		GrafEntity(GrafSystem &grafSystem);

		~GrafEntity();

		inline GrafSystem& GetGrafSystem();

	private:

		GrafSystem &grafSystem;
	};

	inline GrafSystem& GrafEntity::GetGrafSystem()
	{
		return this->grafSystem;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDevice : public GrafEntity
	{
	public:

		GrafDevice(GrafSystem &grafSystem);

		~GrafDevice();

		virtual Result Initialize(ur_uint deviceId);

		virtual Result Submit(GrafCommandList* grafCommandList);

		virtual Result WaitIdle();

		inline ur_uint GetDeviceId();

		inline const GrafPhysicalDeviceDesc* GetPhysicalDeviceDesc(ur_uint deviceId);

	private:

		ur_uint deviceId;
	};

	inline ur_uint GrafDevice::GetDeviceId()
	{
		return deviceId;
	}

	inline const GrafPhysicalDeviceDesc* GrafDevice::GetPhysicalDeviceDesc(ur_uint deviceId)
	{
		return this->GetGrafSystem().GetPhysicalDeviceDesc(this->deviceId);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDeviceEntity : public GrafEntity
	{
	public:

		GrafDeviceEntity(GrafSystem &grafSystem);

		~GrafDeviceEntity();

		virtual Result Initialize(GrafDevice *grafDevice);

		inline GrafDevice* GetGrafDevice();

	private:

		GrafDevice *grafDevice;
	};

	inline GrafDevice* GrafDeviceEntity::GetGrafDevice()
	{
		return this->grafDevice;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafCommandList : public GrafDeviceEntity
	{
	public:

		GrafCommandList(GrafSystem &grafSystem);

		~GrafCommandList();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result Begin();

		virtual Result End();

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);
		
		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafFence : public GrafDeviceEntity
	{
	public:

		GrafFence(GrafSystem &grafSystem);

		~GrafFence();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result SetState(GrafFenceState state);

		virtual Result GetState(GrafFenceState& state);

		virtual Result WaitSignaled();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafCanvas : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafFormat Format;
			GrafPresentMode PresentMode;
			ur_uint SwapChainImageCount;
			static const InitParams Default;
		};

		GrafCanvas(GrafSystem &grafSystem);

		~GrafCanvas();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams = InitParams::Default);

		virtual Result Present();

		virtual GrafImage* GetCurrentImage();

		virtual GrafImage* GetSwapChainImage(ur_uint imageId);

		inline ur_uint GetSwapChainImageCount() const;

	protected:

		ur_uint swapChainImageCount;
	};

	inline ur_uint GrafCanvas::GetSwapChainImageCount() const
	{
		return this->swapChainImageCount;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafImage : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafImageDesc ImageDesc;
		};

		GrafImage(GrafSystem &grafSystem);

		~GrafImage();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafImageDesc& GetDesc() const;

		inline const GrafImageState& GetState() const;

	protected:

		GrafImageDesc imageDesc;
		GrafImageState imageState;
	};

	inline const GrafImageDesc& GrafImage::GetDesc() const
	{
		return this->imageDesc;
	}

	inline const GrafImageState& GrafImage::GetState() const
	{
		return this->imageState;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafShader : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafShaderType ShaderType;
			ur_byte* ByteCode;
			ur_size ByteCodeSize;
			const char* EntryPoint;
		};
		static const char* DefaultEntryPoint;

		GrafShader(GrafSystem &grafSystem);

		~GrafShader();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafShaderType& GetShaderType() const;

		inline const std::string& GetEntryPoint() const;

	protected:

		GrafShaderType shaderType;
		std::string entryPoint;
	};

	inline const GrafShaderType& GrafShader::GetShaderType() const
	{
		return this->shaderType;
	}

	inline const std::string& GrafShader::GetEntryPoint() const
	{
		return this->entryPoint;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafRenderPass : public GrafDeviceEntity
	{
	public:

		GrafRenderPass(GrafSystem& grafSystem);

		~GrafRenderPass();

		virtual Result Initialize(GrafDevice* grafDevice);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafPipeline : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			std::vector<GrafShader*> ShaderStages;
			GrafImage* RenderTarget; // TEMP: passing render target directly for test, render spass should be used instead
		};

		GrafPipeline(GrafSystem &grafSystem);

		~GrafPipeline();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define PROFILER_ENABLED 0
	class /*UR_DECL*/ Profiler
	{
	public:

		struct ScopedMarker
		{
		public:
			
			ScopedMarker(Log* log = ur_null, const char* message = ur_null);
			~ScopedMarker();

		private:

			Log* log;
			const char* message;
		};

		static inline const void Begin();

		static inline const ur_uint64 End(Log* log = ur_null, const char* message = ur_null);
	};

	#if (PROFILER_ENABLED)
	#define PROFILE_LINE(code_line, log) { Profiler ::ScopedMarker marker(log, #code_line); code_line; }
	#define PROFILE(code_line, log, message) { Profiler ::ScopedMarker marker(log, message); code_line; }
	#else
	#define PROFILE_LINE(code_line, log) code_line
	#define PROFILE(code_line, log, message) code_line
	#endif

} // end namespace UnlimRealms


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: VULKAN IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "vulkan/vulkan.h"
#if defined(_WINDOWS)
#include "vulkan/vulkan_win32.h"
#endif

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafSystemVulkan : public GrafSystem
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

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		inline VkInstance GetVkInstance() const;

		inline VkPhysicalDevice GetVkPhysicalDevice(ur_uint deviceId) const;

	private:

		Result Deinitialize();

		VkInstance vkInstance;
		std::vector<VkPhysicalDevice> vkPhysicalDevices;
	};

	inline VkInstance GrafSystemVulkan::GetVkInstance() const
	{
		return this->vkInstance;
	}

	inline VkPhysicalDevice GrafSystemVulkan::GetVkPhysicalDevice(ur_uint deviceId) const
	{
		return (deviceId < (ur_uint)vkPhysicalDevices.size() ? vkPhysicalDevices[deviceId] : VK_NULL_HANDLE);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDeviceVulkan : public GrafDevice
	{
	public:

		GrafDeviceVulkan(GrafSystem &grafSystem);

		~GrafDeviceVulkan();

		virtual Result Initialize(ur_uint deviceId);

		virtual Result Submit(GrafCommandList* grafCommandList);

		virtual Result WaitIdle();

		inline VkDevice GetVkDevice() const;

		inline ur_uint GetVkDeviceGraphicsQueueId() const;

		inline ur_uint GetVkDeviceComputeQueueId() const;

		inline ur_uint GetVkDeviceTransferQueueId() const;

		inline VkCommandPool GetVkGraphicsCommandPool() const;

		inline VkCommandPool GetVkComputeCommandPool() const;

		inline VkCommandPool GetVkTransferCommandPool() const;

	private:

		Result Deinitialize();

		VkDevice vkDevice;
		ur_uint deviceGraphicsQueueId;
		ur_uint deviceComputeQueueId;
		ur_uint deviceTransferQueueId;
		VkCommandPool vkGraphicsCommandPool;
		VkCommandPool vkComputeCommandPool;
		VkCommandPool vkTransferCommandPool;
	};

	inline VkDevice GrafDeviceVulkan::GetVkDevice() const
	{
		return this->vkDevice;
	}

	inline ur_uint GrafDeviceVulkan::GetVkDeviceGraphicsQueueId() const
	{
		return this->deviceGraphicsQueueId;
	}

	inline ur_uint GrafDeviceVulkan::GetVkDeviceComputeQueueId() const
	{
		return this->deviceComputeQueueId;
	}

	inline ur_uint GrafDeviceVulkan::GetVkDeviceTransferQueueId() const
	{
		return this->deviceTransferQueueId;
	}

	inline VkCommandPool GrafDeviceVulkan::GetVkGraphicsCommandPool() const
	{
		return this->vkGraphicsCommandPool;
	}

	inline VkCommandPool GrafDeviceVulkan::GetVkComputeCommandPool() const
	{
		return this->vkComputeCommandPool;
	}

	inline VkCommandPool GrafDeviceVulkan::GetVkTransferCommandPool() const
	{
		return this->vkTransferCommandPool;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafCommandListVulkan : public GrafCommandList
	{
	public:

		GrafCommandListVulkan(GrafSystem &grafSystem);

		~GrafCommandListVulkan();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result Begin();

		virtual Result End();

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

		inline VkCommandBuffer GetVkCommandBuffer() const;

		inline VkFence GetVkSubmitFence() const;

	private:

		Result Deinitialize();

		VkCommandBuffer vkCommandBuffer;
		VkFence vkSubmitFence;
	};

	inline VkCommandBuffer GrafCommandListVulkan::GetVkCommandBuffer() const
	{
		return this->vkCommandBuffer;
	}

	inline VkFence GrafCommandListVulkan::GetVkSubmitFence() const
	{
		return this->vkSubmitFence;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafFenceVulkan : public GrafFence
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

	inline VkEvent GrafFenceVulkan::GetVkEvent() const
	{
		return this->vkEvent;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafCanvasVulkan : public GrafCanvas
	{
	public:

		GrafCanvasVulkan(GrafSystem &grafSystem);

		~GrafCanvasVulkan();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams = InitParams::Default);

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
		ur_uint32 swapChainCurrentImageId;

		// per frame data
		ur_uint32 frameCount;
		ur_uint32 frameIdx;
		std::vector<std::unique_ptr<GrafCommandList>> imageTransitionCmdListBegin;
		std::vector<std::unique_ptr<GrafCommandList>> imageTransitionCmdListEnd;
		std::vector<VkSemaphore> vkSemaphoreImageAcquired;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafImageVulkan : public GrafImage
	{
	public:

		GrafImageVulkan(GrafSystem &grafSystem);

		~GrafImageVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

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
	};

	inline VkImage GrafImageVulkan::GetVkImage() const
	{
		return this->vkImage;
	}

	inline VkImageView GrafImageVulkan::GetVkImageView() const
	{
		return this->vkImageView;
	}

	inline void GrafImageVulkan::SetState(GrafImageState& state)
	{
		this->imageState = state;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafShaderVulkan : public GrafShader
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

	inline VkShaderModule GrafShaderVulkan::GetVkShaderModule() const
	{
		return this->vkShaderModule;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafRenderPassVulkan : public GrafRenderPass
	{
	public:

		GrafRenderPassVulkan(GrafSystem& grafSystem);

		~GrafRenderPassVulkan();

		virtual Result Initialize(GrafDevice* grafDevice);

		inline VkRenderPass GetVkRenderPass() const;
	
	private:

		Result Deinitialize();

		VkRenderPass vkRenderPass;
		VkPipelineLayout vkPipelineLayout;
	};

	inline VkRenderPass GrafRenderPassVulkan::GetVkRenderPass() const
	{
		return this->vkRenderPass;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafPipelineVulkan : public GrafPipeline
	{
	public:

		GrafPipelineVulkan(GrafSystem &grafSystem);

		~GrafPipelineVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const VkPipeline GetVkPipeline() const;

		inline const VkRenderPass GetVkRenderPass() const;

	protected:

		Result Deinitialize();

		VkPipeline vkPipeline;
		VkPipelineLayout vkPipelineLayout;

		// TEMP:
		VkRenderPass vkRenderPass;
	};

	inline const VkPipeline GrafPipelineVulkan::GetVkPipeline() const
	{
		return this->vkPipeline;
	}

	inline const VkRenderPass GrafPipelineVulkan::GetVkRenderPass() const
	{
		return this->vkRenderPass;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafUtilsVulkan
	{
	public:

		static inline VkImageUsageFlags GrafToVkImageUsage(GrafImageUsageFlags usage);
		static inline GrafImageUsageFlags VkToGrafImageUsage(VkImageUsageFlags usage);
		static inline VkImageType GrafToVkImageType(GrafImageType imageType);
		static inline VkImageAspectFlags GrafToVkImageUsageAspect(GrafImageUsageFlags usage);
		static inline VkImageLayout GrafToVkImageLayout(GrafImageState imageState);
		static inline VkShaderStageFlagBits GrafToVkShaderStage(GrafShaderType shaderType);
		static inline VkFormat GrafToVkFormat(GrafFormat grafFormat);
		static inline GrafFormat VkToGrafFormat(VkFormat vkFormat);
	};

} // end namespace UnlimRealms