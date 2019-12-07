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

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct /*UR_DECL*/ GrafDeviceDesc
	{
		std::string Description;
		ur_uint VendorId;
		ur_uint DeviceId;
		ur_size DedicatedVideoMemory;
		ur_size DedicatedSystemMemory;
		ur_size SharedSystemMemory;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class /*UR_DECL*/ GrafSystem : public RealmEntity
	{
	public:

		GrafSystem(Realm &realm);

		virtual ~GrafSystem();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		inline ur_uint GetDeviceDescCount();

		inline const GrafDeviceDesc* GetDeviceDesc(ur_uint deviceId);

	protected:

		std::vector<GrafDeviceDesc> grafDeviceDecsription;
	};

	inline ur_uint GrafSystem::GetDeviceDescCount()
	{
		return (ur_uint)grafDeviceDecsription.size();
	}

	inline const GrafDeviceDesc* GrafSystem::GetDeviceDesc(ur_uint deviceId)
	{
		return (deviceId < GetDeviceDescCount() ? &grafDeviceDecsription[deviceId] : ur_null);
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

} // end namespace UnlimRealms


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: VULKAN IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "vulkan/vulkan.h"

namespace UnlimRealms
{

	class /*UR_DECL*/ GrafSystemVulkan : public GrafSystem
	{
	public:

		GrafSystemVulkan(Realm &realm);

		virtual ~GrafSystemVulkan();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		
		// implementation specific functions

		inline VkPhysicalDevice GetVkPhysicalDevice(ur_uint deviceId);

	private:

		Result Deinitialize();

		VkInstance vkInstance;
		std::vector<VkPhysicalDevice> vkPhysicalDevices;
	};

	inline VkPhysicalDevice GrafSystemVulkan::GetVkPhysicalDevice(ur_uint deviceId)
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

	private:

		Result Deinitialize();

		VkDevice vkDevice;
	};

} // end namespace UnlimRealms