
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

// NOTE:
// dxc -T vs_5_0 -E main Generic_vs.hlsl -Fo Generic_vs.spv
// dxc -T ps_5_0 -E main Generic_ps.hlsl -Fo Generic_ps.spv

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
	std::unique_ptr<GrafSystem> grafSystem(new GrafSystemVulkan(realm));
	std::unique_ptr<GrafDevice> grafDevice;
	std::unique_ptr<GrafCanvas> grafCanvas;
	auto& deinitializeGfxSystem = [&grafSystem, &grafDevice, &grafCanvas]() -> void {
		// order matters!
		grafCanvas.reset();
		grafDevice.reset();
		grafSystem.reset();
	};
	Result grafRes = NotInitialized;
	do
	{
		grafRes = grafSystem->Initialize(realm.GetCanvas());
		if (Failed(grafRes) || 0 == grafSystem->GetPhysicalDeviceCount()) break;
		
		grafRes = grafSystem->CreateDevice(grafDevice);
		if (Failed(grafRes)) break;
	
		grafRes = grafDevice->Initialize(grafSystem->GetRecommendedDeviceId());
		if (Failed(grafRes)) break;

		grafRes = grafSystem->CreateCanvas(grafCanvas);
		if (Failed(grafRes)) break;

		grafRes = grafCanvas->Initialize(grafDevice.get());
		if (Failed(grafRes)) break;

	} while (false);
	if (Failed(grafRes))
	{
		deinitializeGfxSystem();
		realm.GetLog().WriteLine("VulkanSandboxApp: failed to initialize graphics system", Log::Error);
	}
	else
	{
		// TODO: GRAF system should be accessible from everywhere as a realm component;
		// requires keeping pointers to GrafDevice(s) and GrafCanvas(es) in GrafSystem...
		//realm.AddComponent(Component::GetUID<GrafSystem>(), std::unique_ptr<Component>(static_cast<Component*>(std::move(grafSystem))));
		//GrafSystem* grafSystem = realm.GetComponent<GrafSystem>();
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
			if (grafCanvas != ur_null)
			{
				grafCanvas->Initialize(grafDevice.get());
			}
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

	deinitializeGfxSystem();

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
	return Result(NotImplemented);
}

Result GrafSystem::CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas)
{
	return Result(NotImplemented);
}

Result GrafSystem::CreateImage(std::unique_ptr<GrafImage>& grafImage)
{
	return Result(NotImplemented);
}

Result GrafSystem::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
{
	return Result(NotImplemented);
}

ur_uint GrafSystem::GetRecommendedDeviceId()
{
	ur_uint recommendedDeviceId = ur_uint(-1);

	GrafPhysicalDeviceDesc bestDeviceDesc = {};
	for (ur_uint deviceId = 0; deviceId < (ur_uint)grafPhysicalDeviceDesc.size(); ++deviceId)
	{
		const GrafPhysicalDeviceDesc& deviceDesc = grafPhysicalDeviceDesc[deviceId];
		if (deviceDesc.DedicatedVideoMemory > bestDeviceDesc.DedicatedVideoMemory)
		{
			bestDeviceDesc.DedicatedVideoMemory = deviceDesc.DedicatedVideoMemory;
			recommendedDeviceId = deviceId;
		}
	}

	return recommendedDeviceId;
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
	return Result(NotImplemented);
}

GrafDeviceEntity::GrafDeviceEntity(GrafSystem &grafSystem) :
	GrafEntity(grafSystem),
	grafDevice(ur_null)
{
}

GrafDeviceEntity::~GrafDeviceEntity()
{
}

Result GrafDeviceEntity::Initialize(GrafDevice *grafDevice)
{
	this->grafDevice = grafDevice;
	return Result(NotImplemented);
}

GrafCanvas::GrafCanvas(GrafSystem &grafSystem) :
	GrafDeviceEntity(grafSystem)
{
}

GrafCanvas::~GrafCanvas()
{
}

const GrafCanvas::InitParams GrafCanvas::InitParams::Default = {
	GrafFormat::B8G8R8A8_UNORM, GrafPresentMode::VerticalSync, 3
};

Result GrafCanvas::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
{
	GrafDeviceEntity::Initialize(grafDevice);
	return Result(NotImplemented);
}

