
#include "stdafx.h"
#include "VulkanSandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Resources/Resources.h"
#include "Core/Math.h"
#include "ImguiRender/ImguiRender.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

int VulkanSandboxApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"Vulkan Sandbox"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));
	ur_uint canvasWidth = realm.GetCanvas()->GetClientBound().Width();
	ur_uint canvasHeight = realm.GetCanvas()->GetClientBound().Height();

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// initialize gfx system
	realm.AddComponent(Component::GetUID<GrafSystem>(), std::unique_ptr<Component>(static_cast<Component*>(new GrafSystemVulkan(realm))));
	GrafSystem* grafSystem = realm.GetComponent<GrafSystem>();
	std::unique_ptr<GrafDevice> grafDevice;
	if (grafSystem != nullptr)
	{
		Result res = grafSystem->Initialize(realm.GetCanvas());
		if (Succeeded(res) && grafSystem->GetDeviceDescCount() > 0)
		{
			res = grafSystem->CreateDevice(grafDevice);
			if (Succeeded(res))
			{
				grafDevice->Initialize(0);
			}
		}
	}

	// initialize ImguiRender
	ImguiRender* imguiRender = realm.AddComponent<ImguiRender>(realm);
	if (imguiRender != ur_null)
	{
		imguiRender = realm.GetComponent<ImguiRender>();
		Result res = imguiRender->Init();
		if (Failed(res))
		{
			realm.RemoveComponent<ImguiRender>();
			imguiRender = ur_null;
			realm.GetLog().WriteLine("VulkanSandbox: failed to initialize ImguiRender", Log::Error);
		}
	}

	// Main message loop:
	ClockTime timer = Clock::now();
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT && !realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::Escape))
	{
		// process messages first
		ClockTime timeBefore = Clock::now();
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) && msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// forward msg to WinInput system
			WinInput* winInput = static_cast<WinInput*>(realm.GetInput());
			winInput->ProcessMsg(msg);
			break;
		}
		auto inputDelay = Clock::now() - timeBefore;
		timer += inputDelay;  // skip input delay

		// update resolution
		if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() || canvasHeight != realm.GetCanvas()->GetClientBound().Height())
		{
			canvasWidth = realm.GetCanvas()->GetClientBound().Width();
			canvasHeight = realm.GetCanvas()->GetClientBound().Height();
			// TODO: resize frame buffer(s)
		}

		// update sub systems
		realm.GetInput()->Update();
		if (imguiRender != ur_null)
		{
			imguiRender->NewFrame();
		}

		// update frame
		auto updateFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {
			
			// move timer
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f;  // to seconds

			// TODO: animate things here

			ctx.progress = ur_float(1.0f);
		});

		// draw frame
		auto drawFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {
			
			updateFrameJob->WaitProgress(ur_float(1.0f));  // wait till portion of data required for this draw call is fully updated

			// TODO: draw primtives here
		});

		drawFrameJob->Wait();

		// TODO: present & flip here
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: GRaphics Abstraction Front-end
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafSystem::GrafSystem(Realm &realm) :
	RealmEntity(realm)
{
	LogNote("GrafSystem: created");
}

GrafSystem::~GrafSystem()
{
	LogNote("GrafSystem: destroyed");
}

Result GrafSystem::Initialize(Canvas *canvas)
{
	return NotImplemented;
}

Result GrafSystem::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
{
	return NotImplemented;
}

GrafEntity::GrafEntity(GrafSystem &grafSystem) :
	RealmEntity(grafSystem.GetRealm()),
	grafSystem(grafSystem)
{
}

GrafEntity::~GrafEntity()
{
}

GrafDevice::GrafDevice(GrafSystem &grafSystem) :
	GrafEntity(grafSystem)
{
}

GrafDevice::~GrafDevice()
{
}

Result GrafDevice::Initialize(ur_uint deviceId)
{
	this->deviceId = deviceId;
	return NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: VULKAN IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma comment(lib, "vulkan-1.lib")

#if defined(_DEBUG)
#define UR_GRAF_LOG_LEVEL_DEBUG
#define UR_GRAF_VULKAN_DEBUG_LAYER
#endif

#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
#define LogNoteGrafDbg(text) GetRealm().GetLog().WriteLine(text, Log::Note)
#else
#define LogNoteGrafDbg(text)
#endif

#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)
static const char* VulkanLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};
#else
static const char** VulkanLayers = ur_null;
#endif

static const char* VulkanExtensions[] = {
	"VK_KHR_surface"
#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)
	, "VK_EXT_debug_utils"
#endif
};

static const char* VulkanDeviceExtensions[] = {
	"VK_KHR_swapchain"
};

