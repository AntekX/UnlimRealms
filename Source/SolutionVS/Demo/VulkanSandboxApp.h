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
	class GrafCanvas;
	class GrafImage;

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
	enum class GrafFormat
	{
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
	enum class GrafImageUsage
	{
		Undefined = 0,
		TransferSrc,
		TransferDst,
		ColorRenderTarget,
		DepthStencilRenderTarget,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GrafImageDesc
	{
		GrafImageType Type;
		GrafFormat Format;
		ur_uint3 Size;
		ur_uint MipLevels;
		GrafImageUsage Usage;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafSystem : public RealmEntity
	{
	public:

		GrafSystem(Realm &realm);

		virtual ~GrafSystem();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);

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

		inline ur_uint GetDeviceId();

	private:

		ur_uint deviceId;
	};

	inline ur_uint GrafDevice::GetDeviceId()
	{
		return deviceId;
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
	};

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
	};

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

	class /*UR_DECL*/ GrafSystemVulkan : public GrafSystem
	{
	public:

		GrafSystemVulkan(Realm &realm);

		virtual ~GrafSystemVulkan();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);

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

		Result Initialize(ur_uint deviceId);

		inline VkDevice GetVkDevice() const;

		inline ur_uint GetVkDeviceGraphicsQueueId() const;

		inline ur_uint GetVkDeviceComputeQueueId() const;

		inline ur_uint GetVkDeviceTransferQueueId() const;

	private:

		Result Deinitialize();

		VkDevice vkDevice;
		ur_uint deviceGraphicsQueueId;
		ur_uint deviceComputeQueueId;
		ur_uint deviceTransferQueueId;
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


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafCanvasVulkan : public GrafCanvas
	{
	public:

		GrafCanvasVulkan(GrafSystem &grafSystem);

		~GrafCanvasVulkan();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams = InitParams::Default);

	private:

		Result Deinitialize();

		VkSurfaceKHR vkSurface;
		VkSwapchainKHR vkSwapChain;
		std::vector<VkImage> vkSwapChainImages;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafImageVulkan : public GrafImage
	{
	public:

		GrafImageVulkan(GrafSystem &grafSystem);

		~GrafImageVulkan();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	private:

		VkImage vkImage;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafUtilsVulkan
	{
	public:

		static VkFormat GrafToVkFormat(GrafFormat grafFormat);
	};

} // end namespace UnlimRealms