GrafImage::GrafImage(GrafSystem &grafSystem) :
	GrafDeviceEntity(grafSystem)
{
}

GrafImage::~GrafImage()
{
}

Result GrafImage::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	GrafDeviceEntity::Initialize(grafDevice);
	this->imageDesc = initParams.ImageDesc;
	return Result(NotImplemented);
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
#if defined(_WINDOWS)
	, "VK_KHR_win32_surface"
#endif
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	this->grafPhysicalDeviceDesc.clear();
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
	this->grafPhysicalDeviceDesc.resize(physicalDeviceCount);
	for (ur_uint32 deviceId = 0; deviceId < physicalDeviceCount; ++deviceId)
	{
		VkPhysicalDevice& vkPhysicalDevice = vkPhysicalDevices[deviceId];
		VkPhysicalDeviceProperties vkDeviceProperties;
		VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
		vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkDeviceProperties);
		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);
		
		GrafPhysicalDeviceDesc& grafDeviceDesc = this->grafPhysicalDeviceDesc[deviceId];
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

Result GrafSystemVulkan::CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas)
{
	grafCanvas.reset(new GrafCanvasVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateImage(std::unique_ptr<GrafImage>& grafImage)
{
	grafImage.reset(new GrafImageVulkan(*this));
	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafDeviceVulkan::GrafDeviceVulkan(GrafSystem &grafSystem) :
	GrafDevice(grafSystem)
{
	this->vkDevice = VK_NULL_HANDLE;
	this->deviceGraphicsQueueId = ur_uint(-1);
	this->deviceComputeQueueId = ur_uint(-1);
	this->deviceTransferQueueId = ur_uint(-1);
}

GrafDeviceVulkan::~GrafDeviceVulkan()
{
	this->Deinitialize();
}

Result GrafDeviceVulkan::Deinitialize()
{
	this->deviceGraphicsQueueId = ur_uint(-1);
	this->deviceComputeQueueId = ur_uint(-1);
	this->deviceTransferQueueId = ur_uint(-1);

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
	ur_uint firstGraphicsQueueFamily = ur_uint(-1);
	ur_uint firstComputeQueueFamily = ur_uint(-1);
	ur_uint firstTransferQueueFamily = ur_uint(-1);
	for (ur_size i = 0; i < vkQueueProperties.size(); ++i)
	{
		if (VK_QUEUE_GRAPHICS_BIT & vkQueueProperties[i].queueFlags)
			firstGraphicsQueueFamily = std::min(ur_uint(i), firstGraphicsQueueFamily);
		if (VK_QUEUE_COMPUTE_BIT & vkQueueProperties[i].queueFlags)
			firstComputeQueueFamily = std::min(ur_uint(i), firstComputeQueueFamily);
		if (VK_QUEUE_TRANSFER_BIT & vkQueueProperties[i].queueFlags)
			firstTransferQueueFamily = std::min(ur_uint(i), firstTransferQueueFamily);
	}
	if (firstGraphicsQueueFamily == ur_uint(-1))
	{
		return ResultError(Failure, std::string("GrafDeviceVulkan: graphics queue family is not available for device Id = ") + std::to_string(this->GetDeviceId()));
	}
	this->deviceGraphicsQueueId = firstGraphicsQueueFamily;
	this->deviceComputeQueueId = firstComputeQueueFamily;
	this->deviceTransferQueueId = firstTransferQueueFamily;

	// create logical device

	VkDeviceQueueCreateInfo vkQueueInfo = {};
	vkQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	vkQueueInfo.flags = 0;
	vkQueueInfo.queueFamilyIndex = (ur_uint32)firstGraphicsQueueFamily;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafCanvasVulkan::GrafCanvasVulkan(GrafSystem &grafSystem) :
	GrafCanvas(grafSystem)
{
	this->vkSurface = VK_NULL_HANDLE;
	this->vkSwapChain = VK_NULL_HANDLE;
}

GrafCanvasVulkan::~GrafCanvasVulkan()
{
	this->Deinitialize();
}

Result GrafCanvasVulkan::Deinitialize()
{
	this->swapChainImages.clear();
	if (this->vkSwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkSwapChain, ur_null);
		this->vkSwapChain = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafCanvasVulkan: swap chain destroyed");
	}
	if (this->vkSurface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(static_cast<GrafSystemVulkan&>(this->GetGrafSystem()).GetVkInstance(), this->vkSurface, ur_null);
		this->vkSurface = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafCanvasVulkan: surface destroyed");
	}

	return Result(Success);
}

Result GrafCanvasVulkan::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	LogNoteGrafDbg("GrafCanvasVulkan: initialization...");

	GrafCanvas::Initialize(grafDevice, initParams);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafCanvasVulkan: failed to initialize, invalid GrafDevice"));
	}

	// create surface

	GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());

	#if defined(_WINDOWS)

	WinCanvas* winCanvas = static_cast<WinCanvas*>(this->GetRealm().GetCanvas());
	if (ur_null == winCanvas)
	{
		return ResultError(InvalidArgs, std::string("GrafCanvasVulkan: failed to initialize, invalid WinCanvas"));
	}

	VkWin32SurfaceCreateInfoKHR vkSurfaceInfo = {};
	vkSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkSurfaceInfo.flags = 0;
	vkSurfaceInfo.hinstance = winCanvas->GetHinstance();
	vkSurfaceInfo.hwnd = winCanvas->GetHwnd();

	VkResult res = vkCreateWin32SurfaceKHR(grafSystemVulkan.GetVkInstance(), &vkSurfaceInfo, ur_null, &this->vkSurface);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateWin32SurfaceKHR failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafCanvasVulkan: VkSurfaceKHR created");

	#else

	return ResultError(NotImplemented, std::string("GrafCanvasVulkan: failed to initialize, unsupported platform"));

	#endif

	// validate presentation support on current device

	VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(grafDeviceVulkan->GetDeviceId());
	ur_uint32 vkDeviceGraphicsQueueId = (ur_uint32)grafDeviceVulkan->GetVkDeviceGraphicsQueueId();
	VkBool32 presentationSupported = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, vkDeviceGraphicsQueueId, this->vkSurface, &presentationSupported);
	if (!presentationSupported)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: failed to initialize, selected physical device graphics queue does not support presentation");
	}
	const ur_uint32 vkDevicePresentQueueCount = 1;
	ur_uint32 vkDevicePresentQueueIds[vkDevicePresentQueueCount] = { vkDeviceGraphicsQueueId };

	// validate swap chain caps

	VkSurfaceCapabilitiesKHR vkSurfaceCaps = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, this->vkSurface, &vkSurfaceCaps);

	VkSurfaceFormatKHR vkSurfaceFormat = {};
	ur_uint32 vkSurfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, this->vkSurface, &vkSurfaceFormatCount, ur_null);
	std::vector<VkSurfaceFormatKHR> vkSurfaceFormats(vkSurfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, this->vkSurface, &vkSurfaceFormatCount, vkSurfaceFormats.data());
	ur_bool surfaceFormatFound = false;
	VkFormat vkFormatRequested = GrafUtilsVulkan::GrafToVkFormat(initParams.Format);
	for (ur_size i = 0; i < vkSurfaceFormats.size(); ++i)
	{
		if (vkFormatRequested == vkSurfaceFormats[i].format)
			surfaceFormatFound = true;
		if (surfaceFormatFound)
		{
			vkSurfaceFormat = vkSurfaceFormats[i];
			break;
		}
	}
	if (!surfaceFormatFound)
	{
		LogWarning("GrafCanvasVulkan: requested format is not supported, switching to the first available");
		if (!vkSurfaceFormats.empty())
		{
			vkSurfaceFormat = vkSurfaceFormats[0];
			surfaceFormatFound = true;
		}
	}
	if (!surfaceFormatFound)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: failed to initialize, no surface format supported");
	}

	VkPresentModeKHR vkPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
	ur_uint32 vkPresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, this->vkSurface, &vkPresentModeCount, ur_null);
	std::vector<VkPresentModeKHR> vkPresentModes(vkPresentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, this->vkSurface, &vkPresentModeCount, vkPresentModes.data());
	ur_bool presentModeFound = false;
	for (ur_size i = 0; i < vkPresentModes.size(); ++i)
	{
		if (GrafPresentMode::Immediate == initParams.PresentMode &&
			VK_PRESENT_MODE_IMMEDIATE_KHR == vkPresentModes[i])
			presentModeFound = true;
		else if (GrafPresentMode::VerticalSync == initParams.PresentMode &&
			VK_PRESENT_MODE_FIFO_KHR == vkPresentModes[i])
			presentModeFound = true;
		if (presentModeFound)
		{
			vkPresentMode = vkPresentModes[i];
			break;
		}
	}
	if (!presentModeFound)
	{
		LogWarning("GrafCanvasVulkan: requested presentation mode is not supported, switching to the first available");
		if (!vkPresentModes.empty())
		{
			vkPresentMode = vkPresentModes[0];
			presentModeFound = true;
		}
	}
	if (!presentModeFound)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: failed to initialize, no presentation mode supported");
	}
	
	VkExtent2D vkSurfaceExtent = {};
	if (vkSurfaceCaps.currentExtent.width != ur_uint32(-1))
	{
		// only this extent can be used
		vkSurfaceExtent = vkSurfaceCaps.currentExtent;
	}
	else
	{
		// extent between reported min/max can be used, we choose the maximal
		vkSurfaceExtent = vkSurfaceCaps.maxImageExtent;
	}

	ur_uint32 vkSwapChainImageCount = (ur_uint32)initParams.SwapChainImageCount;
	vkSwapChainImageCount = std::max(vkSurfaceCaps.minImageCount, vkSwapChainImageCount);
	vkSwapChainImageCount = std::min(vkSurfaceCaps.maxImageCount, vkSwapChainImageCount);

	// create swap chain

	VkSwapchainCreateInfoKHR vkSwapChainInfo = {};
	vkSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	vkSwapChainInfo.flags = 0;
	vkSwapChainInfo.surface = this->vkSurface;
	vkSwapChainInfo.minImageCount = vkSwapChainImageCount;
	vkSwapChainInfo.imageFormat = vkSurfaceFormat.format;
	vkSwapChainInfo.imageColorSpace = vkSurfaceFormat.colorSpace;
	vkSwapChainInfo.imageExtent = vkSurfaceExtent;
	vkSwapChainInfo.imageArrayLayers = 1;
	vkSwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	vkSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // the same graphics queue is used (vkDeviceGraphicsQueueId)
	vkSwapChainInfo.queueFamilyIndexCount = vkDevicePresentQueueCount;
	vkSwapChainInfo.pQueueFamilyIndices = vkDevicePresentQueueIds;
	vkSwapChainInfo.preTransform = vkSurfaceCaps.currentTransform;
	vkSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	vkSwapChainInfo.presentMode = vkPresentMode;
	vkSwapChainInfo.clipped = VK_TRUE;
	vkSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	res = vkCreateSwapchainKHR(grafDeviceVulkan->GetVkDevice(), &vkSwapChainInfo, ur_null, &this->vkSwapChain);
	if (res != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSwapchainKHR failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafCanvasVulkan: VkSwapchainKHR created");

	// retrieve swap chain images

	ur_uint32 vkSwapChainImageRealCount = 0;
	vkGetSwapchainImagesKHR(grafDeviceVulkan->GetVkDevice(), this->vkSwapChain, &vkSwapChainImageRealCount, ur_null);
	std::vector<VkImage> vkSwapChainImages(vkSwapChainImageRealCount);
	vkGetSwapchainImagesKHR(grafDeviceVulkan->GetVkDevice(), this->vkSwapChain, &vkSwapChainImageRealCount, vkSwapChainImages.data());

	this->swapChainImages.resize(vkSwapChainImages.size());
	for (VkImage& vkImage : vkSwapChainImages)
	{
		GrafImage::InitParams imageParams = {};
		imageParams.ImageDesc.Type = GrafImageType::Tex2D;
		imageParams.ImageDesc.Format = GrafUtilsVulkan::VkToGrafFormat(vkSwapChainInfo.imageFormat);
		imageParams.ImageDesc.Size.x = vkSwapChainInfo.imageExtent.width;
		imageParams.ImageDesc.Size.y = vkSwapChainInfo.imageExtent.height;
		imageParams.ImageDesc.Size.z = 0;
		imageParams.ImageDesc.MipLevels = 1;
		imageParams.ImageDesc.Usage = GrafUtilsVulkan::VkToGrafImageUsage(vkSwapChainInfo.imageUsage);
		
		std::unique_ptr<GrafImage> grafImage;
		grafSystemVulkan.CreateImage(grafImage);
		Result res = static_cast<GrafImageVulkan*>(grafImage.get())->InitializeFromVkImage(grafDevice, imageParams, vkImage);
		if (Failed(res))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafCanvasVulkan: failed to create swap chain images");
		}
		this->swapChainImages.push_back(std::move(grafImage));
	}

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafImageVulkan::GrafImageVulkan(GrafSystem &grafSystem) :
	GrafImage(grafSystem)
{
	this->imageExternalHandle = false;
	this->vkImage = VK_NULL_HANDLE;
	this->vkImageView = VK_NULL_HANDLE;
}