static const char* VkResultToString(VkResult res)
{
	switch (res)
	{
	case VK_SUCCESS: return "VK_SUCCESS";
	case VK_NOT_READY: return "VK_NOT_READY";
	case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_FRAGMENTATION_EXT: return "VK_ERROR_FRAGMENTATION_EXT";
    case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
	};
	return "VK_UNKNOWN";
}

GrafSystemVulkan::GrafSystemVulkan(Realm &realm) : 
	GrafSystem(realm)
{
	this->vkInstance = VK_NULL_HANDLE;
}

GrafSystemVulkan::~GrafSystemVulkan()
{
	this->Deinitialize();
}

Result GrafSystemVulkan::Deinitialize()
{
	this->grafDeviceDecsription.clear();
	this->vkPhysicalDevices.clear();

	if (this->vkInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(this->vkInstance, ur_null);
		this->vkInstance = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafSystemVulkan: deinitialized");
	}

	return Result(Success);
}

Result GrafSystemVulkan::Initialize(Canvas *canvas)
{
	this->Deinitialize();

	LogNoteGrafDbg("GrafSystemVulkan: initialization...");
	
	#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
	
	// enumerate extentions

	ur_uint32 vkExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(ur_null, &vkExtensionCount, ur_null);
	std::vector<VkExtensionProperties> vkExtensionProperties(vkExtensionCount);
	vkEnumerateInstanceExtensionProperties(ur_null, &vkExtensionCount, vkExtensionProperties.data());
	LogNoteGrafDbg("GrafSystemVulkan: extensions available:");
	for (auto& extensionProperty : vkExtensionProperties)
	{
		LogNoteGrafDbg(extensionProperty.extensionName);
	}

	// enumerate layers

	ur_uint32 vkLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, ur_null);
	std::vector<VkLayerProperties> vkLayerProperties(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkLayerProperties.data());
	LogNoteGrafDbg("GrafSystemVulkan: layers available:");
	for (auto& layerProperty : vkLayerProperties)
	{
		LogNoteGrafDbg(std::string(layerProperty.layerName) + " (" + layerProperty.description + ")");
	}

	// TODO: consider setting up custom debug callback for vulkan messages
	
	#endif

	// create instance

	VkApplicationInfo vkAppInfo = {};
	vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkAppInfo.pApplicationName = "UnlimRealms Application";
	vkAppInfo.pEngineName = "UnlimRealms";
	vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo vkCreateInfo = {};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.flags = 0;
	vkCreateInfo.pApplicationInfo = &vkAppInfo;
	vkCreateInfo.enabledLayerCount = (VulkanLayers ? ur_array_size(VulkanLayers) : 0);
	vkCreateInfo.ppEnabledLayerNames = VulkanLayers;
	vkCreateInfo.enabledExtensionCount = (VulkanExtensions ? ur_array_size(VulkanExtensions) : 0);
	vkCreateInfo.ppEnabledExtensionNames = VulkanExtensions;

	VkResult res = vkCreateInstance(&vkCreateInfo, ur_null, &this->vkInstance);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafSystemVulkan: vkCreateInstance failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafSystemVulkan: VkInstance created");

	// enumerate physical devices

	ur_uint32 physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, ur_null);
	if (0 == physicalDeviceCount)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafSystemVulkan: init failed, no physical device found");
	}
	this->vkPhysicalDevices.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, vkPhysicalDevices.data());
	this->grafDeviceDecsription.resize(physicalDeviceCount);
	for (ur_uint32 deviceId = 0; deviceId < physicalDeviceCount; ++deviceId)
	{
		VkPhysicalDevice& vkPhysicalDevice = vkPhysicalDevices[deviceId];
		VkPhysicalDeviceProperties vkDeviceProperties;
		VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
		vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkDeviceProperties);
		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);
		
		GrafDeviceDesc& grafDeviceDesc = this->grafDeviceDecsription[deviceId];
		grafDeviceDesc = {};
		grafDeviceDesc.Description = vkDeviceProperties.deviceName;
		grafDeviceDesc.VendorId = (ur_uint)vkDeviceProperties.vendorID;
		grafDeviceDesc.DeviceId = (ur_uint)vkDeviceProperties.deviceID;
		std::vector<VkMemoryPropertyFlags> perHeapFlags(vkDeviceMemoryProperties.memoryHeapCount);
		for (ur_uint32 memTypeIdx = 0; memTypeIdx < vkDeviceMemoryProperties.memoryTypeCount; ++memTypeIdx)
		{
			VkMemoryType& vkMemType = vkDeviceMemoryProperties.memoryTypes[memTypeIdx];
			perHeapFlags[vkMemType.heapIndex] |= vkMemType.propertyFlags;
		}
		for (ur_uint32 memHeapIdx = 0; memHeapIdx < vkDeviceMemoryProperties.memoryHeapCount; ++memHeapIdx)
		{
			VkMemoryHeap& vkMemHeap = vkDeviceMemoryProperties.memoryHeaps[memHeapIdx];
			if (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & perHeapFlags[memHeapIdx]) grafDeviceDesc.DedicatedVideoMemory += (ur_size)vkMemHeap.size;
			if (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & perHeapFlags[memHeapIdx]) grafDeviceDesc.SharedSystemMemory += (ur_size)vkMemHeap.size;
		}
		
		#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
		LogNoteGrafDbg(std::string("GrafSystemVulkan: device available ") + grafDeviceDesc.Description +
			", VRAM = " + std::to_string(grafDeviceDesc.DedicatedVideoMemory >> 20) + " Mb");
		#endif
	}

	return Result(Success);
}

