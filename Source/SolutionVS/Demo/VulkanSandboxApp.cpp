
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
	if (grafSystem != nullptr)
	{
		Result res = grafSystem->Initialize(realm.GetCanvas());
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
	Deinitialize();
}

Result GrafSystemVulkan::Deinitialize()
{
	if (this->vkInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(this->vkInstance, ur_null);
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
	GetRealm().GetLog().WriteLine("GrafSystemVulkan: extensions available:");
	for (auto& extensionProperty : vkExtensionProperties)
	{
		GetRealm().GetLog().WriteLine(extensionProperty.extensionName);
	}

	// enumerate layers
	ur_uint32 vkLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, ur_null);
	std::vector<VkLayerProperties> vkLayerProperties(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkLayerProperties.data());
	GetRealm().GetLog().WriteLine("GrafSystemVulkan: layers available:");
	for (auto& layerProperty : vkLayerProperties)
	{
		GetRealm().GetLog().WriteLine(std::string(layerProperty.layerName) + " (" + layerProperty.description + ")");
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
		GetRealm().GetLog().WriteLine(std::string("GrafSystemVulkan: vkCreateInstance failed with VkResult = ") + VkResultToString(res));
		return Result(Failure);
	}
	LogNoteGrafDbg("GrafSystemVulkan: VkInstance created");

	return Result(Success);
}