GrafImageVulkan::~GrafImageVulkan()
{
	this->Deinitialize();
}

Result GrafImageVulkan::Deinitialize()
{
	if (this->vkImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkImageView, ur_null);
		this->vkImageView = VK_NULL_HANDLE;
	}
	if (this->vkImage != VK_NULL_HANDLE && !this->imageExternalHandle)
	{
		vkDestroyImage(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkImage, ur_null);
	}
	this->vkImage = VK_NULL_HANDLE;
	this->imageExternalHandle = false;

	return Result(Success);
}

Result GrafImageVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	GrafImage::Initialize(grafDevice, initParams);
	
	// TODO

	// create views

	Result res = this->CreateVkImageViews();
	if (Failed(res))
		return res;
	
	return Result(NotImplemented);
}

Result GrafImageVulkan::InitializeFromVkImage(GrafDevice *grafDevice, const InitParams& initParams, VkImage vkImage)
{
	this->Deinitialize();

	GrafImage::Initialize(grafDevice, initParams);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
	}

	// init external image

	this->imageExternalHandle = true;
	this->vkImage = vkImage;

	// create views

	if (this->vkImage != VK_NULL_HANDLE)
	{
		Result res = this->CreateVkImageViews();
		if (Failed(res))
			return res;
	}

	return Result(Success);
}