Result GrafSystemVulkan::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
{
	grafDevice.reset(new GrafDeviceVulkan(*this));
	return Result(Success);
}

GrafDeviceVulkan::GrafDeviceVulkan(GrafSystem &grafSystem) :
	GrafDevice(grafSystem)
{
	this->vkDevice = VK_NULL_HANDLE;
}

GrafDeviceVulkan::~GrafDeviceVulkan()
{
	this->Deinitialize();
}

Result GrafDeviceVulkan::Deinitialize()
{
	if (this->vkDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(this->vkDevice, ur_null);
		this->vkDevice = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafDeviceVulkan: deinitialized");
	}

	return Result(Success);
}

Result GrafDeviceVulkan::Initialize(ur_uint deviceId)
{
	this->Deinitialize();

	LogNoteGrafDbg("GrafDeviceVulkan: initialization...");

	GrafDevice::Initialize(deviceId);

	// get corresponding physical device 

	GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
	VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(this->GetDeviceId());
	if (VK_NULL_HANDLE == vkPhysicalDevice)
	{
		return ResultError(Failure, std::string("GrafDeviceVulkan: can not find VkPhysicalDevice for deviceId = ") + std::to_string(this->GetDeviceId()));
	}

	// enumerate device queue families

	ur_uint32 vkQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, ur_null);
	std::vector<VkQueueFamilyProperties> vkQueueProperties(vkQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, vkQueueProperties.data());
	ur_uint32 firstGraphicsQueueFamily = 0;
	ur_uint32 firstComputeQueueFamily = 0;
	ur_uint32 firstTransferQueueFamily = 0;
	for (ur_size i = 0; i < vkQueueProperties.size(); ++i)
	{
		if (VK_QUEUE_GRAPHICS_BIT & vkQueueProperties[i].queueFlags)
			firstGraphicsQueueFamily = std::min(ur_uint32(i), firstGraphicsQueueFamily);
		if (VK_QUEUE_COMPUTE_BIT & vkQueueProperties[i].queueFlags)
			firstComputeQueueFamily = std::min(ur_uint32(i), firstComputeQueueFamily);
		if (VK_QUEUE_TRANSFER_BIT & vkQueueProperties[i].queueFlags)
			firstTransferQueueFamily = std::min(ur_uint32(i), firstTransferQueueFamily);
	}

	// create logical device

	VkDeviceQueueCreateInfo vkQueueInfo = {};
	vkQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	vkQueueInfo.flags = 0;
	vkQueueInfo.queueFamilyIndex = firstGraphicsQueueFamily;
	vkQueueInfo.queueCount = 1;
	ur_float queuePriority = 1.0f;
	vkQueueInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures vkDeviceFeatures = {};
	// todo: pass to Initialize a GRAF structure describing essential user defined feature list and fill corresponding fields in VkPhysicalDeviceFeatures

	VkDeviceCreateInfo vkDeviceInfo = {};
	vkDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	vkDeviceInfo.flags = 0;
	vkDeviceInfo.queueCreateInfoCount = 1;
	vkDeviceInfo.pQueueCreateInfos = &vkQueueInfo;
	vkDeviceInfo.enabledLayerCount = (VulkanLayers ? ur_array_size(VulkanLayers) : 0);
	vkDeviceInfo.ppEnabledLayerNames = VulkanLayers;
	vkDeviceInfo.enabledExtensionCount = (VulkanDeviceExtensions ? ur_array_size(VulkanDeviceExtensions) : 0);
	vkDeviceInfo.ppEnabledExtensionNames = VulkanDeviceExtensions;
	vkDeviceInfo.pEnabledFeatures = &vkDeviceFeatures;

	VkResult res = vkCreateDevice(vkPhysicalDevice, &vkDeviceInfo, ur_null, &this->vkDevice);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafDeviceVulkan: vkCreateDevice failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafDeviceVulkan: VkDevice created");

	return Result(Success);
}