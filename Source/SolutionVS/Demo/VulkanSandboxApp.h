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

	struct UR_DECL GrafDeviceDesc
	{
		std::wstring Description;
		ur_uint VendorId;
		ur_uint DeviceId;
		ur_size DedicatedVideoMemory;
		ur_size DedicatedSystemMemory;
		ur_size SharedSystemMemory;
	};

	class /*UR_DECL*/ GrafSystem : public RealmEntity
	{
	public:

		GrafSystem(Realm &realm);

		virtual ~GrafSystem();

		virtual Result Initialize(Canvas *canvas);
	};

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

	private:

		Result Deinitialize();

		VkInstance vkInstance;
	};

} // end namespace UnlimRealms