Result GrafImageVulkan::CreateVkImageViews()
{
	if (VK_NULL_HANDLE == this->vkImage || this->vkImageView != VK_NULL_HANDLE)
	{
		return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to create image views, invalid parameters"));
	}

	VkImageType vkImageType = GrafUtilsVulkan::GrafToVkImageType(this->GetDesc().Type);
	VkImageViewType vkViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	switch (vkImageType)
	{
	case VK_IMAGE_TYPE_1D: vkViewType = VK_IMAGE_VIEW_TYPE_1D; break;
	case VK_IMAGE_TYPE_2D: vkViewType = VK_IMAGE_VIEW_TYPE_2D; break;
	case VK_IMAGE_TYPE_3D: vkViewType = VK_IMAGE_VIEW_TYPE_3D; break;
	// TODO: handle texture arrays
	//VK_IMAGE_VIEW_TYPE_CUBE;
	//VK_IMAGE_VIEW_TYPE_1D_ARRAY;
	//VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	//VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	default:
		return ResultError(InvalidArgs, "GrafImageVulkan: failed to create image views, unsupported image type");
	};

	VkImageUsageFlags vkImageUsage = GrafUtilsVulkan::GrafToVkImageUsage(this->GetDesc().Usage);
	VkImageAspectFlags vkImageAspectFlags = 0;
	if (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT & vkImageUsage)
		vkImageAspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
	if (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT & vkImageUsage)
		vkImageAspectFlags |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

	VkImageViewCreateInfo vkViewInfo = {};
	vkViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	vkViewInfo.flags = 0;
	vkViewInfo.image = this->vkImage;
	vkViewInfo.viewType = vkViewType;
	vkViewInfo.format = GrafUtilsVulkan::GrafToVkFormat(this->GetDesc().Format);
	vkViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.subresourceRange.aspectMask = vkImageAspectFlags;
	vkViewInfo.subresourceRange.baseMipLevel = 0;
	vkViewInfo.subresourceRange.levelCount = this->GetDesc().MipLevels;
	vkViewInfo.subresourceRange.baseArrayLayer = 0;
	vkViewInfo.subresourceRange.layerCount = 1;

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice()); // at this point device expected to be validate
	VkResult res = vkCreateImageView(grafDeviceVulkan->GetVkDevice(), &vkViewInfo, ur_null, &this->vkImageView);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafImageVulkan: vkCreateImageView failed with VkResult = ") + VkResultToString(res));
	}

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkImageUsageFlags GrafUtilsVulkan::GrafToVkImageUsage(GrafImageUsage usage)
{
	VkImageUsageFlags vkImageUsage = 0;
	switch (usage)
	{
	case GrafImageUsage::TransferSrc: vkImageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT; break;
	case GrafImageUsage::TransferDst: vkImageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; break;
	case GrafImageUsage::ColorRenderTarget: vkImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; break;
	case GrafImageUsage::DepthStencilRenderTarget: vkImageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; break;
	};
	return vkImageUsage;
}

