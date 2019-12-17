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
	class GrafBuffer;
	class GrafShader;
	class GrafRenderPass;
	class GrafRenderTarget;
	class GrafDescriptorTableLayout;
	class GrafDescriptorTable;
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
	enum class /*UR_DECL*/ GrafDeviceMemoryFlag
	{
		Undefined = 0,
		GpuLocal = (1 << 0),
		CpuVisible = (1 << 1)
	};
	typedef ur_uint GrafDeviceMemoryFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafPresentMode
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
	enum class /*UR_DECL*/ GrafFenceState
	{
		Undefined = -1,
		Reset,
		Signaled
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafFormat
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
		R32_SFLOAT,
		R32G32_SFLOAT,
		R32G32B32_SFLOAT,
		R32G32B32A32_SFLOAT,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafImageType
	{
		Undefined = 0,
		Tex1D,
		Tex2D,
		Tex3D,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafImageUsageFlag
	{
		Undefined = 0,
		TransferSrc = (1 << 0),
		TransferDst = (1 << 1),
		ColorRenderTarget = (1 << 2),
		DepthStencilRenderTarget = (1 << 3)
	};
	typedef ur_uint GrafImageUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafImageState
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
	struct /*UR_DECL*/ GrafImageDesc
	{
		GrafImageType Type;
		GrafFormat Format;
		ur_uint3 Size;
		ur_uint MipLevels;
		GrafImageUsageFlags Usage;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafBufferUsageFlag
	{
		VertexBuffer = (1 << 0),
		IndexBuffer = (1 << 1),
		ConstantBuffer = (1 << 2)
	};
	typedef ur_uint GrafBufferUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafBufferDesc
	{
		GrafBufferUsageFlags Usage;
		GrafDeviceMemoryFlags MemoryType;
		ur_size SizeInBytes;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafClearValue
	{
		union
		{
			ur_float f32[4];
			ur_int i32[4];
			ur_uint u32[4];
		};
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafShaderType
	{
		Undefined = -1,
		Vertex,
		Pixel,
		Compute,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafShaderStageFlag
	{
		Undefined = -1,
		Vertex = (0x1 << 0),
		Pixel = (0x1 << 1),
		Compute = (0x1 << 2),
		All = (Vertex | Pixel | Compute)
	};
	typedef ur_uint GrafShaderStageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafViewportDesc
	{
		ur_float X;
		ur_float Y;
		ur_float Width;
		ur_float Height;
		ur_float Near;
		ur_float Far;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafPrimitiveTopology
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class /*UR_DECL*/ GrafVertexInputType
	{
		PerVertex,
		PerInstance
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafVertexElementDesc
	{
		GrafFormat Format;
		ur_uint Offset;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafVertexInputDesc
	{
		GrafVertexInputType InputType;
		ur_uint BindingIdx;
		ur_uint Stride;
		GrafVertexElementDesc* Elements;
		ur_uint ElementCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafDescriptorType
	{
		Undefined = -1,
		ConstantBuffer,
		Texture,
		Sampler,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafDescriptorRangeDesc
	{
		GrafDescriptorType Type;
		ur_uint BindingCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafDescriptorTableLayoutDesc
	{
		GrafShaderStageFlags ShaderStageVisibility;
		GrafDescriptorRangeDesc* DescriptorRanges;
		ur_uint DescriptorRangeCount;
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

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);
		
		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

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

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);
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

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Present();

		virtual GrafImage* GetCurrentImage();

		virtual GrafImage* GetSwapChainImage(ur_uint imageId);

		inline ur_uint GetSwapChainImageCount() const;

		inline ur_uint GetCurrentImageId() const;

	protected:

		ur_uint swapChainImageCount;
		ur_uint swapChainCurrentImageId;
	};

	inline ur_uint GrafCanvas::GetSwapChainImageCount() const
	{
		return this->swapChainImageCount;
	}

	inline ur_uint GrafCanvas::GetCurrentImageId() const
	{
		return this->swapChainCurrentImageId;
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
	class /*UR_DECL*/ GrafBuffer : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafBufferDesc BufferDesc;
		};

		GrafBuffer(GrafSystem &grafSystem);

		~GrafBuffer();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Transfer(GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline const GrafBufferDesc& GetDesc() const;

	protected:

		GrafBufferDesc bufferDesc;
	};

	inline const GrafBufferDesc& GrafBuffer::GetDesc() const
	{
		return this->bufferDesc;
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
	class /*UR_DECL*/ GrafRenderTarget : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafRenderPass* RenderPass;
			GrafImage** Images;
			ur_uint ImageCount;
		};

		GrafRenderTarget(GrafSystem &grafSystem);

		~GrafRenderTarget();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline GrafRenderPass* GetRenderPass() const;

		inline GrafImage* GetImage(ur_uint imageId) const;

		inline ur_uint GetImageCount() const;

	protected:

		GrafRenderPass* renderPass;
		std::vector<GrafImage*> images;
	};

	GrafRenderPass* GrafRenderTarget::GetRenderPass() const
	{
		return this->renderPass;
	}

	GrafImage* GrafRenderTarget::GetImage(ur_uint imageId) const
	{
		return (imageId < this->images.size() ? this->images[imageId] : ur_null);
	}

	ur_uint GrafRenderTarget::GetImageCount() const
	{
		return (ur_uint)this->images.size();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDescriptorTableLayout : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafDescriptorTableLayoutDesc LayoutDesc;
		};

		GrafDescriptorTableLayout(GrafSystem &grafSystem);

		~GrafDescriptorTableLayout();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafDescriptorTableLayoutDesc& GetLayoutDesc() const;

	protected:

		GrafDescriptorTableLayoutDesc layoutDesc;
		std::vector<GrafDescriptorRangeDesc> descriptorRanges;
	};

	inline const GrafDescriptorTableLayoutDesc& GrafDescriptorTableLayout::GetLayoutDesc() const
	{
		return this->layoutDesc;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDescriptorTable : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafDescriptorTableLayout* Layout;
		};

		GrafDescriptorTable(GrafSystem &grafSystem);

		~GrafDescriptorTable();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_uint bufferOfs = 0, ur_uint bufferRange = 0);

		inline GrafDescriptorTableLayout* GetLayout() const;

	protected:

		GrafDescriptorTableLayout* layout;
	};

	inline GrafDescriptorTableLayout* GrafDescriptorTable::GetLayout() const
	{
		return this->layout;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafPipeline : public GrafDeviceEntity
	{
	public:

		struct /*UR_DECL*/ InitParams
		{
			GrafRenderPass* RenderPass;
			GrafShader** ShaderStages;
			ur_uint ShaderStageCount;
			GrafDescriptorTableLayout** DescriptorTableLayouts;
			ur_uint DescriptorTableLayoutCount;
			GrafVertexInputDesc* VertexInputDesc;
			ur_uint VertexInputCount;
			GrafPrimitiveTopology PrimitiveTopology;
			GrafViewportDesc ViewportDesc;
			static const InitParams Default;
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

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

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

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);

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
	class /*UR_DECL*/ GrafBufferVulkan : public GrafBuffer
	{
	public:

		GrafBufferVulkan(GrafSystem &grafSystem);

		~GrafBufferVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Transfer(GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline VkBuffer GetVkBuffer() const;

	private:

		Result Deinitialize();

		VkBuffer vkBuffer;
		VkDeviceMemory vkDeviceMemory;
		VkDeviceSize vkDeviceMemoryOffset;
		VkDeviceSize vkDeviceMemorySize;
		VkDeviceSize vkDeviceMemoryAlignment;
	};

	inline VkBuffer GrafBufferVulkan::GetVkBuffer() const
	{
		return this->vkBuffer;
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
	};

	inline VkRenderPass GrafRenderPassVulkan::GetVkRenderPass() const
	{
		return this->vkRenderPass;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafRenderTargetVulkan : public GrafRenderTarget
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

	inline VkFramebuffer GrafRenderTargetVulkan::GetVkFramebuffer() const
	{
		return this->vkFramebuffer;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDescriptorTableLayoutVulkan : public GrafDescriptorTableLayout
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

	inline VkDescriptorSetLayout GrafDescriptorTableLayoutVulkan::GetVkDescriptorSetLayout() const
	{
		return this->vkDescriptorSetLayout;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafDescriptorTableVulkan : public GrafDescriptorTable
	{
	public:

		GrafDescriptorTableVulkan(GrafSystem &grafSystem);

		~GrafDescriptorTableVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_uint bufferOfs = 0, ur_uint bufferRange = 0);

		inline VkDescriptorSet GetVkDescriptorSet() const;

	private:

		Result Deinitialize();

		VkDescriptorSet vkDescriptorSet;
		VkDescriptorPool vkDescriptorPool;
	};

	inline VkDescriptorSet GrafDescriptorTableVulkan::GetVkDescriptorSet() const
	{
		return this->vkDescriptorSet;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafPipelineVulkan : public GrafPipeline
	{
	public:

		GrafPipelineVulkan(GrafSystem &grafSystem);

		~GrafPipelineVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline VkPipeline GetVkPipeline() const;

		inline VkPipelineLayout GetVkPipelineLayout() const;

		// TEMP: updated descriptor sets directly
		Result UpdateConstantBuffer(ur_uint setIdx, GrafBuffer* buffer);
		Result BindDescriptorSet(ur_uint setIdx, GrafCommandList* commandList);

	protected:

		Result Deinitialize();

		VkPipeline vkPipeline;
		VkPipelineLayout vkPipelineLayout;

		// TEMP: descriptors sample
		VkDescriptorSetLayout vkDescriptorSetLayout;
		VkDescriptorPool vkDescriptorPool; // TODO: should be part of GrafDevice, consider to grow/shrink allocated pool automatically
		std::vector<VkDescriptorSet> vkDescriptorSets;
	};

	inline VkPipeline GrafPipelineVulkan::GetVkPipeline() const
	{
		return this->vkPipeline;
	}

	inline VkPipelineLayout GrafPipelineVulkan::GetVkPipelineLayout() const
	{
		return this->vkPipelineLayout;
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
		static inline VkBufferUsageFlags GrafToVkBufferUsage(GrafBufferUsageFlags usage);
		static inline VkMemoryPropertyFlags GrafToVkMemoryProperties(GrafDeviceMemoryFlags memoryType);
		static inline VkShaderStageFlagBits GrafToVkShaderStage(GrafShaderType shaderType);
		static inline VkShaderStageFlags GrafToVkShaderStage(GrafShaderStageFlags shaderStages);
		static inline VkDescriptorType GrafToVkDescriptorType(GrafDescriptorType descriptorType);
		static inline VkPrimitiveTopology GrafToVkPrimitiveTopology(GrafPrimitiveTopology topology);
		static inline VkFormat GrafToVkFormat(GrafFormat grafFormat);
		static inline GrafFormat VkToGrafFormat(VkFormat vkFormat);
	};

} // end namespace UnlimRealms