GrafImageUsage GrafUtilsVulkan::VkToGrafImageUsage(VkImageUsageFlags usage)
{
	GrafImageUsage grafUsage = GrafImageUsage::Undefined;
	switch (usage)
	{
	case VK_IMAGE_USAGE_TRANSFER_SRC_BIT: grafUsage = GrafImageUsage::TransferSrc; break;
	case VK_IMAGE_USAGE_TRANSFER_DST_BIT: grafUsage = GrafImageUsage::TransferDst; break;
	case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: grafUsage = GrafImageUsage::ColorRenderTarget; break;
	case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT: grafUsage = GrafImageUsage::DepthStencilRenderTarget; break;
	};
	return grafUsage;
}

VkImageType GrafUtilsVulkan::GrafToVkImageType(GrafImageType imageType)
{
	VkImageType vkImageType = VK_IMAGE_TYPE_MAX_ENUM;
	switch (imageType)
	{
	case GrafImageType::Tex1D: vkImageType = VK_IMAGE_TYPE_1D; break;
	case GrafImageType::Tex2D: vkImageType = VK_IMAGE_TYPE_2D; break;
	case GrafImageType::Tex3D: vkImageType = VK_IMAGE_TYPE_3D; break;
	};
	return vkImageType;
}

static const VkFormat GrafToVkFormatLUT[ur_uint(GrafFormat::Count)] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R8G8B8_UNORM,
	VK_FORMAT_R8G8B8_SNORM,
	VK_FORMAT_R8G8B8_UINT,
	VK_FORMAT_R8G8B8_SINT,
	VK_FORMAT_R8G8B8_SRGB,
	VK_FORMAT_B8G8R8_UNORM,
	VK_FORMAT_B8G8R8_SNORM,
	VK_FORMAT_B8G8R8_UINT,
	VK_FORMAT_B8G8R8_SINT,
	VK_FORMAT_B8G8R8_SRGB,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SNORM,
	VK_FORMAT_R8G8B8A8_UINT,
	VK_FORMAT_R8G8B8A8_SINT,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SNORM,
	VK_FORMAT_B8G8R8A8_UINT,
	VK_FORMAT_B8G8R8A8_SINT,
	VK_FORMAT_B8G8R8A8_SRGB
};
VkFormat GrafUtilsVulkan::GrafToVkFormat(GrafFormat grafFormat)
{
	return GrafToVkFormatLUT[ur_uint(grafFormat)];
}

static const GrafFormat VkToGrafFormatLUT[VK_FORMAT_END_RANGE + 1] = {
	GrafFormat::Undefined,
	GrafFormat::Unsupported,		// VK_FORMAT_R4G4_UNORM_PACK8 = 1,
	GrafFormat::Unsupported,		// VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
	GrafFormat::Unsupported,		// VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
	GrafFormat::Unsupported,		// VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
	GrafFormat::Unsupported,		// VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
	GrafFormat::Unsupported,		// VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
	GrafFormat::Unsupported,		// VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
	GrafFormat::Unsupported,		// VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_UNORM = 9,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_SNORM = 10,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_USCALED = 11,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_SSCALED = 12,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_UINT = 13,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_SINT = 14,
	GrafFormat::Unsupported,		// VK_FORMAT_R8_SRGB = 15,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_UNORM = 16,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_SNORM = 17,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_USCALED = 18,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_SSCALED = 19,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_UINT = 20,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_SINT = 21,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8_SRGB = 22,
	GrafFormat::R8G8B8_UNORM,		// VK_FORMAT_R8G8B8_UNORM = 23,
	GrafFormat::R8G8B8_SNORM,		// VK_FORMAT_R8G8B8_SNORM = 24,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8B8_USCALED = 25,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8B8_SSCALED = 26,
	GrafFormat::R8G8B8_UINT,		// VK_FORMAT_R8G8B8_UINT = 27,
	GrafFormat::R8G8B8_SINT,		// VK_FORMAT_R8G8B8_SINT = 28,
	GrafFormat::R8G8B8_SRGB,		// VK_FORMAT_R8G8B8_SRGB = 29,
	GrafFormat::B8G8R8_UNORM,		// VK_FORMAT_B8G8R8_UNORM = 30,
	GrafFormat::B8G8R8_SNORM,		// VK_FORMAT_B8G8R8_SNORM = 31,
	GrafFormat::Unsupported,		// VK_FORMAT_B8G8R8_USCALED = 32,
	GrafFormat::Unsupported,		// VK_FORMAT_B8G8R8_SSCALED = 33,
	GrafFormat::B8G8R8_UINT,		// VK_FORMAT_B8G8R8_UINT = 34,
	GrafFormat::B8G8R8_SINT,		// VK_FORMAT_B8G8R8_SINT = 35,
	GrafFormat::B8G8R8_SRGB,		// VK_FORMAT_B8G8R8_SRGB = 36,
	GrafFormat::R8G8B8A8_UNORM,		// VK_FORMAT_R8G8B8A8_UNORM = 37,
	GrafFormat::R8G8B8A8_SNORM,		// VK_FORMAT_R8G8B8A8_SNORM = 38,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8B8A8_USCALED = 39,
	GrafFormat::Unsupported,		// VK_FORMAT_R8G8B8A8_SSCALED = 40,
	GrafFormat::R8G8B8A8_UINT,		// VK_FORMAT_R8G8B8A8_UINT = 41,
	GrafFormat::R8G8B8A8_SINT,		// VK_FORMAT_R8G8B8A8_SINT = 42,
	GrafFormat::R8G8B8A8_SRGB,		// VK_FORMAT_R8G8B8A8_SRGB = 43,
	GrafFormat::B8G8R8A8_UNORM,		// VK_FORMAT_B8G8R8A8_UNORM = 44,
	GrafFormat::B8G8R8A8_SNORM,		// VK_FORMAT_B8G8R8A8_SNORM = 45,
	GrafFormat::Unsupported,		// VK_FORMAT_B8G8R8A8_USCALED = 46,
	GrafFormat::Unsupported,		// VK_FORMAT_B8G8R8A8_SSCALED = 47,
	GrafFormat::B8G8R8A8_UINT,		// VK_FORMAT_B8G8R8A8_UINT = 48,
	GrafFormat::B8G8R8A8_SINT,		// VK_FORMAT_B8G8R8A8_SINT = 49,
	GrafFormat::B8G8R8A8_SRGB,		// VK_FORMAT_B8G8R8A8_SRGB = 50,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
	GrafFormat::Unsupported,		// VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
	GrafFormat::Unsupported,		// VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
	GrafFormat::Unsupported,		// VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
	GrafFormat::Unsupported,		// VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
	GrafFormat::Unsupported,		// VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
	GrafFormat::Unsupported,		// VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
	GrafFormat::Unsupported,		// VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
	GrafFormat::Unsupported,		// VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
	GrafFormat::Unsupported,		// VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
	GrafFormat::Unsupported,		// VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
	GrafFormat::Unsupported,		// VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
	GrafFormat::Unsupported,		// VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
	GrafFormat::Unsupported,		// VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_UNORM = 70,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_SNORM = 71,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_USCALED = 72,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_SSCALED = 73,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_UINT = 74,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_SINT = 75,
	GrafFormat::Unsupported,		// VK_FORMAT_R16_SFLOAT = 76,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_UNORM = 77,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_SNORM = 78,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_USCALED = 79,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_SSCALED = 80,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_UINT = 81,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_SINT = 82,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16_SFLOAT = 83,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_UNORM = 84,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_SNORM = 85,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_USCALED = 86,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_SSCALED = 87,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_UINT = 88,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_SINT = 89,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16_SFLOAT = 90,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_UNORM = 91,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_SNORM = 92,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_USCALED = 93,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_SSCALED = 94,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_UINT = 95,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_SINT = 96,
	GrafFormat::Unsupported,		// VK_FORMAT_R16G16B16A16_SFLOAT = 97,
	GrafFormat::Unsupported,		// VK_FORMAT_R32_UINT = 98,
	GrafFormat::Unsupported,		// VK_FORMAT_R32_SINT = 99,
	GrafFormat::Unsupported,		// VK_FORMAT_R32_SFLOAT = 100,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32_UINT = 101,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32_SINT = 102,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32_SFLOAT = 103,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32B32_UINT = 104,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32B32_SINT = 105,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32B32_SFLOAT = 106,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32B32A32_UINT = 107,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32B32A32_SINT = 108,
	GrafFormat::Unsupported,		// VK_FORMAT_R32G32B32A32_SFLOAT = 109,
	GrafFormat::Unsupported,		// VK_FORMAT_R64_UINT = 110,
	GrafFormat::Unsupported,		// VK_FORMAT_R64_SINT = 111,
	GrafFormat::Unsupported,		// VK_FORMAT_R64_SFLOAT = 112,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64_UINT = 113,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64_SINT = 114,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64_SFLOAT = 115,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64B64_UINT = 116,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64B64_SINT = 117,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64B64_SFLOAT = 118,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64B64A64_UINT = 119,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64B64A64_SINT = 120,
	GrafFormat::Unsupported,		// VK_FORMAT_R64G64B64A64_SFLOAT = 121,
	GrafFormat::Unsupported,		// VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
	GrafFormat::Unsupported,		// VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
	GrafFormat::Unsupported,		// VK_FORMAT_D16_UNORM = 124,
	GrafFormat::Unsupported,		// VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
	GrafFormat::Unsupported,		// VK_FORMAT_D32_SFLOAT = 126,
	GrafFormat::Unsupported,		// VK_FORMAT_S8_UINT = 127,
	GrafFormat::Unsupported,		// VK_FORMAT_D16_UNORM_S8_UINT = 128,
	GrafFormat::Unsupported,		// VK_FORMAT_D24_UNORM_S8_UINT = 129,
	GrafFormat::Unsupported,		// VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
	GrafFormat::Unsupported,		// VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
	GrafFormat::Unsupported,		// VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
	GrafFormat::Unsupported,		// VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
	GrafFormat::Unsupported,		// VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
	GrafFormat::Unsupported,		// VK_FORMAT_BC2_UNORM_BLOCK = 135,
	GrafFormat::Unsupported,		// VK_FORMAT_BC2_SRGB_BLOCK = 136,
	GrafFormat::Unsupported,		// VK_FORMAT_BC3_UNORM_BLOCK = 137,
	GrafFormat::Unsupported,		// VK_FORMAT_BC3_SRGB_BLOCK = 138,
	GrafFormat::Unsupported,		// VK_FORMAT_BC4_UNORM_BLOCK = 139,
	GrafFormat::Unsupported,		// VK_FORMAT_BC4_SNORM_BLOCK = 140,
	GrafFormat::Unsupported,		// VK_FORMAT_BC5_UNORM_BLOCK = 141,
	GrafFormat::Unsupported,		// VK_FORMAT_BC5_SNORM_BLOCK = 142,
	GrafFormat::Unsupported,		// VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
	GrafFormat::Unsupported,		// VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
	GrafFormat::Unsupported,		// VK_FORMAT_BC7_UNORM_BLOCK = 145,
	GrafFormat::Unsupported,		// VK_FORMAT_BC7_SRGB_BLOCK = 146,
	GrafFormat::Unsupported,		// VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
	GrafFormat::Unsupported,		// VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
	GrafFormat::Unsupported,		// VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
	GrafFormat::Unsupported,		// VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
	GrafFormat::Unsupported,		// VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
	GrafFormat::Unsupported,		// VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
	GrafFormat::Unsupported,		// VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
	GrafFormat::Unsupported,		// VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
	GrafFormat::Unsupported,		// VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
	GrafFormat::Unsupported,		// VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
	GrafFormat::Unsupported,		// VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
};
GrafFormat GrafUtilsVulkan::VkToGrafFormat(VkFormat vkFormat)
{
	return VkToGrafFormatLUT[ur_uint(vkFormat)];
}