///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "GrafSystemVulkan.h"
#include "Sys/Log.h"
#if defined(_WINDOWS)
#include "Sys/Windows/WinCanvas.h"
#endif
#pragma comment(lib, "vulkan-1.lib")
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

namespace UnlimRealms
{

	#if defined(_DEBUG)
	#define UR_GRAF_VULKAN_DEBUG_LAYER
	#else
	//#define UR_GRAF_VULKAN_DEBUG_LAYER
	#endif
	#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)
	#define UR_GRAF_VULKAN_DEBUG_MSG_SEVIRITY_MIN VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
	#endif
	#define UR_GRAF_VULKAN_DEBUG_LAYER_ASSERT 0

	#define UR_GRAF_VULKAN_VERSION VK_API_VERSION_1_3
	#define UR_GRAF_VULKAN_DEBUG_LABLES 1
	#define UR_GRAF_VULKAN_VMA_ENABLED 1
	#define UR_GRAF_VULKAN_IMPLICIT_WAIT_DEVICE 1

	// command buffer synchronisation policy
	#define UR_GRAF_VULKAN_COMMAND_BUFFER_SYNC_DESTROY	1
	#define UR_GRAF_VULKAN_COMMAND_BUFFER_SYNC_RESET	1

	// enables swap chain image layout automatic transition to general/common state when it becomes a current render target
	#define UR_GRAF_VULKAN_SWAPCHAIN_NEXT_IMAGE_IMPLICIT_TRANSITION_TO_GENERAL 1

	// descritor pool per type size
	struct VulkanDescriptorTypeSize
	{
		VkDescriptorType Type;
		ur_size Size;
	};
	static const VulkanDescriptorTypeSize VulkanDescriptorPoolSize[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER,						1 * 1024 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		0 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,					2 * 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,					1 * 1024 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,			0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,			0 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				4 * 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				2 * 1024 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		0 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,				0 },
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,	1 * 1024 },
		#endif
	};
	static const ur_size VulkanDescriptorPoolMaxSetCount = 2 * 1024;

	// binding offsets per descriptor register type
	// spir-v bytecode compiled from HLSL must use "-fvk-<b,s,t,u>-shift N M" command to apply offsets to corresponding register types (b,s,t,u)
	static const ur_uint VulkanBindingsPerRegisterType = 256;
	static const ur_uint VulkanBindingOffsetConstantBuffer = 0;
	static const ur_uint VulkanBindingOffsetSampler = VulkanBindingOffsetConstantBuffer + VulkanBindingsPerRegisterType;
	static const ur_uint VulkanBindingOffsetReadWriteResource = VulkanBindingOffsetSampler + VulkanBindingsPerRegisterType;
	static const ur_uint VulkanBindingOffsetReadOnlyResource = VulkanBindingOffsetReadWriteResource + VulkanBindingsPerRegisterType;
	static const ur_uint VulkanBindingOffsetTexture = VulkanBindingOffsetReadOnlyResource;
	static const ur_uint VulkanBindingOffsetBuffer = VulkanBindingOffsetReadOnlyResource;
	static const ur_uint VulkanBindingOffsetRWTexture = VulkanBindingOffsetReadWriteResource;
	static const ur_uint VulkanBindingOffsetRWBuffer = VulkanBindingOffsetReadWriteResource;
	static const ur_uint VulkanBindingOffsetAccelerationStructure = VulkanBindingOffsetReadOnlyResource;
	static const ur_uint VulkanBindingOffsetTextureDynamicArray = VulkanBindingOffsetReadOnlyResource;
	static const ur_uint VulkanBindingOffsetBufferDynamicArray = VulkanBindingOffsetReadOnlyResource;

	#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)
	static const char* VulkanLayers[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	#else
	static const char** VulkanLayers = ur_null;
	#endif

	static const char* VulkanExtensions[] = {
		"VK_KHR_surface",
		"VK_KHR_get_physical_device_properties2"
		#if defined(_WINDOWS)
		, "VK_KHR_win32_surface"
		#endif
		#if defined(UR_GRAF_VULKAN_DEBUG_LAYER) || (UR_GRAF_VULKAN_DEBUG_LABLES)
		, "VK_EXT_debug_utils"
		#endif
	};

	static const char* VulkanDeviceExtensions[] = {
		"VK_KHR_swapchain"
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		, "VK_KHR_get_memory_requirements2"
		, "VK_KHR_acceleration_structure"
		, "VK_KHR_ray_tracing_pipeline"
		, "VK_KHR_ray_query"
		, "VK_KHR_pipeline_library"
		, "VK_KHR_deferred_host_operations"
		, "VK_KHR_buffer_device_address"
		#endif
	};

	#if defined(UR_GRAF_VULKAN_DEBUG_LABLES)
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = ur_null;
	PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT = ur_null;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = ur_null;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = ur_null;
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = ur_null;
	#endif

	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = ur_null;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = ur_null;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = ur_null;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = ur_null;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = ur_null;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = ur_null;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = ur_null;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = ur_null;
	#endif

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
		this->vkDebugUtilsMessenger = VK_NULL_HANDLE;
	}

	GrafSystemVulkan::~GrafSystemVulkan()
	{
		this->Deinitialize();
	}

	Result GrafSystemVulkan::Deinitialize()
	{
		this->grafPhysicalDeviceDesc.clear();
		this->vkPhysicalDevices.clear();

		if (this->vkDebugUtilsMessenger != VK_NULL_HANDLE)
		{
			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->vkInstance, "vkDestroyDebugUtilsMessengerEXT");
			if (vkDestroyDebugUtilsMessengerEXT != ur_null)
			{
				vkDestroyDebugUtilsMessengerEXT(this->vkInstance, this->vkDebugUtilsMessenger, ur_null);
			}
		}

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

		// NOTE: consider setting up custom debug callback for vulkan messages

		#endif

		// create instance

		VkApplicationInfo vkAppInfo = {};
		vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vkAppInfo.pApplicationName = "UnlimRealms Application";
		vkAppInfo.pEngineName = "UnlimRealms";
		vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		vkAppInfo.apiVersion = UR_GRAF_VULKAN_VERSION;

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
			// note: if zero devices returned try adding DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1 to system env variables
			this->Deinitialize();
			return ResultError(Failure, "GrafSystemVulkan: init failed, no physical device found");
		}
		this->vkPhysicalDevices.resize(physicalDeviceCount);
		vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, vkPhysicalDevices.data());
		this->grafPhysicalDeviceDesc.resize(physicalDeviceCount);
		for (ur_uint32 deviceId = 0; deviceId < physicalDeviceCount; ++deviceId)
		{
			VkPhysicalDevice& vkPhysicalDevice = vkPhysicalDevices[deviceId];
			
			VkPhysicalDeviceProperties2 vkDeviceProperties2 = {};
			VkPhysicalDeviceProperties& vkDeviceProperties = vkDeviceProperties2.properties;
			vkDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkDeviceProperties2.pNext = ur_null;
			#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
			VkPhysicalDeviceRayTracingPipelinePropertiesKHR vkDeviceRayTracingProperties = {};
			vkDeviceRayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
			vkDeviceRayTracingProperties.pNext = ur_null;
			vkDeviceProperties2.pNext = &vkDeviceRayTracingProperties;
			#endif
			vkGetPhysicalDeviceProperties2(vkPhysicalDevice, &vkDeviceProperties2);

			VkPhysicalDeviceFeatures2 vkDeviceFeatures2 = {};
			VkPhysicalDeviceFeatures& vkDeviceFeatures = vkDeviceFeatures2.features;
			vkDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			vkDeviceFeatures2.pNext = ur_null;
			#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR vkDeviceRayTracingFeatures = {};
			vkDeviceRayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
			vkDeviceRayTracingFeatures.pNext = ur_null;
			vkDeviceFeatures2.pNext = &vkDeviceRayTracingFeatures;
			VkPhysicalDeviceRayQueryFeaturesKHR vkDeviceRayQueryFeatures = {};
			vkDeviceRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
			vkDeviceRayQueryFeatures.pNext = ur_null;
			vkDeviceRayTracingFeatures.pNext = &vkDeviceRayQueryFeatures;
			#endif
			vkGetPhysicalDeviceFeatures2(vkPhysicalDevice, &vkDeviceFeatures2);

			VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
			vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);

			GrafPhysicalDeviceDesc& grafDeviceDesc = this->grafPhysicalDeviceDesc[deviceId];
			grafDeviceDesc = {};
			grafDeviceDesc.Description = vkDeviceProperties.deviceName;
			grafDeviceDesc.VendorId = (ur_uint)vkDeviceProperties.vendorID;
			grafDeviceDesc.DeviceId = (ur_uint)vkDeviceProperties.deviceID;
			grafDeviceDesc.ConstantBufferOffsetAlignment = (ur_size)vkDeviceProperties.limits.minUniformBufferOffsetAlignment;
			grafDeviceDesc.ImageDataPlacementAlignment = 1;
			grafDeviceDesc.ImageDataPitchAlignment = 1;
			std::vector<VkMemoryPropertyFlags> perHeapFlags(vkDeviceMemoryProperties.memoryHeapCount);
			for (ur_uint32 memTypeIdx = 0; memTypeIdx < vkDeviceMemoryProperties.memoryTypeCount; ++memTypeIdx)
			{
				VkMemoryType& vkMemType = vkDeviceMemoryProperties.memoryTypes[memTypeIdx];
				perHeapFlags[vkMemType.heapIndex] |= vkMemType.propertyFlags;
			}
			for (ur_uint32 memHeapIdx = 0; memHeapIdx < vkDeviceMemoryProperties.memoryHeapCount; ++memHeapIdx)
			{
				VkMemoryHeap& vkMemHeap = vkDeviceMemoryProperties.memoryHeaps[memHeapIdx];
				ur_bool deviceLocal = (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & perHeapFlags[memHeapIdx]);
				ur_bool hostVisible = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & perHeapFlags[memHeapIdx]);
				if (deviceLocal) grafDeviceDesc.LocalMemory += (ur_size)vkMemHeap.size;
				if (deviceLocal && !hostVisible) grafDeviceDesc.LocalMemoryExclusive += (ur_size)vkMemHeap.size;
				if (deviceLocal && hostVisible) grafDeviceDesc.LocalMemoryHostVisible += (ur_size)vkMemHeap.size;
				if (!deviceLocal) grafDeviceDesc.SystemMemory += (ur_size)vkMemHeap.size;
			}

			#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
			grafDeviceDesc.RayTracing.RayTraceSupported = (ur_bool)vkDeviceRayTracingFeatures.rayTracingPipeline;
			grafDeviceDesc.RayTracing.RayQuerySupported = (ur_bool)vkDeviceRayQueryFeatures.rayQuery;
			grafDeviceDesc.RayTracing.ShaderGroupHandleSize = vkDeviceRayTracingProperties.shaderGroupHandleSize;
			grafDeviceDesc.RayTracing.ShaderGroupBaseAlignment = vkDeviceRayTracingProperties.shaderGroupBaseAlignment;
			grafDeviceDesc.RayTracing.RecursionDepthMax = vkDeviceRayTracingProperties.maxRayRecursionDepth;
			grafDeviceDesc.RayTracing.InstanceCountMax = ur_uint64(-1);
			grafDeviceDesc.RayTracing.PrimitiveCountMax = ur_uint64(-1);
			#endif

			#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
			LogNoteGrafDbg(std::string("GrafSystemVulkan: device available ") + grafDeviceDesc.Description +
				", VRAM = " + std::to_string(grafDeviceDesc.LocalMemory >> 20) + " Mb");
			#endif
		}

		// setup debug utils
		#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)

		VkDebugUtilsMessengerCreateInfoEXT vkDebugUtilsInfo = {};
		vkDebugUtilsInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		vkDebugUtilsInfo.flags = 0;
		vkDebugUtilsInfo.messageSeverity = (
			std::max(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, UR_GRAF_VULKAN_DEBUG_MSG_SEVIRITY_MIN) |
			std::max(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, UR_GRAF_VULKAN_DEBUG_MSG_SEVIRITY_MIN) |
			std::max(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, UR_GRAF_VULKAN_DEBUG_MSG_SEVIRITY_MIN) |
			std::max(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, UR_GRAF_VULKAN_DEBUG_MSG_SEVIRITY_MIN));
		vkDebugUtilsInfo.messageType = (
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
		vkDebugUtilsInfo.pfnUserCallback = GrafSystemVulkan::DebugUtilsCallback;
		vkDebugUtilsInfo.pUserData = reinterpret_cast<void*>(&this->GetRealm().GetLog());

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->vkInstance, "vkCreateDebugUtilsMessengerEXT");
		if (vkCreateDebugUtilsMessengerEXT != ur_null)
		{
			res = vkCreateDebugUtilsMessengerEXT(this->vkInstance, &vkDebugUtilsInfo, ur_null, &this->vkDebugUtilsMessenger);
			if (res != VK_SUCCESS)
			{
				return ResultError(Failure, std::string("GrafSystemVulkan: vkCreateDebugUtilsMessengerEXT failed with VkResult = ") + VkResultToString(res));
			}
		}
		#endif
		
		return Result(Success);
	}

	VkBool32 GrafSystemVulkan::DebugUtilsCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT				messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
		void*										pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			if (pCallbackData != ur_null)
			{
				Log* log = reinterpret_cast<Log*>(pUserData);
				log->WriteLine(std::string("VulkanValidationLayer: ") + pCallbackData->pMessage);
			}
			#if (UR_GRAF_VULKAN_DEBUG_LAYER_ASSERT)
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				assert(false);
			}
			#endif
		}
		return VK_FALSE;
	}

	Result GrafSystemVulkan::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
	{
		grafDevice.reset(new GrafDeviceVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList)
	{
		grafCommandList.reset(new GrafCommandListVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateFence(std::unique_ptr<GrafFence>& grafFence)
	{
		grafFence.reset(new GrafFenceVulkan(*this));
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

	Result GrafSystemVulkan::CreateImageSubresource(std::unique_ptr<GrafImageSubresource>& grafImageSubresource)
	{
		grafImageSubresource.reset(new GrafImageSubresourceVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer)
	{
		grafBuffer.reset(new GrafBufferVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateSampler(std::unique_ptr<GrafSampler>& grafSampler)
	{
		grafSampler.reset(new GrafSamplerVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateShader(std::unique_ptr<GrafShader>& grafShader)
	{
		grafShader.reset(new GrafShaderVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateShaderLib(std::unique_ptr<GrafShaderLib>& grafShaderLib)
	{
		grafShaderLib.reset(new GrafShaderLibVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass)
	{
		grafRenderPass.reset(new GrafRenderPassVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget)
	{
		grafRenderTarget.reset(new GrafRenderTargetVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout)
	{
		grafDescriptorTableLayout.reset(new GrafDescriptorTableLayoutVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable)
	{
		grafDescriptorTable.reset(new GrafDescriptorTableVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline)
	{
		grafPipeline.reset(new GrafPipelineVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateComputePipeline(std::unique_ptr<GrafComputePipeline>& grafComputePipeline)
	{
		grafComputePipeline.reset(new GrafComputePipelineVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline)
	{
		grafRayTracingPipeline.reset(new GrafRayTracingPipelineVulkan(*this));
		return Result(Success);
	}

	Result GrafSystemVulkan::CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct)
	{
		grafAccelStruct.reset(new GrafAccelerationStructureVulkan(*this));
		return Result(Success);
	}

	const char* GrafSystemVulkan::GetShaderExtension() const
	{
		static const char* VulkanShaderExt = "spv";
		return VulkanShaderExt;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDeviceVulkan::GrafDeviceVulkan(GrafSystem &grafSystem) :
		GrafDevice(grafSystem)
	{
		this->vkDevice = VK_NULL_HANDLE;
		this->vmaAllocator = VK_NULL_HANDLE;
		this->vkDescriptorPool = VK_NULL_HANDLE;
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

		if (!this->graphicsCommandPools.empty())
		{
			for (auto& poolEntry : this->graphicsCommandPools)
			{
				vkDestroyCommandPool(this->vkDevice, poolEntry.second->vkCommandPool, ur_null);
			}
			this->graphicsCommandPools.clear();
		}

		if (!this->computeCommandPools.empty())
		{
			for (auto& poolEntry : this->computeCommandPools)
			{
				vkDestroyCommandPool(this->vkDevice, poolEntry.second->vkCommandPool, ur_null);
			}
			this->computeCommandPools.clear();
		}

		if (!this->transferCommandPools.empty())
		{
			for (auto& poolEntry : this->transferCommandPools)
			{
				vkDestroyCommandPool(this->vkDevice, poolEntry.second->vkCommandPool, ur_null);
			}
			this->transferCommandPools.clear();
		}

		if (this->vmaAllocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(this->vmaAllocator);
			this->vmaAllocator = VK_NULL_HANDLE;
		}

		if (this->vkDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(this->vkDevice, this->vkDescriptorPool, ur_null);
			this->vkDescriptorPool = VK_NULL_HANDLE;
		}

		if (this->vkDevice != VK_NULL_HANDLE)
		{
			#if (UR_GRAF_VULKAN_IMPLICIT_WAIT_DEVICE)
			this->WaitIdle();
			#endif
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

		// enumerate extentions

		ur_uint32 vkExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, ur_null, &vkExtensionCount, ur_null);
		std::vector<VkExtensionProperties> vkExtensionProperties(vkExtensionCount);
		vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, ur_null, &vkExtensionCount, vkExtensionProperties.data());
		LogNoteGrafDbg("GrafDeviceVulkan: extensions available:");
		for (auto& extensionProperty : vkExtensionProperties)
		{
			LogNoteGrafDbg(extensionProperty.extensionName);
		}

		// verify extensions

		for (auto& extensionRequired : VulkanDeviceExtensions)
		{
			ur_bool extensionAvailable = false;
			for (auto& extensionProperty : vkExtensionProperties)
			{
				extensionAvailable = (strcmp(extensionProperty.extensionName, extensionRequired) == 0);
				if (extensionAvailable) break;
			}
			if (!extensionAvailable)
			{
				return ResultError(Failure, std::string("GrafDeviceVulkan: requested extension is not supported = ") + std::string(extensionRequired));
			}
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

		VkDeviceCreateInfo vkDeviceInfo = {};
		vkDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		vkDeviceInfo.pNext = ur_null;
		vkDeviceInfo.flags = 0;
		vkDeviceInfo.queueCreateInfoCount = 1;
		vkDeviceInfo.pQueueCreateInfos = &vkQueueInfo;
		vkDeviceInfo.enabledLayerCount = (VulkanLayers ? ur_array_size(VulkanLayers) : 0);
		vkDeviceInfo.ppEnabledLayerNames = VulkanLayers;
		vkDeviceInfo.enabledExtensionCount = (VulkanDeviceExtensions ? ur_array_size(VulkanDeviceExtensions) : 0);
		vkDeviceInfo.ppEnabledExtensionNames = VulkanDeviceExtensions;
		vkDeviceInfo.pEnabledFeatures = ur_null;

		// enable required & supported features
		const GrafPhysicalDeviceDesc* grafDeviceDesc = grafSystemVulkan.GetPhysicalDeviceDesc(deviceId);
		void** ppCrntCreateNextPtr = const_cast<void**>(&vkDeviceInfo.pNext);

		VkPhysicalDeviceDescriptorIndexingFeatures vkDescriptorIndexingFeatures = {};
		vkDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		vkDescriptorIndexingFeatures.pNext = ur_null;
		vkDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = true;
		vkDescriptorIndexingFeatures.descriptorBindingPartiallyBound = true;
		vkDescriptorIndexingFeatures.runtimeDescriptorArray = true;
		vkDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = true;
		vkDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = true;
		vkDescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = true;
		*ppCrntCreateNextPtr = &vkDescriptorIndexingFeatures;
		ppCrntCreateNextPtr = &vkDescriptorIndexingFeatures.pNext;
		
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)

		VkPhysicalDeviceFeatures2 vkDeviceFeatures2 = {};
		vkDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		vkDeviceFeatures2.pNext = ur_null;
		*ppCrntCreateNextPtr = &vkDeviceFeatures2;
		ppCrntCreateNextPtr = &vkDeviceFeatures2.pNext;

		VkPhysicalDeviceBufferDeviceAddressFeatures vkBufferDeviceAddressFeatures = {};
		vkBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		vkBufferDeviceAddressFeatures.pNext = ur_null;
		vkBufferDeviceAddressFeatures.bufferDeviceAddress = true;
		*ppCrntCreateNextPtr = &vkBufferDeviceAddressFeatures;
		ppCrntCreateNextPtr = &vkBufferDeviceAddressFeatures.pNext;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR vkDeviceRayTracingFeatures = {};
		vkDeviceRayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		vkDeviceRayTracingFeatures.rayTracingPipeline = grafDeviceDesc->RayTracing.RayTraceSupported;
		vkDeviceRayTracingFeatures.rayTracingPipelineTraceRaysIndirect = grafDeviceDesc->RayTracing.RayTraceSupported;
		vkDeviceRayTracingFeatures.pNext = ur_null;
		*ppCrntCreateNextPtr = &vkDeviceRayTracingFeatures;
		ppCrntCreateNextPtr = &vkDeviceRayTracingFeatures.pNext;

		VkPhysicalDeviceRayQueryFeaturesKHR vkDeviceRayQueryFeatures = {};
		vkDeviceRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		vkDeviceRayQueryFeatures.pNext = ur_null;
		vkDeviceRayQueryFeatures.rayQuery = grafDeviceDesc->RayTracing.RayQuerySupported;
		*ppCrntCreateNextPtr = &vkDeviceRayQueryFeatures;
		ppCrntCreateNextPtr = &vkDeviceRayQueryFeatures.pNext;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR vkDeviceAccelStructFeatures = {};
		vkDeviceAccelStructFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		vkDeviceAccelStructFeatures.pNext = ur_null;
		vkDeviceAccelStructFeatures.accelerationStructure = grafDeviceDesc->RayTracing.RayTraceSupported;
		*ppCrntCreateNextPtr = &vkDeviceAccelStructFeatures;
		ppCrntCreateNextPtr = &vkDeviceAccelStructFeatures.pNext;

		#endif // UR_GRAF_VULKAN_RAY_TRACING_KHR

		VkResult res = vkCreateDevice(vkPhysicalDevice, &vkDeviceInfo, ur_null, &this->vkDevice);
		if (res != VK_SUCCESS)
		{
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkCreateDevice failed with VkResult = ") + VkResultToString(res));
		}
		LogNoteGrafDbg(std::string("GrafDeviceVulkan: VkDevice created for ") + grafDeviceDesc->Description);

		// initialize device extension functions

		#if defined(UR_GRAF_VULKAN_DEBUG_LABLES)
		vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(this->vkDevice, "vkSetDebugUtilsObjectNameEXT");
		vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetDeviceProcAddr(this->vkDevice, "vkSetDebugUtilsObjectTagEXT");
		vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(this->vkDevice, "vkCmdBeginDebugUtilsLabelEXT");
		vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(this->vkDevice, "vkCmdEndDebugUtilsLabelEXT");
		vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(this->vkDevice, "vkCmdInsertDebugUtilsLabelEXT");
		#endif

		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(this->vkDevice, "vkCreateAccelerationStructureKHR");
		vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(this->vkDevice, "vkDestroyAccelerationStructureKHR");
		vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(this->vkDevice, "vkGetAccelerationStructureBuildSizesKHR");
		vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(this->vkDevice, "vkGetAccelerationStructureDeviceAddressKHR");
		vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(this->vkDevice, "vkCreateRayTracingPipelinesKHR");
		vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(this->vkDevice, "vkGetRayTracingShaderGroupHandlesKHR");
		vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(this->vkDevice, "vkCmdBuildAccelerationStructuresKHR");
		vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(this->vkDevice, "vkCmdTraceRaysKHR");
		#endif

		// create vulkan memory allocator
		#if (UR_GRAF_VULKAN_VMA_ENABLED)

		VmaAllocatorCreateInfo vmaAllocatorInfo = {};
		vmaAllocatorInfo.flags = 0;
		vmaAllocatorInfo.physicalDevice = vkPhysicalDevice;
		vmaAllocatorInfo.device = this->vkDevice;
		vmaAllocatorInfo.preferredLargeHeapBlockSize = 0;
		vmaAllocatorInfo.pHeapSizeLimit = ur_null;
		vmaAllocatorInfo.instance = grafSystemVulkan.GetVkInstance();
		vmaAllocatorInfo.vulkanApiVersion = UR_GRAF_VULKAN_VERSION;

		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		vmaAllocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		#endif

		res = vmaCreateAllocator(&vmaAllocatorInfo, &this->vmaAllocator);
		if (res != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceVulkan: vmaCreateAllocator failed with VkResult = ") + VkResultToString(res));
		}
		#endif

		// create common descriptor pool

		std::vector<VkDescriptorPoolSize> vkDescriptorPoolSizes;
		ur_size descriptorPoolSizeCount = ur_array_size(VulkanDescriptorPoolSize);
		vkDescriptorPoolSizes.reserve(descriptorPoolSizeCount);
		for (ur_size i = 0; i < descriptorPoolSizeCount; ++i)
		{
			if (VulkanDescriptorPoolSize[i].Size > 0)
			{
				vkDescriptorPoolSizes.push_back({ VulkanDescriptorPoolSize[i].Type, ur_uint32(VulkanDescriptorPoolSize[i].Size) });
			}
		}

		VkDescriptorPoolCreateInfo vkDescriptorPoolInfo = {};
		vkDescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		vkDescriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		vkDescriptorPoolInfo.maxSets = (ur_uint32)VulkanDescriptorPoolMaxSetCount;
		vkDescriptorPoolInfo.poolSizeCount = (ur_uint32)vkDescriptorPoolSizes.size();
		vkDescriptorPoolInfo.pPoolSizes = vkDescriptorPoolSizes.data();
		VkResult vkRes = vkCreateDescriptorPool(this->vkDevice, &vkDescriptorPoolInfo, ur_null, &this->vkDescriptorPool);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkCreateDescriptorPool failed with VkResult = ") + VkResultToString(vkRes));
		}

		// pre-initialize default command pool for current thread

		ThreadCommandPool* graphicsCommandPool = GetVkGraphicsCommandPool();
		if (ur_null == graphicsCommandPool || VK_NULL_HANDLE == graphicsCommandPool->vkCommandPool)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkCreateCommandPool failed with VkResult = ") + VkResultToString(res));
		}

		return Result(Success);
	}

	GrafDeviceVulkan::ThreadCommandPool* GrafDeviceVulkan::GetVkGraphicsCommandPool()
	{
		std::lock_guard<std::mutex> lock(this->graphicsCommandPoolsMutex);

		// try to find exisitng command pool for current thread

		std::thread::id thisThreadId = std::this_thread::get_id();
		const auto& poolIter = this->graphicsCommandPools.find(thisThreadId);
		if (poolIter != this->graphicsCommandPools.end())
		{
			return poolIter->second.get();
		}

		// create graphics queue command pool

		VkCommandPoolCreateInfo vkCommandPoolInfo = {};
		vkCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		vkCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // command buffers will be resetted implicitly at vkBeginCommandBuffer
		vkCommandPoolInfo.queueFamilyIndex = this->deviceGraphicsQueueId;

		VkCommandPool vkGraphicsCommandPool;
		VkResult res = vkCreateCommandPool(this->vkDevice, &vkCommandPoolInfo, ur_null, &vkGraphicsCommandPool);
		if (vkGraphicsCommandPool != VK_NULL_HANDLE)
		{
			std::unique_ptr<ThreadCommandPool> threadCommandPool(new ThreadCommandPool());
			threadCommandPool->vkCommandPool = vkGraphicsCommandPool;
			this->graphicsCommandPools[thisThreadId] = std::move(threadCommandPool);
		}

		return this->graphicsCommandPools[thisThreadId].get();
	}

	GrafDeviceVulkan::ThreadCommandPool* GrafDeviceVulkan::GetVkComputeCommandPool()
	{
		std::lock_guard<std::mutex> lock(this->computeCommandPoolsMutex);

		// not supported

		return ur_null;
	}

	GrafDeviceVulkan::ThreadCommandPool* GrafDeviceVulkan::GetVkTransferCommandPool()
	{
		std::lock_guard<std::mutex> lock(this->transferCommandPoolsMutex);

		// not supported

		return ur_null;
	}

	Result GrafDeviceVulkan::Record(GrafCommandList* grafCommandList)
	{
		std::lock_guard<std::mutex> lock(this->graphicsCommandListsMutex);

		this->graphicsCommandLists.push_back(grafCommandList);

		return Result(Success);
	}

	Result GrafDeviceVulkan::Submit()
	{
		// NOTE: support submission to different queue families
		// currently everything's done on the graphics queue
		
		VkQueue vkSubmissionQueue;
		vkGetDeviceQueue(this->vkDevice, this->deviceGraphicsQueueId, 0, &vkSubmissionQueue);

		VkSubmitInfo vkSubmitInfo = {};
		vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vkSubmitInfo.waitSemaphoreCount = 0;
		vkSubmitInfo.pWaitSemaphores = ur_null;
		vkSubmitInfo.pWaitDstStageMask = ur_null;
		vkSubmitInfo.commandBufferCount = 0;
		vkSubmitInfo.pCommandBuffers = ur_null;
		vkSubmitInfo.signalSemaphoreCount = 0;
		vkSubmitInfo.pSignalSemaphores = ur_null;

		this->graphicsCommandListsMutex.lock();
		this->graphicsCommandPoolsMutex.lock();
		for (auto &threadCmdPool : this->graphicsCommandPools) threadCmdPool.second->accessMutex.lock();

		VkResult vkRes(VK_SUCCESS);
		for (auto& grafCommandList : this->graphicsCommandLists)
		{
			GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(grafCommandList);
			VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();

			vkSubmitInfo.commandBufferCount = 1;
			vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;

			vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
			if (vkRes != VK_SUCCESS)
				break;
		}
		this->graphicsCommandLists.clear();

		for (auto &threadCmdPool : this->graphicsCommandPools) threadCmdPool.second->accessMutex.unlock();
		this->graphicsCommandPoolsMutex.unlock();
		this->graphicsCommandListsMutex.unlock();

		if (vkRes != VK_SUCCESS)
			ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));

		return Result(Success);
	}

	Result GrafDeviceVulkan::WaitIdle()
	{
		VkResult res = vkDeviceWaitIdle(this->vkDevice);
		if (res != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkDeviceWaitIdle failed with VkResult = ") + VkResultToString(res));
		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafCommandListVulkan::GrafCommandListVulkan(GrafSystem &grafSystem) :
		GrafCommandList(grafSystem)
	{
		this->commandPool = ur_null;
		this->vkCommandBuffer = VK_NULL_HANDLE;
		this->vkSubmitFence = VK_NULL_HANDLE;
		this->isOpen = false;
	}

	GrafCommandListVulkan::~GrafCommandListVulkan()
	{
		this->Deinitialize();
	}

	Result GrafCommandListVulkan::Deinitialize()
	{
		if (this->vkSubmitFence != VK_NULL_HANDLE)
		{
			#if (UR_GRAF_VULKAN_COMMAND_BUFFER_SYNC_DESTROY)
			vkWaitForFences(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), 1, &this->vkSubmitFence, true, ~ur_uint64(0));
			#endif
			vkDestroyFence(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkSubmitFence, ur_null);
			this->vkSubmitFence = VK_NULL_HANDLE;
		}

		if (this->vkCommandBuffer != VK_NULL_HANDLE)
		{
			GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
			std::lock_guard<std::recursive_mutex> lockPool(this->commandPool->accessMutex);
			vkFreeCommandBuffers(grafDeviceVulkan->GetVkDevice(), this->commandPool->vkCommandPool, 1, &this->vkCommandBuffer);
			this->vkCommandBuffer = VK_NULL_HANDLE;
		}

		this->commandPool = ur_null;
		this->isOpen = false;

		return Result(Success);
	}

	Result GrafCommandListVulkan::Initialize(GrafDevice *grafDevice)
	{
		this->Deinitialize();

		GrafCommandList::Initialize(grafDevice);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafCommandListVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// allocate command bufer

		this->commandPool = grafDeviceVulkan->GetVkGraphicsCommandPool();
		if (ur_null == this->commandPool)
		{
			return ResultError(InvalidArgs, std::string("GrafCommandListVulkan: failed to initialize, invalid command pool"));
		}

		VkCommandBufferAllocateInfo vkCommandBufferInfo = {};
		vkCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		vkCommandBufferInfo.commandPool = this->commandPool->vkCommandPool;
		vkCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkCommandBufferInfo.commandBufferCount = 1;

		VkResult vkRes(VK_SUCCESS);
		{
			std::lock_guard<std::recursive_mutex> lockPool(this->commandPool->accessMutex);
			vkRes = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferInfo, &this->vkCommandBuffer);
		}
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafCommandListVulkan: vkAllocateCommandBuffers failed with VkResult = ") + VkResultToString(vkRes));

		// submission sync object

		VkFenceCreateInfo vkFenceInfo = {};
		vkFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		vkRes = vkCreateFence(vkDevice, &vkFenceInfo, ur_null, &this->vkSubmitFence);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafCommandListVulkan: vkCreateFence failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	Result GrafCommandListVulkan::Begin()
	{
		if (VK_NULL_HANDLE == this->vkCommandBuffer)
			return Result(NotInitialized);

		if (this->isOpen)
			return Result(Success);

		this->isOpen = true;

		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();
		#if (UR_GRAF_VULKAN_COMMAND_BUFFER_SYNC_RESET)
		vkWaitForFences(vkDevice, 1, &this->vkSubmitFence, true, ~ur_uint64(0)); // make sure command buffer is no longer used (previous submission can still be executed)
		#endif
		vkResetFences(vkDevice, 1, &this->vkSubmitFence);

		VkCommandBufferBeginInfo vkBeginInfo = {};
		vkBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		this->commandPool->accessMutex.lock();
		VkResult vkRes = vkBeginCommandBuffer(this->vkCommandBuffer, &vkBeginInfo);
		if (vkRes != VK_SUCCESS)
		{
			this->commandPool->accessMutex.unlock();
			return ResultError(Failure, std::string("GrafCommandListVulkan: vkBeginCommandBuffer failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	Result GrafCommandListVulkan::End()
	{
		if (VK_NULL_HANDLE == this->vkCommandBuffer)
			return Result(NotInitialized);
		
		VkResult vkRes = vkEndCommandBuffer(this->vkCommandBuffer);
		this->commandPool->accessMutex.unlock();
		this->isOpen = false;
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafCommandListVulkan: vkEndCommandBuffer failed with VkResult = ") + VkResultToString(vkRes));

		return Result(Success);
	}

	Result GrafCommandListVulkan::Wait(ur_uint64 timeout)
	{
		if (VK_NULL_HANDLE == this->vkCommandBuffer)
			return Result(NotInitialized);

		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();
		VkResult vkRes = vkWaitForFences(vkDevice, 1, &this->vkSubmitFence, true, timeout);

		return Result(vkRes == VK_SUCCESS ? Success : TimeOut);
	}

	Result GrafCommandListVulkan::InsertDebugLabel(const char* name, const ur_float4& color)
	{
	#if (UR_GRAF_VULKAN_DEBUG_LABLES)
		if (ur_null == vkCmdInsertDebugUtilsLabelEXT)
			return Result(NotImplemented);

		VkDebugUtilsLabelEXT vkDebugUtilsLabel = {};
		vkDebugUtilsLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		vkDebugUtilsLabel.pNext = ur_null;
		vkDebugUtilsLabel.pLabelName = name;
		vkDebugUtilsLabel.color[0] = color[0];
		vkDebugUtilsLabel.color[1] = color[1];
		vkDebugUtilsLabel.color[2] = color[2];
		vkDebugUtilsLabel.color[3] = color[3];

		vkCmdInsertDebugUtilsLabelEXT(this->vkCommandBuffer, &vkDebugUtilsLabel);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafCommandListVulkan::BeginDebugLabel(const char* name, const ur_float4& color)
	{
	#if (UR_GRAF_VULKAN_DEBUG_LABLES)
		if (ur_null == vkCmdBeginDebugUtilsLabelEXT)
			return Result(NotImplemented);

		VkDebugUtilsLabelEXT vkDebugUtilsLabel = {};
		vkDebugUtilsLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		vkDebugUtilsLabel.pNext = ur_null;
		vkDebugUtilsLabel.pLabelName = name;
		vkDebugUtilsLabel.color[0] = color[0];
		vkDebugUtilsLabel.color[1] = color[1];
		vkDebugUtilsLabel.color[2] = color[2];
		vkDebugUtilsLabel.color[3] = color[3];
		
		vkCmdBeginDebugUtilsLabelEXT(this->vkCommandBuffer, &vkDebugUtilsLabel);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafCommandListVulkan::EndDebugLabel()
	{
	#if (UR_GRAF_VULKAN_DEBUG_LABLES)
		if (ur_null == vkCmdEndDebugUtilsLabelEXT)
			return Result(NotImplemented);

		vkCmdEndDebugUtilsLabelEXT(this->vkCommandBuffer);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafCommandListVulkan::BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState)
	{
		if (ur_null == grafBuffer)
			return Result(InvalidArgs);

		if (grafBuffer->GetState() == dstState)
			return Result(Success);

		srcState = (GrafBufferState::Current == srcState ? grafBuffer->GetState() : srcState);

		VkBufferMemoryBarrier vkBufferBarrier = {};
		vkBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vkBufferBarrier.srcAccessMask = 0;
		vkBufferBarrier.dstAccessMask = 0;
		vkBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBufferBarrier.buffer = static_cast<GrafBufferVulkan*>(grafBuffer)->GetVkBuffer();
		vkBufferBarrier.offset = 0;
		vkBufferBarrier.size = VK_WHOLE_SIZE;
		VkPipelineStageFlags vkStageSrc = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags vkStageDst = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		switch (srcState)
		{
		case GrafBufferState::TransferSrc:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafBufferState::TransferDst:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafBufferState::VertexBuffer:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case GrafBufferState::IndexBuffer:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case GrafBufferState::ConstantBuffer:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // wait last graphics shader stage
			break;
		case GrafBufferState::ShaderRead:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // wait last graphics shader stage
			break;
		case GrafBufferState::ShaderReadWrite:
			vkBufferBarrier.srcAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // wait last graphics shader stage
			break;
		case GrafBufferState::ComputeConstantBuffer:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case GrafBufferState::ComputeRead:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case GrafBufferState::ComputeReadWrite:
			vkBufferBarrier.srcAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		case GrafBufferState::RayTracingConstantBuffer:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		case GrafBufferState::RayTracingRead:
			vkBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		case GrafBufferState::RayTracingReadWrite:
			vkBufferBarrier.srcAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		#endif
		};

		switch (dstState)
		{
		case GrafBufferState::TransferSrc:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafBufferState::TransferDst:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkStageDst = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafBufferState::VertexBuffer:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case GrafBufferState::IndexBuffer:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case GrafBufferState::ConstantBuffer:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // must be ready before first graphics shader stage
			break;
		case GrafBufferState::ShaderRead:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // must be ready before first graphics shader stage
			break;
		case GrafBufferState::ShaderReadWrite:
			vkBufferBarrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // must be ready before first graphics shader stage
			break;
		case GrafBufferState::ComputeConstantBuffer:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case GrafBufferState::ComputeRead:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case GrafBufferState::ComputeReadWrite:
			vkBufferBarrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		case GrafBufferState::RayTracingConstantBuffer:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		case GrafBufferState::RayTracingRead:
			vkBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		case GrafBufferState::RayTracingReadWrite:
			vkBufferBarrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		#endif
		};

		vkCmdPipelineBarrier(this->vkCommandBuffer, vkStageSrc, vkStageDst, VkDependencyFlags(0), 0, ur_null, 1, &vkBufferBarrier, 0, ur_null);

		static_cast<GrafBufferVulkan*>(grafBuffer)->SetState(dstState);

		return Result(Success);
	}

	Result GrafCommandListVulkan::ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState)
	{
		if (ur_null == grafImage)
			return Result(InvalidArgs);

		GrafImageVulkan* grafImageVulkan = static_cast<GrafImageVulkan*>(grafImage);
		Result res = this->ImageMemoryBarrier(grafImageVulkan->GetDefaultSubresource(), srcState, dstState);
		if (Succeeded(res))
		{
			// image state = default subresource state (all mips/layers)
			static_cast<GrafImageVulkan*>(grafImage)->SetState(dstState);
		}

		return res;
	}

	Result GrafCommandListVulkan::ImageMemoryBarrier(GrafImageSubresource* grafImageSubresource, GrafImageState srcState, GrafImageState dstState)
	{
		if (ur_null == grafImageSubresource)
			return Result(InvalidArgs);

		const GrafImageVulkan* grafImageVulkan = static_cast<const GrafImageVulkan*>(grafImageSubresource->GetImage());
		if (ur_null == grafImageVulkan)
			return Result(InvalidArgs);

		srcState = (GrafImageState::Current == srcState ? grafImageSubresource->GetState() : srcState);

		VkImageMemoryBarrier vkImageBarrier = {};
		vkImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkImageBarrier.srcAccessMask = 0;
		vkImageBarrier.dstAccessMask = 0;
		vkImageBarrier.oldLayout = GrafUtilsVulkan::GrafToVkImageLayout(srcState);
		vkImageBarrier.newLayout = GrafUtilsVulkan::GrafToVkImageLayout(dstState);
		vkImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkImageBarrier.image = grafImageVulkan->GetVkImage();
		vkImageBarrier.subresourceRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(grafImageVulkan->GetDesc().Usage);
		vkImageBarrier.subresourceRange.baseMipLevel = grafImageSubresource->GetDesc().BaseMipLevel;
		vkImageBarrier.subresourceRange.levelCount = grafImageSubresource->GetDesc().LevelCount;
		vkImageBarrier.subresourceRange.baseArrayLayer = grafImageSubresource->GetDesc().BaseArrayLayer;
		vkImageBarrier.subresourceRange.layerCount = grafImageSubresource->GetDesc().LayerCount;
		VkPipelineStageFlags vkStageSrc = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags vkStageDst = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		switch (srcState)
		{
		case GrafImageState::TransferSrc:
			vkImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::TransferDst:
			vkImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::ColorClear:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_TRANSFER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::ColorWrite:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case GrafImageState::DepthStencilClear:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_TRANSFER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::DepthStencilWrite:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // wait last depth/stencil stage
			break;
		case GrafImageState::DepthStencilRead:
			vkImageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // wait last depth/stencil stage
			break;
		case GrafImageState::ShaderRead:
			vkImageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // wait last graphics shader stage
			break;
		case GrafImageState::ShaderReadWrite:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // wait last graphics shader stage
			break;
		case GrafImageState::ComputeRead:
			vkImageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case GrafImageState::ComputeReadWrite:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		case GrafImageState::RayTracingRead:
			vkImageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageSrc = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		case GrafImageState::RayTracingReadWrite:
			vkImageBarrier.srcAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageSrc = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		#endif
		};

		switch (dstState)
		{
		case GrafImageState::TransferSrc:
			vkImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::TransferDst:
			vkImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkStageDst = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::ColorClear:
			vkImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkStageDst = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::ColorWrite:
			vkImageBarrier.dstAccessMask = (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case GrafImageState::DepthStencilClear:
			vkImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkStageDst = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case GrafImageState::DepthStencilWrite:
			vkImageBarrier.dstAccessMask = (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // must be ready before early depth/stencil stage
			break;
		case GrafImageState::DepthStencilRead:
			vkImageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // must be ready before early depth/stencil stage
			break;
		case GrafImageState::ShaderRead:
			vkImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // must be ready before first graphics shader stage
			break;
		case GrafImageState::ShaderReadWrite:
			vkImageBarrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // must be ready before first graphics shader stage
			break;
		case GrafImageState::ComputeRead:
			vkImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case GrafImageState::ComputeReadWrite:
			vkImageBarrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		case GrafImageState::RayTracingRead:
			vkImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkStageDst = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		case GrafImageState::RayTracingReadWrite:
			vkImageBarrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
			vkStageDst = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
			break;
		#endif
		};

		vkCmdPipelineBarrier(this->vkCommandBuffer, vkStageSrc, vkStageDst, VkDependencyFlags(0), 0, ur_null, 0, ur_null, 1, &vkImageBarrier);

		static_cast<GrafImageSubresourceVulkan*>(grafImageSubresource)->SetState(dstState);

		return Result(Success);
	}

	Result GrafCommandListVulkan::SetFenceState(GrafFence* grafFence, GrafFenceState state)
	{
		if (ur_null == grafFence)
			return Result(InvalidArgs);

		VkPipelineStageFlags vkPipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkEvent vkFenceEvent = static_cast<GrafFenceVulkan*>(grafFence)->GetVkEvent();
		switch (state)
		{
		case GrafFenceState::Signaled:
			vkCmdSetEvent(this->vkCommandBuffer, vkFenceEvent, vkPipelineStage);
			break;
		case GrafFenceState::Reset:
			vkCmdResetEvent(this->vkCommandBuffer, vkFenceEvent, vkPipelineStage);
			break;
		default:
			return ResultError(InvalidArgs, "GrafCommandListVulkan: SetFenceState failed, invalid GrafFenceState");
		};

		return Result(Success);
	}

	Result GrafCommandListVulkan::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		if (ur_null == grafImage)
			return Result(InvalidArgs);

		GrafImageVulkan* grafImageVulkan = static_cast<GrafImageVulkan*>(grafImage);
		return ClearColorImage(grafImageVulkan->GetDefaultSubresource(), clearValue);
	}

	Result GrafCommandListVulkan::ClearColorImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue)
	{
		if (ur_null == grafImageSubresource)
			return Result(InvalidArgs);

		const GrafImage* grafImage = grafImageSubresource->GetImage();
		VkImage vkImage = static_cast<const GrafImageVulkan*>(grafImage)->GetVkImage();
		VkImageLayout vkImageLayout = GrafUtilsVulkan::GrafToVkImageLayout(grafImageSubresource->GetState());
		VkClearColorValue& vkClearValue = reinterpret_cast<VkClearColorValue&>(clearValue);
		VkImageSubresourceRange vkClearRange = {};
		vkClearRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(grafImage->GetDesc().Usage);
		vkClearRange.baseMipLevel = grafImageSubresource->GetDesc().BaseMipLevel;
		vkClearRange.levelCount = grafImageSubresource->GetDesc().LevelCount;
		vkClearRange.baseArrayLayer = grafImageSubresource->GetDesc().BaseArrayLayer;
		vkClearRange.layerCount = grafImageSubresource->GetDesc().LayerCount;

		vkCmdClearColorImage(this->vkCommandBuffer, vkImage, vkImageLayout, &vkClearValue, 1, &vkClearRange);

		return Result(Success);
	}

	Result GrafCommandListVulkan::ClearDepthStencilImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		if (ur_null == grafImage)
			return Result(InvalidArgs);

		GrafImageVulkan* grafImageVulkan = static_cast<GrafImageVulkan*>(grafImage);
		return ClearDepthStencilImage(grafImageVulkan->GetDefaultSubresource(), clearValue);
	}

	Result GrafCommandListVulkan::ClearDepthStencilImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue)
	{
		if (ur_null == grafImageSubresource)
			return Result(InvalidArgs);

		const GrafImage* grafImage = grafImageSubresource->GetImage();

		VkImage vkImage = static_cast<const GrafImageVulkan*>(grafImage)->GetVkImage();
		VkImageLayout vkImageLayout = GrafUtilsVulkan::GrafToVkImageLayout(grafImageSubresource->GetState());
		VkClearDepthStencilValue vkClearValue = {};
		vkClearValue.depth = clearValue.f32[0];
		vkClearValue.stencil = clearValue.u32[1];
		VkImageSubresourceRange vkClearRange = {};
		vkClearRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(grafImage->GetDesc().Usage);
		vkClearRange.baseMipLevel = grafImageSubresource->GetDesc().BaseMipLevel;
		vkClearRange.levelCount = grafImageSubresource->GetDesc().LevelCount;
		vkClearRange.baseArrayLayer = grafImageSubresource->GetDesc().BaseArrayLayer;
		vkClearRange.layerCount = grafImageSubresource->GetDesc().LayerCount;

		vkCmdClearDepthStencilImage(this->vkCommandBuffer, vkImage, vkImageLayout, &vkClearValue, 1, &vkClearRange);

		return Result(Success);
	}

	Result GrafCommandListVulkan::SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect)
	{
		VkViewport vkViewport = {};
		vkViewport.x = grafViewportDesc.X;
		vkViewport.y = grafViewportDesc.Y;
		vkViewport.width = grafViewportDesc.Width;
		vkViewport.height = grafViewportDesc.Height;
		vkViewport.minDepth = grafViewportDesc.Near;
		vkViewport.maxDepth = grafViewportDesc.Far;

		vkCmdSetViewport(this->vkCommandBuffer, 0, 1, &vkViewport);

		if (resetScissorsRect)
		{
			RectI scissorsRect;
			scissorsRect.Min.x = ur_int(grafViewportDesc.X);
			scissorsRect.Min.y = ur_int(grafViewportDesc.Y);
			scissorsRect.Max.x = ur_int(grafViewportDesc.X + grafViewportDesc.Width);
			scissorsRect.Max.y = ur_int(grafViewportDesc.Y + grafViewportDesc.Height);

			return SetScissorsRect(scissorsRect);
		}

		return Result(Success);
	}

	Result GrafCommandListVulkan::SetScissorsRect(const RectI& scissorsRect)
	{
		if (scissorsRect.IsInsideOut())
			return Result(InvalidArgs);

		VkRect2D vkScissors;
		vkScissors.offset.x = (ur_int32)scissorsRect.Min.x;
		vkScissors.offset.y = (ur_int32)scissorsRect.Min.y;
		vkScissors.extent.width = scissorsRect.Width();
		vkScissors.extent.height = scissorsRect.Height();

		vkCmdSetScissor(this->vkCommandBuffer, 0, 1, &vkScissors);

		return Result(Success);
	}

	Result GrafCommandListVulkan::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues)
	{
		if (ur_null == grafRenderPass || ur_null == grafRenderTarget || 0 == grafRenderTarget->GetImageCount())
			return Result(InvalidArgs);

		std::vector<VkClearValue> vkClearValues;
		if (rtClearValues != ur_null)
		{
			vkClearValues.resize(grafRenderTarget->GetImageCount());
			for (ur_uint iimage = 0; iimage < grafRenderTarget->GetImageCount(); ++iimage)
			{
				if (ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget) & grafRenderTarget->GetImage(iimage)->GetDesc().Usage)
				{
					vkClearValues[iimage].depthStencil.depth = rtClearValues[iimage].f32[0];
					vkClearValues[iimage].depthStencil.stencil = (ur_uint32)rtClearValues[iimage].f32[1];
				}
				else
				{
					vkClearValues[iimage].color = reinterpret_cast<VkClearColorValue&>(rtClearValues[iimage]);
				}
			}
		}

		VkRenderPassBeginInfo vkPassBeginInfo = {};
		vkPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkPassBeginInfo.renderPass = static_cast<GrafRenderPassVulkan*>(grafRenderPass)->GetVkRenderPass();
		vkPassBeginInfo.framebuffer = static_cast<GrafRenderTargetVulkan*>(grafRenderTarget)->GetVkFramebuffer();
		vkPassBeginInfo.renderArea.offset = { 0, 0 };
		vkPassBeginInfo.renderArea.extent.width = (ur_uint32)grafRenderTarget->GetImage(0)->GetDesc().Size.x;
		vkPassBeginInfo.renderArea.extent.height = (ur_uint32)grafRenderTarget->GetImage(0)->GetDesc().Size.y;
		vkPassBeginInfo.clearValueCount = (ur_uint32)vkClearValues.size();
		vkPassBeginInfo.pClearValues = vkClearValues.data();

		vkCmdBeginRenderPass(this->vkCommandBuffer, &vkPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		return Result(Success);
	}

	Result GrafCommandListVulkan::EndRenderPass()
	{
		vkCmdEndRenderPass(this->vkCommandBuffer);

		return Result(Success);
	}

	Result GrafCommandListVulkan::BindPipeline(GrafPipeline* grafPipeline)
	{
		if (ur_null == grafPipeline)
			return Result(InvalidArgs);
		
		vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<GrafPipelineVulkan*>(grafPipeline)->GetVkPipeline());

		return Result(Success);
	}

	Result GrafCommandListVulkan::BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		if (ur_null == descriptorTable || ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafPipelineVulkan* grafPipelineVulkan = static_cast<GrafPipelineVulkan*>(grafPipeline);
		GrafDescriptorTableVulkan* descriptorTableVulkan = static_cast<GrafDescriptorTableVulkan*>(descriptorTable);
		VkDescriptorSet vkDescriptorSets[] = { descriptorTableVulkan->GetVkDescriptorSet() };

		vkCmdBindDescriptorSets(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grafPipelineVulkan->GetVkPipelineLayout(), 0, 1, vkDescriptorSets, 0, ur_null);

		return Result(Success);
	}

	Result GrafCommandListVulkan::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset)
	{
		if (ur_null == grafVertexBuffer)
			return Result(InvalidArgs);

		VkBuffer vkBuffers[] = { static_cast<GrafBufferVulkan*>(grafVertexBuffer)->GetVkBuffer() };
		VkDeviceSize vkOffsets[] = { bufferOffset };
		vkCmdBindVertexBuffers(this->vkCommandBuffer, bindingIdx, 1, vkBuffers, vkOffsets);

		return Result(Success);
	}

	Result GrafCommandListVulkan::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset)
	{
		if (ur_null == grafIndexBuffer)
			return Result(InvalidArgs);

		vkCmdBindIndexBuffer(this->vkCommandBuffer, static_cast<GrafBufferVulkan*>(grafIndexBuffer)->GetVkBuffer(),
			VkDeviceSize(bufferOffset), GrafUtilsVulkan::GrafToVkIndexType(indexType));

		return Result(Success);
	}

	Result GrafCommandListVulkan::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
	{
		vkCmdDraw(this->vkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

		return Result(Success);
	}

	Result GrafCommandListVulkan::DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance)
	{
		vkCmdDrawIndexed(this->vkCommandBuffer, indexCount, instanceCount, firstIndex, firstVertex, firstInstance);

		return Result(Success);
	}

	Result GrafCommandListVulkan::Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (ur_null == srcBuffer || ur_null == dstBuffer)
			return Result(InvalidArgs);

		VkBufferCopy vkBufferCopy = {};
		vkBufferCopy.srcOffset = (VkDeviceSize)srcOffset;
		vkBufferCopy.dstOffset = (VkDeviceSize)dstOffset;
		vkBufferCopy.size = VkDeviceSize(dataSize > 0 ? dataSize : srcBuffer->GetDesc().SizeInBytes);

		vkCmdCopyBuffer(this->vkCommandBuffer,
			static_cast<GrafBufferVulkan*>(srcBuffer)->GetVkBuffer(),
			static_cast<GrafBufferVulkan*>(dstBuffer)->GetVkBuffer(),
			1, &vkBufferCopy);

		return Result(Success);
	}

	Result GrafCommandListVulkan::Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset, BoxI imageRegion)
	{
		if (ur_null == srcBuffer || ur_null == dstImage)
			return Result(InvalidArgs);

		GrafImageVulkan* dstImageVulkan = static_cast<GrafImageVulkan*>(dstImage);
		return Copy(srcBuffer, dstImageVulkan->GetDefaultSubresource(), bufferOffset, imageRegion);
	}

	Result GrafCommandListVulkan::Copy(GrafBuffer* srcBuffer, GrafImageSubresource* dstImageSubresource, ur_size bufferOffset, BoxI imageRegion)
	{
		if (ur_null == srcBuffer || ur_null == dstImageSubresource)
			return Result(InvalidArgs);

		const GrafImage* dstImage = dstImageSubresource->GetImage();

		ur_uint3 subResSize = dstImage->GetDesc().Size;
		subResSize.x = std::max(1u, dstImage->GetDesc().Size.x >> dstImageSubresource->GetDesc().BaseMipLevel);
		subResSize.y = std::max(1u, dstImage->GetDesc().Size.y >> dstImageSubresource->GetDesc().BaseMipLevel);
		subResSize.z = std::max(1u, dstImage->GetDesc().Size.z >> dstImageSubresource->GetDesc().BaseMipLevel);

		VkBufferImageCopy vkBufferImageCopy = {};
		vkBufferImageCopy.bufferOffset = bufferOffset;
		vkBufferImageCopy.bufferRowLength = 0;
		vkBufferImageCopy.bufferImageHeight = 0;
		vkBufferImageCopy.imageOffset.x = (ur_int32)imageRegion.Min.x;
		vkBufferImageCopy.imageOffset.y = (ur_int32)imageRegion.Min.y;
		vkBufferImageCopy.imageOffset.z = (ur_int32)imageRegion.Min.z;
		vkBufferImageCopy.imageExtent.width = (ur_uint32)(imageRegion.SizeX() > 0 ? imageRegion.SizeX() : subResSize.x);
		vkBufferImageCopy.imageExtent.height = (ur_uint32)(imageRegion.SizeY() > 0 ? imageRegion.SizeY() : subResSize.y);
		vkBufferImageCopy.imageExtent.depth = (ur_uint32)(imageRegion.SizeZ() > 0 ? imageRegion.SizeZ() : subResSize.z);
		vkBufferImageCopy.imageSubresource.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(dstImage->GetDesc().Usage);
		vkBufferImageCopy.imageSubresource.mipLevel = dstImageSubresource->GetDesc().BaseMipLevel;
		vkBufferImageCopy.imageSubresource.baseArrayLayer = dstImageSubresource->GetDesc().BaseArrayLayer;
		vkBufferImageCopy.imageSubresource.layerCount = dstImageSubresource->GetDesc().LayerCount;

		vkCmdCopyBufferToImage(this->vkCommandBuffer,
			static_cast<const GrafBufferVulkan*>(srcBuffer)->GetVkBuffer(),
			static_cast<const GrafImageVulkan*>(dstImage)->GetVkImage(),
			GrafUtilsVulkan::GrafToVkImageLayout(dstImageSubresource->GetState()),
			1, &vkBufferImageCopy);

		return Result(Success);
	}

	Result GrafCommandListVulkan::Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		if (ur_null == srcImage || ur_null == dstBuffer)
			return Result(InvalidArgs);

		GrafImageVulkan* srcImageVulkan = static_cast<GrafImageVulkan*>(srcImage);
		return Copy(srcImageVulkan->GetDefaultSubresource(), dstBuffer, bufferOffset, imageRegion);
	}

	Result GrafCommandListVulkan::Copy(GrafImageSubresource* srcImageSubresource, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		if (ur_null == srcImageSubresource || ur_null == dstBuffer)
			return Result(InvalidArgs);

		const GrafImage* srcImage = srcImageSubresource->GetImage();

		ur_uint3 subResSize = srcImage->GetDesc().Size;
		subResSize.x = std::max(1u, srcImage->GetDesc().Size.x >> srcImageSubresource->GetDesc().BaseMipLevel);
		subResSize.y = std::max(1u, srcImage->GetDesc().Size.y >> srcImageSubresource->GetDesc().BaseMipLevel);
		subResSize.z = std::max(1u, srcImage->GetDesc().Size.z >> srcImageSubresource->GetDesc().BaseMipLevel);

		VkBufferImageCopy vkBufferImageCopy = {};
		vkBufferImageCopy.bufferOffset = bufferOffset;
		vkBufferImageCopy.bufferRowLength = 0;
		vkBufferImageCopy.bufferImageHeight = 0;
		vkBufferImageCopy.imageOffset.x = (ur_int32)imageRegion.Min.x;
		vkBufferImageCopy.imageOffset.y = (ur_int32)imageRegion.Min.y;
		vkBufferImageCopy.imageOffset.z = (ur_int32)imageRegion.Min.z;
		vkBufferImageCopy.imageExtent.width = (ur_uint32)(imageRegion.SizeX() > 0 ? imageRegion.SizeX() : srcImage->GetDesc().Size.x);
		vkBufferImageCopy.imageExtent.height = (ur_uint32)(imageRegion.SizeY() > 0 ? imageRegion.SizeY() : srcImage->GetDesc().Size.y);
		vkBufferImageCopy.imageExtent.depth = (ur_uint32)(imageRegion.SizeZ() > 0 ? imageRegion.SizeZ() : srcImage->GetDesc().Size.z);
		vkBufferImageCopy.imageSubresource.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(srcImage->GetDesc().Usage);
		vkBufferImageCopy.imageSubresource.mipLevel = srcImageSubresource->GetDesc().BaseMipLevel;
		vkBufferImageCopy.imageSubresource.baseArrayLayer = srcImageSubresource->GetDesc().BaseArrayLayer;
		vkBufferImageCopy.imageSubresource.layerCount = srcImageSubresource->GetDesc().LayerCount;

		vkCmdCopyImageToBuffer(this->vkCommandBuffer,
			static_cast<const GrafImageVulkan*>(srcImage)->GetVkImage(),
			GrafUtilsVulkan::GrafToVkImageLayout(srcImageSubresource->GetState()),
			static_cast<const GrafBufferVulkan*>(dstBuffer)->GetVkBuffer(),
			1, &vkBufferImageCopy);

		return Result(Success);
	}

	Result GrafCommandListVulkan::Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion, BoxI dstRegion)
	{
		if (ur_null == srcImage || ur_null == dstImage)
			return Result(InvalidArgs);

		GrafImageVulkan* srcImageVulkan = static_cast<GrafImageVulkan*>(srcImage);
		GrafImageVulkan* dstImageVulkan = static_cast<GrafImageVulkan*>(dstImage);
		return Copy(srcImageVulkan->GetDefaultSubresource(), dstImageVulkan->GetDefaultSubresource(), srcRegion, dstRegion);
	}

	Result GrafCommandListVulkan::Copy(GrafImageSubresource* srcImageSubresource, GrafImageSubresource* dstImageSubresource, BoxI srcRegion, BoxI dstRegion)
	{
		if (ur_null == srcImageSubresource || ur_null == dstImageSubresource)
			return Result(InvalidArgs);

		ur_int3 srcSize = srcRegion.Size();
		ur_int3 dstSize = dstRegion.Size();
		if (srcSize != dstSize)
			return Result(InvalidArgs);

		const GrafImage* srcImage = srcImageSubresource->GetImage();
		const GrafImage* dstImage = dstImageSubresource->GetImage();

		ur_uint3 subResSize = srcImage->GetDesc().Size;
		subResSize.x = std::max(1u, srcImage->GetDesc().Size.x >> srcImageSubresource->GetDesc().BaseMipLevel);
		subResSize.y = std::max(1u, srcImage->GetDesc().Size.y >> srcImageSubresource->GetDesc().BaseMipLevel);
		subResSize.z = std::max(1u, srcImage->GetDesc().Size.z >> srcImageSubresource->GetDesc().BaseMipLevel);

		VkImageCopy vkImageCopy = {};
		vkImageCopy.srcSubresource.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(srcImage->GetDesc().Usage);
		vkImageCopy.srcSubresource.mipLevel = srcImageSubresource->GetDesc().BaseMipLevel;
		vkImageCopy.srcSubresource.baseArrayLayer = srcImageSubresource->GetDesc().BaseArrayLayer;
		vkImageCopy.srcSubresource.layerCount = srcImageSubresource->GetDesc().LayerCount;
		vkImageCopy.srcOffset.x = (ur_int32)srcRegion.Min.x;
		vkImageCopy.srcOffset.y = (ur_int32)srcRegion.Min.y;
		vkImageCopy.srcOffset.z = (ur_int32)srcRegion.Min.z;
		vkImageCopy.dstSubresource.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(dstImage->GetDesc().Usage);
		vkImageCopy.dstSubresource.mipLevel = dstImageSubresource->GetDesc().BaseMipLevel;
		vkImageCopy.dstSubresource.baseArrayLayer = dstImageSubresource->GetDesc().BaseArrayLayer;
		vkImageCopy.dstSubresource.layerCount = dstImageSubresource->GetDesc().LayerCount;
		vkImageCopy.dstOffset.x = (ur_int32)srcRegion.Min.x;
		vkImageCopy.dstOffset.y = (ur_int32)srcRegion.Min.y;
		vkImageCopy.dstOffset.z = (ur_int32)srcRegion.Min.z;
		vkImageCopy.extent.width = (ur_uint32)(srcSize.x > 0 ? srcSize.x : subResSize.x);
		vkImageCopy.extent.height = (ur_uint32)(srcSize.y > 0 ? srcSize.y : subResSize.y);
		vkImageCopy.extent.depth = (ur_uint32)(srcSize.z > 0 ? srcSize.z : subResSize.z);

		vkCmdCopyImage(this->vkCommandBuffer,
			static_cast<const GrafImageVulkan*>(srcImage)->GetVkImage(),
			GrafUtilsVulkan::GrafToVkImageLayout(srcImageSubresource->GetState()),
			static_cast<const GrafImageVulkan*>(dstImage)->GetVkImage(),
			GrafUtilsVulkan::GrafToVkImageLayout(dstImageSubresource->GetState()),
			1, &vkImageCopy);

		return Result(Success);
	}

	Result GrafCommandListVulkan::BindComputePipeline(GrafComputePipeline* grafPipeline)
	{
		if (ur_null == grafPipeline)
			return Result(InvalidArgs);

		vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, static_cast<GrafComputePipelineVulkan*>(grafPipeline)->GetVkPipeline());

		return Result(Success);
	}

	Result GrafCommandListVulkan::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline)
	{
		if (ur_null == descriptorTable || ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafComputePipelineVulkan* grafPipelineVulkan = static_cast<GrafComputePipelineVulkan*>(grafPipeline);
		GrafDescriptorTableVulkan* descriptorTableVulkan = static_cast<GrafDescriptorTableVulkan*>(descriptorTable);
		VkDescriptorSet vkDescriptorSets[] = { descriptorTableVulkan->GetVkDescriptorSet() };

		vkCmdBindDescriptorSets(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, grafPipelineVulkan->GetVkPipelineLayout(), 0, 1, vkDescriptorSets, 0, ur_null);

		return Result(Success);
	}

	Result GrafCommandListVulkan::Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ)
	{
		vkCmdDispatch(this->vkCommandBuffer, (ur_uint32)groupCountX, (ur_uint32)groupCountY, (ur_uint32)groupCountZ);

		return Result(Success);
	}

	Result GrafCommandListVulkan::BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		GrafAccelerationStructureVulkan* dstStructureVulkan = static_cast<GrafAccelerationStructureVulkan*>(dstStructrure);
		if (ur_null == dstStructureVulkan || ur_null == dstStructureVulkan->GetScratchBuffer() || ur_null == geometryData || 0 == geometryCount)
			return Result(InvalidArgs);

		// fill geometry data structures

		std::vector<VkAccelerationStructureGeometryKHR> vkGeometryInfoArray(geometryCount);
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> vkGeometryBuildRanges(geometryCount);
		for (ur_uint32 igeom = 0; igeom < geometryCount; ++igeom)
		{
			GrafAccelerationStructureGeometryData& geometry = geometryData[igeom];
			
			VkAccelerationStructureBuildRangeInfoKHR& vkGeometryBuildRange = vkGeometryBuildRanges[igeom];
			vkGeometryBuildRange = {};
			vkGeometryBuildRange.primitiveCount = (uint32_t)geometry.PrimitiveCount;
			vkGeometryBuildRange.primitiveOffset = (uint32_t)geometry.PrimitivesOffset;
			vkGeometryBuildRange.firstVertex = (uint32_t)geometry.FirstVertexIndex;
			vkGeometryBuildRange.transformOffset = (uint32_t)geometry.TransformsOffset;

			VkAccelerationStructureGeometryKHR& vkGeometryInfo = vkGeometryInfoArray[igeom];
			vkGeometryInfo = {};
			vkGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vkGeometryInfo.geometryType = GrafUtilsVulkan::GrafToVkAccelerationStructureGeometryType(geometry.GeometryType);
			vkGeometryInfo.flags = 0;
			if (GrafAccelerationStructureGeometryType::Instances == geometry.GeometryType)
			{
				VkAccelerationStructureGeometryInstancesDataKHR& vkGeometryInstances = vkGeometryInfo.geometry.instances;
				vkGeometryInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
				vkGeometryInstances.arrayOfPointers = geometry.InstancesData->IsPointersArray;
				if (geometry.InstancesData->DeviceAddress != 0)
					vkGeometryInstances.data.deviceAddress = (VkDeviceAddress)geometry.InstancesData->DeviceAddress;
				else
					vkGeometryInstances.data.hostAddress = geometry.InstancesData->HostAddress;
			}
			else if (GrafAccelerationStructureGeometryType::Triangles == geometry.GeometryType)
			{
				VkAccelerationStructureGeometryTrianglesDataKHR& vkGeometryTriangles = vkGeometryInfo.geometry.triangles;
				vkGeometryTriangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
				vkGeometryTriangles.vertexFormat = GrafUtilsVulkan::GrafToVkFormat(geometry.TrianglesData->VertexFormat);
				vkGeometryTriangles.vertexStride = (VkDeviceSize)geometry.TrianglesData->VertexStride;
				vkGeometryTriangles.maxVertex = (uint32_t)geometry.TrianglesData->VertexCount;
				vkGeometryTriangles.indexType = GrafUtilsVulkan::GrafToVkIndexType(geometry.TrianglesData->IndexType);
				if (geometry.TrianglesData->VerticesDeviceAddress != 0)
					vkGeometryTriangles.vertexData.deviceAddress = geometry.TrianglesData->VerticesDeviceAddress;
				else
					vkGeometryTriangles.vertexData.hostAddress = geometry.TrianglesData->VerticesHostAddress;
				if (geometry.TrianglesData->IndicesDeviceAddress != 0)
					vkGeometryTriangles.indexData.deviceAddress = geometry.TrianglesData->IndicesDeviceAddress;
				else
					vkGeometryTriangles.indexData.hostAddress = geometry.TrianglesData->IndicesHostAddress;
				vkGeometryTriangles.transformData = {};
				if (geometry.TrianglesData->TransformsDeviceAddress != 0)
					vkGeometryTriangles.transformData.deviceAddress = geometry.TrianglesData->TransformsDeviceAddress;
				else
					vkGeometryTriangles.transformData.hostAddress = geometry.TrianglesData->TransformsHostAddress;
			}
			else if (GrafAccelerationStructureGeometryType::AABBs == geometry.GeometryType)
			{
				VkAccelerationStructureGeometryAabbsDataKHR& vkGeometryAabbs = vkGeometryInfo.geometry.aabbs;
				vkGeometryAabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
				vkGeometryAabbs.stride = (VkDeviceSize)geometry.AabbsData->Stride;
				if (geometry.AabbsData->DeviceAddress != 0)
					vkGeometryAabbs.data.deviceAddress = geometry.AabbsData->DeviceAddress;
				else
					vkGeometryAabbs.data.hostAddress = geometry.AabbsData->HostAddress;
			}
		}

		// build

		VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo = {};
		vkBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		vkBuildInfo.pNext = ur_null;
		vkBuildInfo.type = GrafUtilsVulkan::GrafToVkAccelerationStructureType(dstStructrure->GetStructureType());
		vkBuildInfo.flags = GrafUtilsVulkan::GrafToVkAccelerationStructureBuildFlags(dstStructrure->GetStructureBuildFlags());
		vkBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		vkBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		vkBuildInfo.dstAccelerationStructure = dstStructureVulkan->GetVkAccelerationStructure();
		vkBuildInfo.geometryCount = geometryCount;
		vkBuildInfo.pGeometries = vkGeometryInfoArray.data();
		vkBuildInfo.ppGeometries = ur_null;
		vkBuildInfo.scratchData.deviceAddress = dstStructureVulkan->GetScratchBuffer()->GetDeviceAddress();

		const ur_uint32 buildInfoCount = 1;
		const VkAccelerationStructureBuildRangeInfoKHR* vkBuildRangesPerBuildInfo[buildInfoCount] = { vkGeometryBuildRanges.data() };

		vkCmdBuildAccelerationStructuresKHR(this->vkCommandBuffer, buildInfoCount, &vkBuildInfo, vkBuildRangesPerBuildInfo);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafCommandListVulkan::BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		if (ur_null == grafPipeline)
			return Result(InvalidArgs);

		vkCmdBindPipeline(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, static_cast<GrafRayTracingPipelineVulkan*>(grafPipeline)->GetVkPipeline());

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafCommandListVulkan::BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		if (ur_null == descriptorTable || ur_null == grafPipeline)
			return Result(InvalidArgs);

		GrafRayTracingPipelineVulkan* grafPipelineVulkan = static_cast<GrafRayTracingPipelineVulkan*>(grafPipeline);
		GrafDescriptorTableVulkan* descriptorTableVulkan = static_cast<GrafDescriptorTableVulkan*>(descriptorTable);
		VkDescriptorSet vkDescriptorSets[] = { descriptorTableVulkan->GetVkDescriptorSet() };

		vkCmdBindDescriptorSets(this->vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, grafPipelineVulkan->GetVkPipelineLayout(), 0, 1, vkDescriptorSets, 0, ur_null);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafCommandListVulkan::DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
		const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
		const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		static const VkStridedDeviceAddressRegionKHR NullShaderTableRegion = { 0, 0, 0 };
		auto FillVkStridedBufferRegion = [](VkStridedDeviceAddressRegionKHR& vkStridedBufferRegion, const GrafStridedBufferRegionDesc* grafStridedBufferRegion) -> void
		{
			if (grafStridedBufferRegion)
			{
				vkStridedBufferRegion.deviceAddress = (VkDeviceAddress)(grafStridedBufferRegion->BufferPtr ? grafStridedBufferRegion->BufferPtr->GetDeviceAddress() + grafStridedBufferRegion->Offset : 0);
				vkStridedBufferRegion.stride = (VkDeviceSize)grafStridedBufferRegion->Stride;
				vkStridedBufferRegion.size = (VkDeviceSize)grafStridedBufferRegion->Size;
			}
			else
			{
				vkStridedBufferRegion = NullShaderTableRegion;
			}
		};

		VkStridedDeviceAddressRegionKHR rayGenShaderTableRegion;
		VkStridedDeviceAddressRegionKHR rayMissShaderTableRegion;
		VkStridedDeviceAddressRegionKHR rayHitShaderTableRegion;
		VkStridedDeviceAddressRegionKHR rayCallShaderTableRegion;
		FillVkStridedBufferRegion(rayGenShaderTableRegion, rayGenShaderTable);
		FillVkStridedBufferRegion(rayMissShaderTableRegion, missShaderTable);
		FillVkStridedBufferRegion(rayHitShaderTableRegion, hitShaderTable);
		FillVkStridedBufferRegion(rayCallShaderTableRegion, callableShaderTable);

		vkCmdTraceRaysKHR(this->vkCommandBuffer,
			&rayGenShaderTableRegion,
			&rayMissShaderTableRegion,
			&rayHitShaderTableRegion,
			&rayCallShaderTableRegion,
			(ur_uint32)width, (ur_uint32)height, (ur_uint32)depth);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafFenceVulkan::GrafFenceVulkan(GrafSystem &grafSystem) :
		GrafFence(grafSystem)
	{
		this->vkEvent = VK_NULL_HANDLE;
	}

	GrafFenceVulkan::~GrafFenceVulkan()
	{
		this->Deinitialize();
	}

	Result GrafFenceVulkan::Deinitialize()
	{
		if (this->vkEvent != VK_NULL_HANDLE)
		{
			vkDestroyEvent(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent, ur_null);
			this->vkEvent = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafFenceVulkan::Initialize(GrafDevice *grafDevice)
	{
		this->Deinitialize();

		GrafFence::Initialize(grafDevice);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafFenceVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// create synchronization object

		VkEventCreateInfo vkEventInfo = {};
		vkEventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		vkEventInfo.flags = 0;

		VkResult vkRes = vkCreateEvent(vkDevice, &vkEventInfo, ur_null, &this->vkEvent);
		if (vkRes != VK_SUCCESS)
		{
			return ResultError(Failure, std::string("GrafFenceVulkan: vkCreateEvent failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	Result GrafFenceVulkan::SetState(GrafFenceState state)
	{
		VkResult vkRes = VK_SUCCESS;

		switch (state)
		{
		case GrafFenceState::Reset:
			vkRes = vkResetEvent(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent);
			break;
		case GrafFenceState::Signaled:
			vkRes = vkSetEvent(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent);
			break;
		default:
			return ResultError(InvalidArgs, "GrafFenceVulkan: SetState failed, invalid GrafFenceState");
		};

		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafFenceVulkan: vkResetEvent failed with VkResult = ") + VkResultToString(vkRes));

		return Result(Success);
	}

	Result GrafFenceVulkan::WaitSignaled()
	{
		Result res(Success);

		GrafFenceState state;
		do
		{
			res = this->GetState(state);
		} while (state != GrafFenceState::Signaled && !Failed(res));

		return res;
	}

	Result GrafFenceVulkan::GetState(GrafFenceState& state)
	{
		VkResult vkRes = vkGetEventStatus(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent);
		if (VK_EVENT_SET == vkRes)
			state = GrafFenceState::Signaled;
		else if (VK_EVENT_RESET == vkRes)
			state = GrafFenceState::Reset;
		else
			return ResultError(Failure, std::string("GrafFenceVulkan: vkGetEventStatus failed with VkResult = ") + VkResultToString(vkRes));
		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafCanvasVulkan::GrafCanvasVulkan(GrafSystem &grafSystem) :
		GrafCanvas(grafSystem)
	{
		this->vkSurface = VK_NULL_HANDLE;
		this->vkSwapChain = VK_NULL_HANDLE;
		this->vkDevicePresentQueueId = 0;
		this->frameCount = 0;
		this->frameIdx = 0;
	}

	GrafCanvasVulkan::~GrafCanvasVulkan()
	{
		this->Deinitialize();
	}

	Result GrafCanvasVulkan::Deinitialize()
	{
		this->frameCount = 0;
		this->frameIdx = 0;
		this->swapChainImageCount = 0;
		this->swapChainCurrentImageId = 0;

		this->imageTransitionCmdListBegin.clear();
		this->imageTransitionCmdListEnd.clear();

		for (auto& vkSemaphore : this->vkSemaphoreImageAcquired)
		{
			if (vkSemaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), vkSemaphore, ur_null);
			}
		}
		this->vkSemaphoreImageAcquired.clear();

		for (auto& vkSemaphore : this->vkSemaphoreRenderFinished)
		{
			if (vkSemaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), vkSemaphore, ur_null);
			}
		}
		this->vkSemaphoreRenderFinished.clear();

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
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

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
		this->vkDevicePresentQueueId = vkDeviceGraphicsQueueId;
		const ur_uint32 vkDevicePresentQueueCount = 1;
		ur_uint32 vkDevicePresentQueueIds[vkDevicePresentQueueCount] = { this->vkDevicePresentQueueId };

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
				(VK_PRESENT_MODE_IMMEDIATE_KHR == vkPresentModes[i] || VK_PRESENT_MODE_MAILBOX_KHR == vkPresentModes[i]))
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
		vkSwapChainInfo.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		vkSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // the same graphics queue is used (vkDeviceGraphicsQueueId)
		vkSwapChainInfo.queueFamilyIndexCount = vkDevicePresentQueueCount;
		vkSwapChainInfo.pQueueFamilyIndices = vkDevicePresentQueueIds;
		vkSwapChainInfo.preTransform = vkSurfaceCaps.currentTransform;
		vkSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		vkSwapChainInfo.presentMode = vkPresentMode;
		vkSwapChainInfo.clipped = VK_TRUE;
		vkSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

		res = vkCreateSwapchainKHR(vkDevice, &vkSwapChainInfo, ur_null, &this->vkSwapChain);
		if (res != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSwapchainKHR failed with VkResult = ") + VkResultToString(res));
		}
		LogNoteGrafDbg("GrafCanvasVulkan: VkSwapchainKHR created");

		// retrieve swap chain images

		ur_uint32 vkSwapChainImageRealCount = 0;
		vkGetSwapchainImagesKHR(vkDevice, this->vkSwapChain, &vkSwapChainImageRealCount, ur_null);
		std::vector<VkImage> vkSwapChainImages(vkSwapChainImageRealCount);
		vkGetSwapchainImagesKHR(vkDevice, this->vkSwapChain, &vkSwapChainImageRealCount, vkSwapChainImages.data());
		if (0 == vkSwapChainImageRealCount)
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafCanvasVulkan: vkGetSwapchainImagesKHR failed with zero images returned");
		}
		this->swapChainImageCount = vkSwapChainImageRealCount;
		this->swapChainCurrentImageId = 0;

		this->swapChainImages.reserve(vkSwapChainImages.size());
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
			imageParams.ImageDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;

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

		// per frame objects

		this->frameCount = vkSwapChainImageRealCount; // command list(s) and sync object(s) per image
		this->frameIdx = ur_uint32(this->frameCount - 1); // first AcquireNextImage will make it 0

		// create presentation sync objects 

		VkSemaphoreCreateInfo vkSemaphoreInfo = {};
		vkSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkSemaphoreInfo.flags = 0;

		this->vkSemaphoreImageAcquired.reserve(this->frameCount);
		for (ur_uint32 iframe = 0; iframe < this->frameCount; ++iframe)
		{
			VkSemaphore vkSemaphore;
			res = vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, ur_null, &vkSemaphore);
			if (res != VK_SUCCESS)
			{
				this->Deinitialize();
				return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSemaphore failed with VkResult = ") + VkResultToString(res));
			}
			this->vkSemaphoreImageAcquired.push_back(vkSemaphore);
		}

		this->vkSemaphoreRenderFinished.reserve(this->frameCount);
		for (ur_uint32 iframe = 0; iframe < this->frameCount; ++iframe)
		{
			VkSemaphore vkSemaphore;
			res = vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, ur_null, &vkSemaphore);
			if (res != VK_SUCCESS)
			{
				this->Deinitialize();
				return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSemaphore failed with VkResult = ") + VkResultToString(res));
			}
			this->vkSemaphoreRenderFinished.push_back(vkSemaphore);
		}

		// create comamnd list for image state/layout transitions

		this->imageTransitionCmdListBegin.resize(this->frameCount);
		this->imageTransitionCmdListEnd.resize(this->frameCount);
		for (ur_uint32 iframe = 0; iframe < this->frameCount; ++iframe)
		{
			Result urRes = Success;
			urRes &= this->GetGrafSystem().CreateCommandList(this->imageTransitionCmdListBegin[iframe]);
			urRes &= this->GetGrafSystem().CreateCommandList(this->imageTransitionCmdListEnd[iframe]);
			if (Succeeded(urRes))
			{
				urRes &= this->imageTransitionCmdListBegin[iframe]->Initialize(grafDevice);
				urRes &= this->imageTransitionCmdListEnd[iframe]->Initialize(grafDevice);
			}
			if (Failed(urRes))
			{
				this->Deinitialize();
				return ResultError(Failure, "GrafCanvasVulkan: failed to create transition command list");
			}
		}

		// acquire an image to use as a current RT

		Result urRes = this->AcquireNextImage();
		if (Failed(urRes))
		{
			this->Deinitialize();
			return urRes;
		}

		// initial image layout transition into common state

		#if (UR_GRAF_VULKAN_SWAPCHAIN_NEXT_IMAGE_IMPLICIT_TRANSITION_TO_GENERAL)
		GrafImage* swapChainNextImage = this->swapChainImages[this->swapChainCurrentImageId].get();
		GrafCommandList* imageTransitionCmdListBeginCrnt = this->imageTransitionCmdListBegin[this->frameIdx].get();
		imageTransitionCmdListBeginCrnt->Begin();
		imageTransitionCmdListBeginCrnt->ImageMemoryBarrier(swapChainNextImage, GrafImageState::Current, GrafImageState::Common);
		imageTransitionCmdListBeginCrnt->End();
		{
			// submit with semaphore (wait image's acquired before using it as render target)
			GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(imageTransitionCmdListBeginCrnt);
			VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
			VkQueue vkSubmissionQueue;
			vkGetDeviceQueue(vkDevice, grafDeviceVulkan->GetVkDeviceGraphicsQueueId(), 0, &vkSubmissionQueue);
			VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			VkSubmitInfo vkSubmitInfo = {};
			vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			vkSubmitInfo.waitSemaphoreCount = 1;
			vkSubmitInfo.pWaitSemaphores = &this->vkSemaphoreImageAcquired[this->frameIdx];
			vkSubmitInfo.pWaitDstStageMask = &waitStageFlags;
			vkSubmitInfo.commandBufferCount = 1;
			vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
			vkSubmitInfo.signalSemaphoreCount = 0;
			vkSubmitInfo.pSignalSemaphores = ur_null;
			VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
			if (vkRes != VK_SUCCESS)
				return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));
		}
		#endif

		return Result(Success);
	}

	Result GrafCanvasVulkan::Present()
	{
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkQueue vkDevicePresentQueue;
		vkGetDeviceQueue(vkDevice, this->vkDevicePresentQueueId, 0, &vkDevicePresentQueue);

		// do image layout transition to presentation state

		GrafImage* swapChainCurrentImage = this->swapChainImages[this->swapChainCurrentImageId].get();
		GrafCommandList* imageTransitionCmdListEndCrnt = this->imageTransitionCmdListEnd[this->frameIdx].get();
		imageTransitionCmdListEndCrnt->Begin();
		imageTransitionCmdListEndCrnt->ImageMemoryBarrier(swapChainCurrentImage, GrafImageState::Current, GrafImageState::Present);
		imageTransitionCmdListEndCrnt->End();
		{
			// submit with semaphore (signal rendering's finished before presenting )
			GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(imageTransitionCmdListEndCrnt);
			VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
			VkQueue vkSubmissionQueue;
			vkGetDeviceQueue(vkDevice, grafDeviceVulkan->GetVkDeviceGraphicsQueueId(), 0, &vkSubmissionQueue);
			VkSubmitInfo vkSubmitInfo = {};
			vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			vkSubmitInfo.waitSemaphoreCount = 0;
			vkSubmitInfo.pWaitSemaphores = ur_null;
			vkSubmitInfo.pWaitDstStageMask = ur_null;
			vkSubmitInfo.commandBufferCount = 1;
			vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
			vkSubmitInfo.signalSemaphoreCount = 1;
			vkSubmitInfo.pSignalSemaphores = &this->vkSemaphoreRenderFinished[this->frameIdx];
			VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
			if (vkRes != VK_SUCCESS)
				return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));
		}

		// present current image

		VkSwapchainKHR swapChains[] = { this->vkSwapChain };

		VkPresentInfoKHR vkPresentInfo = {};
		vkPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		vkPresentInfo.waitSemaphoreCount = 1;
		vkPresentInfo.pWaitSemaphores = &this->vkSemaphoreRenderFinished[this->frameIdx]; // make sure rendering's finished before presenting
		vkPresentInfo.swapchainCount = ur_array_size(swapChains);
		vkPresentInfo.pSwapchains = swapChains;
		vkPresentInfo.pImageIndices = &this->swapChainCurrentImageId;
		vkPresentInfo.pResults = ur_null;

		VkResult vkRes = vkQueuePresentKHR(vkDevicePresentQueue, &vkPresentInfo);
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafCanvasVulkan: vkQueuePresentKHR failed with VkResult = ") + VkResultToString(vkRes));

		// acquire next available image to use as RT

		Result res = this->AcquireNextImage();
		if (Failed(res))
			return res;

		// do image layout transition to common state

		#if (UR_GRAF_VULKAN_SWAPCHAIN_NEXT_IMAGE_IMPLICIT_TRANSITION_TO_GENERAL)
		GrafImage* swapChainNextImage = this->swapChainImages[this->swapChainCurrentImageId].get();
		GrafCommandList* imageTransitionCmdListBeginNext = this->imageTransitionCmdListBegin[this->frameIdx].get();
		imageTransitionCmdListBeginNext->Begin();
		imageTransitionCmdListBeginNext->ImageMemoryBarrier(swapChainNextImage, GrafImageState::Current, GrafImageState::Common);
		imageTransitionCmdListBeginNext->End();
		{
			// submit with semaphore (wait image's acquired before using it as render target)
			GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(imageTransitionCmdListBeginNext);
			VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
			VkQueue vkSubmissionQueue;
			vkGetDeviceQueue(vkDevice, grafDeviceVulkan->GetVkDeviceGraphicsQueueId(), 0, &vkSubmissionQueue);
			VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			VkSubmitInfo vkSubmitInfo = {};
			vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			vkSubmitInfo.waitSemaphoreCount = 1;
			vkSubmitInfo.pWaitSemaphores = &this->vkSemaphoreImageAcquired[this->frameIdx];
			vkSubmitInfo.pWaitDstStageMask = &waitStageFlags;
			vkSubmitInfo.commandBufferCount = 1;
			vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
			vkSubmitInfo.signalSemaphoreCount = 0;
			vkSubmitInfo.pSignalSemaphores = ur_null;
			VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
			if (vkRes != VK_SUCCESS)
				return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));
		}
		#endif

		return Result(Success);
	}

	Result GrafCanvasVulkan::AcquireNextImage()
	{
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		this->frameIdx = (this->frameIdx + 1) % this->frameCount;

		VkResult res = vkAcquireNextImageKHR(vkDevice, this->vkSwapChain, ~ur_uint64(0), this->vkSemaphoreImageAcquired[this->frameIdx], VK_NULL_HANDLE, &this->swapChainCurrentImageId);
		if (res != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafCanvasVulkan: vkAcquireNextImageKHR failed with VkResult = ") + VkResultToString(res));

		return Result(Success);
	}

	GrafImage* GrafCanvasVulkan::GetCurrentImage()
	{
		return this->GetSwapChainImage((ur_uint)this->swapChainCurrentImageId);
	}

	GrafImage* GrafCanvasVulkan::GetSwapChainImage(ur_uint imageId)
	{
		if ((ur_size)imageId > this->swapChainImages.size())
			return ur_null;
		return this->swapChainImages[imageId].get();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafImageVulkan::GrafImageVulkan(GrafSystem &grafSystem) :
		GrafImage(grafSystem)
	{
		this->imageExternalHandle = false;
		this->vkImage = VK_NULL_HANDLE;
		this->vkImageView = VK_NULL_HANDLE;
		this->vkDeviceMemory = VK_NULL_HANDLE;
		this->vkDeviceMemoryOffset = 0;
		this->vkDeviceMemorySize = 0;
		this->vkDeviceMemoryAlignment = 0;
		this->vmaAllocation = VK_NULL_HANDLE;
	}

	GrafImageVulkan::~GrafImageVulkan()
	{
		this->Deinitialize();
	}

	Result GrafImageVulkan::Deinitialize()
	{
		this->grafDefaultSubresource.reset(ur_null);
		this->vkImageView = VK_NULL_HANDLE;

		if (this->vkImage != VK_NULL_HANDLE && !this->imageExternalHandle)
		{
			vkDestroyImage(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkImage, ur_null);
		}
		this->vkImage = VK_NULL_HANDLE;
		this->imageExternalHandle = false;

		this->vkDeviceMemoryOffset = 0;
		this->vkDeviceMemorySize = 0;
		this->vkDeviceMemoryAlignment = 0;
		if (this->vmaAllocation != VK_NULL_HANDLE)
		{
			vmaFreeMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVmaAllocator(), this->vmaAllocation);
			this->vmaAllocation = VK_NULL_HANDLE;
			this->vkDeviceMemory = VK_NULL_HANDLE;  // handle to VMA allocation info
		}
		if (this->vkDeviceMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDeviceMemory, ur_null);
			this->vkDeviceMemory = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafImageVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafImage::Initialize(grafDevice, initParams);

		// validate logical device

		GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(grafDeviceVulkan->GetDeviceId());
		ur_uint32 vkDeviceGraphicsQueueId = (ur_uint32)grafDeviceVulkan->GetVkDeviceGraphicsQueueId();

		// create image

		ur_bool isStagingImage = (initParams.ImageDesc.MemoryType & (ur_uint)GrafDeviceMemoryFlag::CpuVisible);
		VkImageCreateInfo vkImageInfo = {};
		vkImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		vkImageInfo.flags = 0;
		vkImageInfo.imageType = GrafUtilsVulkan::GrafToVkImageType(initParams.ImageDesc.Type);
		vkImageInfo.format = GrafUtilsVulkan::GrafToVkFormat(initParams.ImageDesc.Format);
		vkImageInfo.extent.width = (ur_uint32)initParams.ImageDesc.Size.x;
		vkImageInfo.extent.height = (ur_uint32)initParams.ImageDesc.Size.y;
		vkImageInfo.extent.depth = (ur_uint32)initParams.ImageDesc.Size.z;
		vkImageInfo.mipLevels = (ur_uint32)initParams.ImageDesc.MipLevels;
		vkImageInfo.arrayLayers = 1;
		vkImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		vkImageInfo.tiling = (isStagingImage ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL);
		vkImageInfo.usage = GrafUtilsVulkan::GrafToVkImageUsage(initParams.ImageDesc.Usage);
		vkImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkImageInfo.queueFamilyIndexCount = 1;
		vkImageInfo.pQueueFamilyIndices = &vkDeviceGraphicsQueueId;
		vkImageInfo.initialLayout = (isStagingImage ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED);

		VkResult vkRes = vkCreateImage(vkDevice, &vkImageInfo, ur_null, &this->vkImage);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafImageVulkan: vkCreateImage failed with VkResult = ") + VkResultToString(vkRes));
		}

		VkMemoryPropertyFlags vkMemoryPropertiesExpected = GrafUtilsVulkan::GrafToVkMemoryProperties(initParams.ImageDesc.MemoryType);

		VkMemoryRequirements vkMemoryRequirements = {};
		vkGetImageMemoryRequirements(vkDevice, this->vkImage, &vkMemoryRequirements);

		VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);

		#if (UR_GRAF_VULKAN_VMA_ENABLED)

		VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
		for (ur_uint32 itype = 0; itype < vkDeviceMemoryProperties.memoryTypeCount; ++itype)
		{
			if ((vkDeviceMemoryProperties.memoryTypes[itype].propertyFlags & vkMemoryPropertiesExpected) &&
				(vkMemoryRequirements.memoryTypeBits & (1 << itype)))
			{
				vmaAllocationCreateInfo.memoryTypeBits |= (1 << itype);
				break;
			}
		}
		if (ur_uint32(0) == vmaAllocationCreateInfo.memoryTypeBits)
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafImageVulkan: failed to find required memory type");
		}

		VmaAllocationInfo vmaAllocationInfo = {};
		vkRes = vmaAllocateMemoryForImage(grafDeviceVulkan->GetVmaAllocator(), this->vkImage, &vmaAllocationCreateInfo, &this->vmaAllocation, &vmaAllocationInfo);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafImageVulkan: vmaAllocateMemoryForBuffer failed with VkResult = ") + VkResultToString(vkRes));
		}

		this->vkDeviceMemory = vmaAllocationInfo.deviceMemory;
		this->vkDeviceMemoryOffset = vmaAllocationInfo.offset;
		this->vkDeviceMemorySize = vmaAllocationInfo.size;
		this->vkDeviceMemoryAlignment = vkMemoryRequirements.alignment;

		#else

		VkMemoryAllocateInfo vkAllocateInfo = {};
		vkAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkAllocateInfo.allocationSize = vkMemoryRequirements.size;
		vkAllocateInfo.memoryTypeIndex = ur_uint32(-1);
		for (ur_uint32 itype = 0; itype < vkDeviceMemoryProperties.memoryTypeCount; ++itype)
		{
			if ((vkDeviceMemoryProperties.memoryTypes[itype].propertyFlags & vkMemoryPropertiesExpected) &&
				(vkMemoryRequirements.memoryTypeBits & (1 << itype)))
			{
				vkAllocateInfo.memoryTypeIndex = itype;
				break;
			}
		}
		if (ur_uint32(-1) == vkAllocateInfo.memoryTypeIndex)
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafImageVulkan: failed to find required memory type");
		}

		// allocate memory

		vkRes = vkAllocateMemory(vkDevice, &vkAllocateInfo, ur_null, &this->vkDeviceMemory);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafImageVulkan: vkAllocateMemory failed with VkResult = ") + VkResultToString(vkRes));
		}

		this->vkDeviceMemoryOffset = 0;
		this->vkDeviceMemorySize = vkMemoryRequirements.size;
		this->vkDeviceMemoryAlignment = vkMemoryRequirements.alignment;

		#endif

		// bind device memory allocation to image

		vkRes = vkBindImageMemory(vkDevice, this->vkImage, this->vkDeviceMemory, this->vkDeviceMemoryOffset);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafImageVulkan: vkBindImageMemory failed with VkResult = ") + VkResultToString(vkRes));
		}

		// create default subresource

		Result res = this->CreateDefaultSubresource();
		if (Failed(res))
		{
			this->Deinitialize();
			return res;
		}

		return Result(Success);
	}

	Result GrafImageVulkan::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->vkDeviceMemorySize; // entire allocation range

		GrafWriteCallback copyWriteCallback = [&dataPtr, &dataSize, &srcOffset](ur_byte *mappedDataPtr) -> Result
		{
			memcpy(mappedDataPtr, dataPtr + srcOffset, dataSize);
			return Result(Success);
		};

		return this->Write(copyWriteCallback, dataSize, srcOffset, dstOffset);
	}

	Result GrafImageVulkan::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->vkDeviceMemorySize; // entire allocation range

		if (ur_null == writeCallback || dstOffset + dataSize > this->vkDeviceMemorySize)
			return Result(InvalidArgs);

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		void* mappedMemoryPtr = ur_null;
		VkResult vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset + dstOffset, dataSize, 0, &mappedMemoryPtr);
		if (vkRes != VK_SUCCESS)
		{
			return ResultError(Failure, std::string("GrafImageVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
		}

		writeCallback((ur_byte*)mappedMemoryPtr);

		vkUnmapMemory(vkDevice, this->vkDeviceMemory);

		return Result(Success);
	}

	Result GrafImageVulkan::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->vkDeviceMemorySize; // entire allocation range

		if (ur_null == dataPtr || srcOffset + dataSize > this->vkDeviceMemorySize)
			return Result(InvalidArgs);

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		void* mappedMemoryPtr = ur_null;
		VkResult vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset + srcOffset, dataSize, 0, &mappedMemoryPtr);
		if (vkRes != VK_SUCCESS)
		{
			return ResultError(Failure, std::string("GrafImageVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
		}

		memcpy(dataPtr + dstOffset, mappedMemoryPtr, dataSize);

		vkUnmapMemory(vkDevice, this->vkDeviceMemory);

		return Result(Success);
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
			Result res = this->CreateDefaultSubresource();
			if (Failed(res))
				return res;
		}

		return Result(Success);
	}

	Result GrafImageVulkan::CreateDefaultSubresource()
	{
		if (VK_NULL_HANDLE == this->vkImage || this->vkImageView != VK_NULL_HANDLE)
		{
			return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to create default subresource & view(s), invalid parameters"));
		}

		GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice()); // at this point device expected to be validate
		
		Result res = grafSystemVulkan.CreateImageSubresource(this->grafDefaultSubresource);
		if (Failed(res))
		{
			return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to create image subresource"));
		}

		GrafImageSubresource::InitParams grafSubresParams = {};
		grafSubresParams.Image = this;
		grafSubresParams.SubresourceDesc.BaseMipLevel = 0;
		grafSubresParams.SubresourceDesc.LevelCount = this->GetDesc().MipLevels;
		grafSubresParams.SubresourceDesc.BaseArrayLayer = 0;
		grafSubresParams.SubresourceDesc.LayerCount = 1;
		
		res = this->grafDefaultSubresource->Initialize(grafDeviceVulkan, grafSubresParams);
		if (Failed(res))
		{
			this->grafDefaultSubresource.reset(ur_null);
			return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize image subresource"));
		}

		GrafImageSubresourceVulkan* grafSubresourceVulkan = static_cast<GrafImageSubresourceVulkan*>(this->grafDefaultSubresource.get());
		this->vkImageView = grafSubresourceVulkan->GetVkImageView(); // shortcut

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafImageSubresourceVulkan::GrafImageSubresourceVulkan(GrafSystem &grafSystem) :
		GrafImageSubresource(grafSystem)
	{
		this->vkImageView = VK_NULL_HANDLE;
	}

	GrafImageSubresourceVulkan::~GrafImageSubresourceVulkan()
	{
		this->Deinitialize();
	}

	Result GrafImageSubresourceVulkan::Deinitialize()
	{
		if (this->vkImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkImageView, ur_null);
			this->vkImageView = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafImageSubresourceVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafImageSubresource::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageSubresourceVulkan: failed to initialize, invalid GrafDevice"));
		}

		// create view
		
		Result res = this->CreateVkImageView();
		if (Failed(res))
			return res;

		return Result(Success);
	}

	Result GrafImageSubresourceVulkan::CreateVkImageView()
	{
		// validate image

		const GrafImageVulkan* grafImageVulkan = static_cast<const GrafImageVulkan*>(this->GetImage());
		if (ur_null == grafImageVulkan || VK_NULL_HANDLE == grafImageVulkan->GetVkImage())
		{
			return ResultError(InvalidArgs, std::string("GrafImageSubresourceVulkan: failed to initialize, invalid image"));
		}

		const GrafImageDesc& grafImageDesc = grafImageVulkan->GetDesc();
		const GrafImageSubresourceDesc& grafSubresDesc = this->GetDesc();

		VkImageType vkImageType = GrafUtilsVulkan::GrafToVkImageType(grafImageDesc.Type);
		VkImageViewType vkViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		switch (vkImageType)
		{
		case VK_IMAGE_TYPE_1D: vkViewType = VK_IMAGE_VIEW_TYPE_1D; break;
		case VK_IMAGE_TYPE_2D: vkViewType = VK_IMAGE_VIEW_TYPE_2D; break;
		case VK_IMAGE_TYPE_3D: vkViewType = VK_IMAGE_VIEW_TYPE_3D; break;
			// NOTE: handle texture arrays
			//VK_IMAGE_VIEW_TYPE_CUBE;
			//VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			//VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			//VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		default:
			return ResultError(InvalidArgs, "GrafImageSubresourceVulkan: failed to create image views, unsupported image type");
		};

		VkImageViewCreateInfo vkViewInfo = {};
		vkViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vkViewInfo.flags = 0;
		vkViewInfo.image = grafImageVulkan->GetVkImage();
		vkViewInfo.viewType = vkViewType;
		vkViewInfo.format = GrafUtilsVulkan::GrafToVkFormat(grafImageDesc.Format);
		vkViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		vkViewInfo.subresourceRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(grafImageDesc.Usage);
		vkViewInfo.subresourceRange.baseMipLevel = grafSubresDesc.BaseMipLevel;
		vkViewInfo.subresourceRange.levelCount = std::max(grafSubresDesc.LevelCount, 1u);
		vkViewInfo.subresourceRange.baseArrayLayer = grafSubresDesc.BaseArrayLayer;
		vkViewInfo.subresourceRange.layerCount = std::max(grafSubresDesc.LayerCount, 1u);

		if ((grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::DepthStencilRenderTarget)) &&
			(grafImageDesc.Usage & ur_uint(GrafImageUsageFlag::ShaderRead)))
		{
			// create depth only view if used as a shader input
			// TODO: investigate whether separate views required for VkFramebufferCreateInfo and shader input
			vkViewInfo.subresourceRange.aspectMask = (VK_IMAGE_ASPECT_DEPTH_BIT);
		}

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice()); // at this point device expected to be validate
		VkResult res = vkCreateImageView(grafDeviceVulkan->GetVkDevice(), &vkViewInfo, ur_null, &this->vkImageView);
		if (res != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafImageSubresourceVulkan: vkCreateImageView failed with VkResult = ") + VkResultToString(res));

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafBufferVulkan::GrafBufferVulkan(GrafSystem &grafSystem) :
		GrafBuffer(grafSystem)
	{
		this->vkBuffer = VK_NULL_HANDLE;
		this->vkDeviceMemory = VK_NULL_HANDLE;
		this->vkDeviceMemoryOffset = 0;
		this->vkDeviceMemorySize = 0;
		this->vkDeviceMemoryAlignment = 0;
		this->vmaAllocation = VK_NULL_HANDLE;
		this->mappedMemoryDataPtr = ur_null;
	}

	GrafBufferVulkan::~GrafBufferVulkan()
	{
		this->Deinitialize();
	}

	Result GrafBufferVulkan::Deinitialize()
	{
		this->vkDeviceMemoryOffset = 0;
		this->vkDeviceMemorySize = 0;
		this->vkDeviceMemoryAlignment = 0;
		if (this->vmaAllocation != VK_NULL_HANDLE)
		{
			vmaFreeMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVmaAllocator(), this->vmaAllocation);
			this->vmaAllocation = VK_NULL_HANDLE;
			this->vkDeviceMemory = VK_NULL_HANDLE; // handle to VMA allocation info
			this->mappedMemoryDataPtr = ur_null; // managed by VMA if was created mapped
		}
		if (this->vkDeviceMemory != VK_NULL_HANDLE)
		{
			if (this->mappedMemoryDataPtr)
			{
				// unmap permanently mapped memory
				vkUnmapMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDeviceMemory);
				this->mappedMemoryDataPtr = ur_null;
			}
			vkFreeMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDeviceMemory, ur_null);
			this->vkDeviceMemory = VK_NULL_HANDLE;
		}
		if (this->vkBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkBuffer, ur_null);
			this->vkBuffer = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafBufferVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafBuffer::Initialize(grafDevice, initParams);

		// validate logical device

		GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafBufferVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(grafDeviceVulkan->GetDeviceId());
		ur_uint32 vkDeviceGraphicsQueueId = (ur_uint32)grafDeviceVulkan->GetVkDeviceGraphicsQueueId();

		// create buffer

		VkBufferCreateInfo vkBufferInfo = {};
		vkBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vkBufferInfo.flags = 0;
		vkBufferInfo.size = (VkDeviceSize)initParams.BufferDesc.SizeInBytes;
		vkBufferInfo.usage = GrafUtilsVulkan::GrafToVkBufferUsage(initParams.BufferDesc.Usage);
		vkBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkBufferInfo.queueFamilyIndexCount = 1;
		vkBufferInfo.pQueueFamilyIndices = &vkDeviceGraphicsQueueId;

		VkResult vkRes = vkCreateBuffer(vkDevice, &vkBufferInfo, ur_null, &this->vkBuffer);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafBufferVulkan: vkCreateBuffer failed with VkResult = ") + VkResultToString(vkRes));
		}

		// request buffer memory requirements and find corresponding physical device memory type for allocation

		VkMemoryPropertyFlags vkMemoryPropertiesExpected = GrafUtilsVulkan::GrafToVkMemoryProperties(initParams.BufferDesc.MemoryType);
		
		VkMemoryRequirements vkMemoryRequirements = {};
		vkGetBufferMemoryRequirements(vkDevice, this->vkBuffer, &vkMemoryRequirements);

		VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);

		ur_bool alwaysMapped = (ur_uint(GrafDeviceMemoryFlag::CpuVisible) == initParams.BufferDesc.MemoryType); // keep mapped cpu read/write memory

		// allocate memory
		#if (UR_GRAF_VULKAN_VMA_ENABLED)

		VmaAllocationCreateInfo vmaAllocationCreateInfo = {};
		for (ur_uint32 itype = 0; itype < vkDeviceMemoryProperties.memoryTypeCount; ++itype)
		{
			if ((vkDeviceMemoryProperties.memoryTypes[itype].propertyFlags & vkMemoryPropertiesExpected) &&
				(vkMemoryRequirements.memoryTypeBits & (1 << itype)))
			{
				vmaAllocationCreateInfo.memoryTypeBits |= (1 << itype);
				break;
			}
		}
		if (ur_uint32(0) == vmaAllocationCreateInfo.memoryTypeBits)
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafBufferVulkan: failed to find required memory type");
		}

		if (alwaysMapped)
		{
			vmaAllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		VmaAllocationInfo vmaAllocationInfo = {};
		vkRes = vmaAllocateMemoryForBuffer(grafDeviceVulkan->GetVmaAllocator(), this->vkBuffer, &vmaAllocationCreateInfo, &this->vmaAllocation, &vmaAllocationInfo);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafBufferVulkan: vmaAllocateMemoryForBuffer failed with VkResult = ") + VkResultToString(vkRes));
		}

		this->vkDeviceMemory = vmaAllocationInfo.deviceMemory;
		this->vkDeviceMemoryOffset = vmaAllocationInfo.offset;
		this->vkDeviceMemorySize = vmaAllocationInfo.size;
		this->vkDeviceMemoryAlignment = vkMemoryRequirements.alignment;
		this->mappedMemoryDataPtr = vmaAllocationInfo.pMappedData;

		#else

		VkMemoryAllocateInfo vkAllocateInfo = {};
		vkAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vkAllocateInfo.allocationSize = vkMemoryRequirements.size;
		vkAllocateInfo.memoryTypeIndex = ur_uint32(-1);
		for (ur_uint32 itype = 0; itype < vkDeviceMemoryProperties.memoryTypeCount; ++itype)
		{
			if ((vkDeviceMemoryProperties.memoryTypes[itype].propertyFlags & vkMemoryPropertiesExpected) &&
				(vkMemoryRequirements.memoryTypeBits & (1 << itype)))
			{
				vkAllocateInfo.memoryTypeIndex = itype;
				break;
			}
		}
		if (ur_uint32(-1) == vkAllocateInfo.memoryTypeIndex)
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafBufferVulkan: failed to find required memory type");
		}

		// allocate memory

		vkRes = vkAllocateMemory(vkDevice, &vkAllocateInfo, ur_null, &this->vkDeviceMemory);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafBufferVulkan: vkAllocateMemory failed with VkResult = ") + VkResultToString(vkRes));
		}

		this->vkDeviceMemoryOffset = 0;
		this->vkDeviceMemorySize = vkMemoryRequirements.size;
		this->vkDeviceMemoryAlignment = vkMemoryRequirements.alignment;
		this->mappedMemoryDataPtr = ur_null;

		if (alwaysMapped)
		{
			vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset, this->vkDeviceMemorySize, 0, &this->mappedMemoryDataPtr);
			if (vkRes != VK_SUCCESS)
			{
				this->Deinitialize();
				return ResultError(Failure, std::string("GrafBufferVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
			}
		}

		#endif

		// bind device memory allocation to buffer

		vkRes = vkBindBufferMemory(vkDevice, this->vkBuffer, this->vkDeviceMemory, this->vkDeviceMemoryOffset);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafBufferVulkan: vkBindBufferMemory failed with VkResult = ") + VkResultToString(vkRes));
		}

		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		// retrieve device address
	
		if (ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress) & initParams.BufferDesc.Usage)
		{
			VkBufferDeviceAddressInfo vkBufferAddressInfo = {};
			vkBufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			vkBufferAddressInfo.pNext = ur_null;
			vkBufferAddressInfo.buffer = this->vkBuffer;

			this->bufferDeviceAddress = vkGetBufferDeviceAddress(vkDevice, &vkBufferAddressInfo);
		}
		#endif
		
		return Result(Success);
	}

	Result GrafBufferVulkan::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->GetDesc().SizeInBytes; // entire allocation range

		GrafWriteCallback copyWriteCallback = [&dataPtr, &dataSize, &srcOffset](ur_byte *mappedDataPtr) -> Result
		{
			memcpy(mappedDataPtr, dataPtr + srcOffset, dataSize);
			return Result(Success);
		};

		return this->Write(copyWriteCallback, dataSize, srcOffset, dstOffset);
	}

	Result GrafBufferVulkan::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->GetDesc().SizeInBytes; // entire allocation range

		if (ur_null == writeCallback || dstOffset + dataSize > this->vkDeviceMemorySize)
			return Result(InvalidArgs);

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		void* dstMappedPtr = ur_null;
		ur_bool localyMapped = (ur_null == this->mappedMemoryDataPtr);
		if (localyMapped)
		{
			VkResult vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset + dstOffset, dataSize, 0, &dstMappedPtr);
			if (vkRes != VK_SUCCESS)
			{
				return ResultError(Failure, std::string("GrafBufferVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
			}
		}
		else
		{
			dstMappedPtr = (ur_byte*)this->mappedMemoryDataPtr + dstOffset;
		}

		writeCallback((ur_byte*)dstMappedPtr);

		if (localyMapped)
		{
			vkUnmapMemory(vkDevice, this->vkDeviceMemory);
		}

		return Result(Success);
	}

	Result GrafBufferVulkan::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		if (0 == dataSize)
			dataSize = this->GetDesc().SizeInBytes; // entire allocation range

		if (ur_null == dataPtr || srcOffset + dataSize > this->vkDeviceMemorySize)
			return Result(InvalidArgs);

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		void* srcMappedPtr = ur_null;
		ur_bool localyMapped = (ur_null == this->mappedMemoryDataPtr);
		if (localyMapped)
		{
			VkResult vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset + srcOffset, dataSize, 0, &srcMappedPtr);
			if (vkRes != VK_SUCCESS)
			{
				return ResultError(Failure, std::string("GrafBufferVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
			}
		}
		else
		{
			srcMappedPtr = (ur_byte*)this->mappedMemoryDataPtr + srcOffset;
		}

		memcpy(dataPtr + dstOffset, srcMappedPtr, dataSize);

		if (localyMapped)
		{
			vkUnmapMemory(vkDevice, this->vkDeviceMemory);
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafSamplerVulkan::GrafSamplerVulkan(GrafSystem &grafSystem) :
		GrafSampler(grafSystem)
	{
		this->vkSampler = VK_NULL_HANDLE;
	}

	GrafSamplerVulkan::~GrafSamplerVulkan()
	{
		this->Deinitialize();
	}

	Result GrafSamplerVulkan::Deinitialize()
	{
		if (this->vkSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkSampler, ur_null);
			this->vkSampler = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafSamplerVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafSampler::Initialize(grafDevice, initParams);

		// validate logical device

		GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafSamplerVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// create sampler object

		VkSamplerCreateInfo vkSamplerInfo = {};
		vkSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		vkSamplerInfo.flags = 0;
		vkSamplerInfo.magFilter = GrafUtilsVulkan::GrafToVkFilter(initParams.SamplerDesc.FilterMag);
		vkSamplerInfo.minFilter = GrafUtilsVulkan::GrafToVkFilter(initParams.SamplerDesc.FilterMin);
		vkSamplerInfo.mipmapMode = (VkSamplerMipmapMode)GrafUtilsVulkan::GrafToVkFilter(initParams.SamplerDesc.FilterMip);
		vkSamplerInfo.addressModeU = GrafUtilsVulkan::GrafToVkAddressMode(initParams.SamplerDesc.AddressModeU);
		vkSamplerInfo.addressModeV = GrafUtilsVulkan::GrafToVkAddressMode(initParams.SamplerDesc.AddressModeV);
		vkSamplerInfo.addressModeW = GrafUtilsVulkan::GrafToVkAddressMode(initParams.SamplerDesc.AddressModeW);
		vkSamplerInfo.anisotropyEnable = initParams.SamplerDesc.AnisoFilterEanbled;
		vkSamplerInfo.maxAnisotropy = initParams.SamplerDesc.AnisoFilterMax;
		vkSamplerInfo.mipLodBias = initParams.SamplerDesc.MipLodBias;
		vkSamplerInfo.minLod = initParams.SamplerDesc.MipLodMin;
		vkSamplerInfo.maxLod = initParams.SamplerDesc.MipLodMax;
		vkSamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		vkSamplerInfo.unnormalizedCoordinates = VK_FALSE;
		vkSamplerInfo.compareEnable = VK_FALSE;
		vkSamplerInfo.compareOp = VK_COMPARE_OP_NEVER;

		VkResult vkRes = vkCreateSampler(vkDevice, &vkSamplerInfo, ur_null, &this->vkSampler);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafSamplerVulkan: vkCreateSampler failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafShaderVulkan::GrafShaderVulkan(GrafSystem &grafSystem) :
		GrafShader(grafSystem)
	{
		this->vkShaderModule = VK_NULL_HANDLE;
		this->moduleOwner = true;
	}

	GrafShaderVulkan::~GrafShaderVulkan()
	{
		this->Deinitialize();
	}

	Result GrafShaderVulkan::Deinitialize()
	{
		if (this->vkShaderModule != VK_NULL_HANDLE)
		{
			if (this->moduleOwner)
			{
				vkDestroyShaderModule(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkShaderModule, ur_null);
			}
			this->vkShaderModule = VK_NULL_HANDLE;
		}
		this->moduleOwner = true;

		return Result(Success);
	}

	Result GrafShaderVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafShader::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafShaderVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// create shader module

		VkShaderModuleCreateInfo vkShaderInfo = {};
		vkShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vkShaderInfo.pNext = ur_null;
		vkShaderInfo.flags = 0;
		vkShaderInfo.codeSize = initParams.ByteCodeSize;
		vkShaderInfo.pCode = (const uint32_t*)initParams.ByteCode;

		VkResult vkRes = vkCreateShaderModule(vkDevice, &vkShaderInfo, ur_null, &this->vkShaderModule);
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafShaderVulkan: vkCreateShaderModule failed with VkResult = ") + VkResultToString(vkRes));

		this->moduleOwner = true;

		return Result(Success);
	}

	Result GrafShaderVulkan::InitializeFromVkShaderModule(GrafDevice *grafDevice, const InitParams& initParams, VkShaderModule vkShaderModule)
	{
		this->Deinitialize();

		GrafShader::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafShaderVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// this shader references extranl shader module

		this->vkShaderModule = vkShaderModule;
		this->moduleOwner = false;

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafShaderLibVulkan::GrafShaderLibVulkan(GrafSystem &grafSystem) :
		GrafShaderLib(grafSystem)
	{
		this->vkShaderModule = VK_NULL_HANDLE;
	}

	GrafShaderLibVulkan::~GrafShaderLibVulkan()
	{
		this->Deinitialize();
	}

	Result GrafShaderLibVulkan::Deinitialize()
	{
		this->shaders.clear();

		if (this->vkShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkShaderModule, ur_null);
			this->vkShaderModule = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafShaderLibVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafShaderLib::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafShaderLibVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// create shader module

		VkShaderModuleCreateInfo vkShaderInfo = {};
		vkShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vkShaderInfo.pNext = ur_null;
		vkShaderInfo.flags = 0;
		vkShaderInfo.codeSize = initParams.ByteCodeSize;
		vkShaderInfo.pCode = (const uint32_t*)initParams.ByteCode;

		VkResult vkRes = vkCreateShaderModule(vkDevice, &vkShaderInfo, ur_null, &this->vkShaderModule);
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafShaderLibVulkan: vkCreateShaderModule failed with VkResult = ") + VkResultToString(vkRes));

		// initialize shaders

		Result res(Success);
		this->shaders.reserve(initParams.EntryPointCount);
		for (ur_uint ientry = 0; ientry < initParams.EntryPointCount; ++ientry)
		{
			const GrafShaderLib::EntryPoint& libEntryPoint = initParams.EntryPoints[ientry];
			GrafShader::InitParams grafShaderParams = {};
			grafShaderParams.ShaderType = libEntryPoint.Type;
			grafShaderParams.EntryPoint = libEntryPoint.Name;
			
			std::unique_ptr<GrafShader> grafShader;
			this->GetGrafSystem().CreateShader(grafShader);
			Result shaderRes = static_cast<GrafShaderVulkan*>(grafShader.get())->InitializeFromVkShaderModule(grafDevice, grafShaderParams, this->vkShaderModule);
			if (Failed(shaderRes))
			{
				LogError(std::string("GrafShaderLibVulkan: failed to initialize shader for entry point = ") + libEntryPoint.Name);
				continue;
			}
			res &= shaderRes;
			
			this->shaders.push_back(std::move(grafShader));
		}

		return res;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRenderPassVulkan::GrafRenderPassVulkan(GrafSystem& grafSystem) :
		GrafRenderPass(grafSystem)
	{
		this->vkRenderPass = VK_NULL_HANDLE;
	}

	GrafRenderPassVulkan::~GrafRenderPassVulkan()
	{
		this->Deinitialize();
	}

	Result GrafRenderPassVulkan::Deinitialize()
	{
		this->renderPassImageDescs.clear();

		if (this->vkRenderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkRenderPass, ur_null);
			this->vkRenderPass = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafRenderPassVulkan::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafRenderPass::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// describe all used attachments

		std::vector<VkAttachmentDescription> vkAttachments(initParams.PassDesc.ImageCount);
		ur_uint colorAttachmentCount = 0;
		ur_uint depthStencilAttachmentCount = 0;
		for (ur_uint iimage = 0; iimage < initParams.PassDesc.ImageCount; ++iimage)
		{
			GrafRenderPassImageDesc &grafPassImage = initParams.PassDesc.Images[iimage];
			VkAttachmentDescription &vkAttachmentDesc = vkAttachments[iimage];
			vkAttachmentDesc.flags = 0;
			vkAttachmentDesc.format = GrafUtilsVulkan::GrafToVkFormat(grafPassImage.Format);
			vkAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			vkAttachmentDesc.loadOp = GrafUtilsVulkan::GrafToVkLoadOp(grafPassImage.LoadOp);
			vkAttachmentDesc.storeOp = GrafUtilsVulkan::GrafToVkStoreOp(grafPassImage.StoreOp);
			vkAttachmentDesc.stencilLoadOp = GrafUtilsVulkan::GrafToVkLoadOp(grafPassImage.StencilLoadOp);;
			vkAttachmentDesc.stencilStoreOp = GrafUtilsVulkan::GrafToVkStoreOp(grafPassImage.StencilStoreOp);
			vkAttachmentDesc.initialLayout = GrafUtilsVulkan::GrafToVkImageLayout(grafPassImage.InitialState);
			vkAttachmentDesc.finalLayout = GrafUtilsVulkan::GrafToVkImageLayout(grafPassImage.FinalState);
			if (GrafUtils::IsDepthStencilFormat(grafPassImage.Format))
				++depthStencilAttachmentCount;
			else
				++colorAttachmentCount;
		}

		// default subpass
		// TODO: should be configurable thriyght GRAF desc structure(s)

		std::vector<VkAttachmentReference> vkColorAttachmentRefs(colorAttachmentCount);
		VkAttachmentReference vkDepthStencilAttachment;
		ur_uint colorAttachmentIdx = 0;
		for (ur_uint iimage = 0; iimage < initParams.PassDesc.ImageCount; ++iimage)
		{
			GrafRenderPassImageDesc &grafPassImage = initParams.PassDesc.Images[iimage];
			if (GrafUtils::IsDepthStencilFormat(grafPassImage.Format))
			{
				vkDepthStencilAttachment.attachment = (ur_uint32)iimage;
				vkDepthStencilAttachment.layout = vkAttachments[iimage].finalLayout;
			}
			else
			{
				VkAttachmentReference &vkColorAttachmentRef = vkColorAttachmentRefs[colorAttachmentIdx++];
				vkColorAttachmentRef.attachment = (ur_uint32)iimage;
				vkColorAttachmentRef.layout = vkAttachments[iimage].finalLayout;
			}
		}
		std::vector<VkAttachmentReference> vkDepthStencilAttachmentRefs;
		VkSubpassDescription vkSubpassDesc = {};
		vkSubpassDesc.flags = 0;
		vkSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkSubpassDesc.inputAttachmentCount = 0;
		vkSubpassDesc.pInputAttachments = ur_null;
		vkSubpassDesc.colorAttachmentCount = (ur_uint32)vkColorAttachmentRefs.size();
		vkSubpassDesc.pColorAttachments = vkColorAttachmentRefs.data();
		vkSubpassDesc.pResolveAttachments = ur_null;
		vkSubpassDesc.pDepthStencilAttachment = (depthStencilAttachmentCount > 0 ? &vkDepthStencilAttachment : ur_null);
		vkSubpassDesc.preserveAttachmentCount = 0;
		vkSubpassDesc.pPreserveAttachments = ur_null;

		// create render pass

		VkRenderPassCreateInfo vkRenderPassInfo = {};
		vkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		vkRenderPassInfo.flags = 0;
		vkRenderPassInfo.attachmentCount = (ur_uint32)vkAttachments.size();
		vkRenderPassInfo.pAttachments = vkAttachments.data();
		vkRenderPassInfo.subpassCount = 1;
		vkRenderPassInfo.pSubpasses = &vkSubpassDesc;
		vkRenderPassInfo.dependencyCount = 0;
		vkRenderPassInfo.pDependencies = ur_null;

		VkResult vkRes = vkCreateRenderPass(vkDevice, &vkRenderPassInfo, ur_null, &this->vkRenderPass);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRenderPassVulkan: vkCreateRenderPass failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRenderTargetVulkan::GrafRenderTargetVulkan(GrafSystem &grafSystem) :
		GrafRenderTarget(grafSystem)
	{
		this->vkFramebuffer = VK_NULL_HANDLE;
	}

	GrafRenderTargetVulkan::~GrafRenderTargetVulkan()
	{
		this->Deinitialize();
	}

	Result GrafRenderTargetVulkan::Deinitialize()
	{
		if (this->vkFramebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkFramebuffer, ur_null);
			this->vkFramebuffer = VK_NULL_HANDLE;
		}

		this->images.clear();
		this->renderPass = ur_null;

		return Result(Success);
	}

	Result GrafRenderTargetVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		if (ur_null == initParams.RenderPass || 0 == initParams.ImageCount)
			return Result(InvalidArgs);

		GrafRenderTarget::Initialize(grafDevice, initParams);

		// validate logical device

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// create frame buffer object

		std::vector<VkImageView> attachmentViews(initParams.ImageCount);
		for (ur_size iimage = 0; iimage < initParams.ImageCount; ++iimage)
		{
			attachmentViews[iimage] = static_cast<GrafImageVulkan*>(initParams.Images[iimage])->GetVkImageView();
		}

		VkFramebufferCreateInfo vkFramebufferInfo = {};
		vkFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		vkFramebufferInfo.flags = 0;
		vkFramebufferInfo.renderPass = static_cast<GrafRenderPassVulkan*>(initParams.RenderPass)->GetVkRenderPass();
		vkFramebufferInfo.attachmentCount = (ur_uint32)attachmentViews.size();
		vkFramebufferInfo.pAttachments = attachmentViews.data();
		vkFramebufferInfo.width = (ur_uint32)initParams.Images[0]->GetDesc().Size.x;
		vkFramebufferInfo.height = (ur_uint32)initParams.Images[0]->GetDesc().Size.y;
		vkFramebufferInfo.layers = 1;

		VkResult vkRes = vkCreateFramebuffer(vkDevice, &vkFramebufferInfo, ur_null, &this->vkFramebuffer);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRenderTargetVulkan: vkCreateFramebuffer failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableLayoutVulkan::GrafDescriptorTableLayoutVulkan(GrafSystem &grafSystem) :
		GrafDescriptorTableLayout(grafSystem)
	{
		this->vkDescriptorSetLayout = VK_NULL_HANDLE;
	}

	GrafDescriptorTableLayoutVulkan::~GrafDescriptorTableLayoutVulkan()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableLayoutVulkan::Deinitialize()
	{
		if (this->vkDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDescriptorSetLayout, ur_null);
			this->vkDescriptorSetLayout = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafDescriptorTableLayoutVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafDescriptorTableLayout::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafPipelineVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		// initialize bindings

		ur_uint bindingsTotalCount = 0;
		for (ur_uint irange = 0; irange < initParams.LayoutDesc.DescriptorRangeCount; ++irange)
		{
			const GrafDescriptorRangeDesc& rangeDesc = initParams.LayoutDesc.DescriptorRanges[irange];
			bindingsTotalCount += (GrafUtils::IsDescriptorTypeWithDynamicIndexing(rangeDesc.Type) ?
				1 : rangeDesc.BindingCount);
		}

		std::vector<VkDescriptorSetLayoutBinding> vkDescriptorSetBindingInfos(bindingsTotalCount);
		std::vector<VkDescriptorBindingFlags> vkDescriptorSetBindingFlags(bindingsTotalCount);
		ur_uint bindingGlobalIdx = 0;
		for (ur_uint irange = 0; irange < initParams.LayoutDesc.DescriptorRangeCount; ++irange)
		{
			const GrafDescriptorRangeDesc& rangeDesc = initParams.LayoutDesc.DescriptorRanges[irange];
			ur_uint32 bindingCount = rangeDesc.BindingCount;
			ur_uint32 descriptorCount = 1;
			ur_bool descriptorWithDynamicIndexing = GrafUtils::IsDescriptorTypeWithDynamicIndexing(rangeDesc.Type);
			if (descriptorWithDynamicIndexing)
			{
				descriptorCount = bindingCount;
				bindingCount = 1;
			}

			for (ur_uint ibinding = 0; ibinding < bindingCount; ++ibinding, ++bindingGlobalIdx)
			{
				VkDescriptorSetLayoutBinding& vkDescriptorSetBindingInfo = vkDescriptorSetBindingInfos[bindingGlobalIdx];
				vkDescriptorSetBindingInfo.binding = ibinding + rangeDesc.BindingOffset + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(rangeDesc.Type);
				vkDescriptorSetBindingInfo.descriptorType = GrafUtilsVulkan::GrafToVkDescriptorType(rangeDesc.Type);
				vkDescriptorSetBindingInfo.descriptorCount = descriptorCount;
				vkDescriptorSetBindingInfo.stageFlags = GrafUtilsVulkan::GrafToVkShaderStage(initParams.LayoutDesc.ShaderStageVisibility);
				vkDescriptorSetBindingInfo.pImmutableSamplers = ur_null;
				
				VkDescriptorBindingFlags& vkDescriptorSetBindingFlag = vkDescriptorSetBindingFlags[bindingGlobalIdx];
				vkDescriptorSetBindingFlag = (descriptorWithDynamicIndexing ?
					VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
			}
		}

		// create descriptor set layout

		VkDescriptorSetLayoutBindingFlagsCreateInfo vkDescriptorSetFlagsInfo = {};
		vkDescriptorSetFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		vkDescriptorSetFlagsInfo.bindingCount = (ur_uint32)vkDescriptorSetBindingFlags.size();;
		vkDescriptorSetFlagsInfo.pBindingFlags = vkDescriptorSetBindingFlags.data();

		VkDescriptorSetLayoutCreateInfo vkDescriptorSetInfo = {};
		vkDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		vkDescriptorSetInfo.pNext = &vkDescriptorSetFlagsInfo;
		vkDescriptorSetInfo.bindingCount = (ur_uint32)vkDescriptorSetBindingInfos.size();
		vkDescriptorSetInfo.pBindings = vkDescriptorSetBindingInfos.data();

		VkResult vkRes = vkCreateDescriptorSetLayout(vkDevice, &vkDescriptorSetInfo, ur_null, &this->vkDescriptorSetLayout);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDescriptorTableLayoutVulkan: vkCreateDescriptorSetLayout failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableVulkan::GrafDescriptorTableVulkan(GrafSystem &grafSystem) :
		GrafDescriptorTable(grafSystem)
	{
		this->vkDescriptorSet = VK_NULL_HANDLE;
		this->vkDescriptorPool = VK_NULL_HANDLE;
	}

	Result GrafDescriptorTableVulkan::Deinitialize()
	{
		this->layout = ur_null;
		if (this->vkDescriptorSet != VK_NULL_HANDLE)
		{
			vkFreeDescriptorSets(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDescriptorPool, 1, &this->vkDescriptorSet);
			this->vkDescriptorSet = VK_NULL_HANDLE;
		}
		this->vkDescriptorPool = VK_NULL_HANDLE;

		return Result(Success);
	}

	GrafDescriptorTableVulkan::~GrafDescriptorTableVulkan()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		if (ur_null == initParams.Layout)
			return Result(InvalidArgs);

		GrafDescriptorTable::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafPipelineVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

		#if (0)
		// TEST: create pool per table

		GrafDescriptorTableLayoutVulkan* tableLayoutVulkan = static_cast<GrafDescriptorTableLayoutVulkan*>(initParams.Layout);
		const GrafDescriptorTableLayoutDesc& layoutDesc = tableLayoutVulkan->GetLayoutDesc();
		VkDescriptorPoolSize vkDescriptorPoolSizeUnused = {};
		std::vector<VkDescriptorPoolSize> vkDescriptorPoolSizesAll((ur_uint)GrafDescriptorType::Count, vkDescriptorPoolSizeUnused);
		for (ur_uint irange = 0; irange < layoutDesc.DescriptorRangeCount; ++irange)
		{
			const GrafDescriptorRangeDesc& rangeDesc = layoutDesc.DescriptorRanges[irange];
			ur_uint poolIdx = (ur_uint)rangeDesc.Type;
			vkDescriptorPoolSizesAll[poolIdx].type = GrafUtilsVulkan::GrafToVkDescriptorType(rangeDesc.Type);
			vkDescriptorPoolSizesAll[poolIdx].descriptorCount += rangeDesc.BindingCount;
		}
		std::vector<VkDescriptorPoolSize> vkDescriptorPoolSizes;
		vkDescriptorPoolSizes.reserve((ur_uint)GrafDescriptorType::Count);
		for (auto& vkDescriptorPoolSize : vkDescriptorPoolSizesAll)
		{
			if (vkDescriptorPoolSize.descriptorCount > 0)
			{
				vkDescriptorPoolSizes.push_back(vkDescriptorPoolSize);
			}
		}

		VkDescriptorPoolCreateInfo vkDescriptorPoolInfo = {};
		vkDescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		vkDescriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		vkDescriptorPoolInfo.maxSets = 1;
		vkDescriptorPoolInfo.poolSizeCount = (ur_uint32)vkDescriptorPoolSizes.size();
		vkDescriptorPoolInfo.pPoolSizes = vkDescriptorPoolSizes.data();
		VkResult vkRes = vkCreateDescriptorPool(vkDevice, &vkDescriptorPoolInfo, ur_null, &this->vkDescriptorPool);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDescriptorTableVulkan: vkCreateDescriptorPool failed with VkResult = ") + VkResultToString(vkRes));
		}
		#endif

		// prepare info for descriptors with dynamic indexing

		GrafDescriptorTableLayoutVulkan* tableLayoutVulkan = static_cast<GrafDescriptorTableLayoutVulkan*>(initParams.Layout);
		const GrafDescriptorTableLayoutDesc& tableLayoutDesc = tableLayoutVulkan->GetLayoutDesc();
		std::vector<ur_uint32> variableDescriptorCounts;
		for (ur_uint irange = 0; irange < tableLayoutDesc.DescriptorRangeCount; ++irange)
		{
			if (GrafUtils::IsDescriptorTypeWithDynamicIndexing(tableLayoutDesc.DescriptorRanges[irange].Type))
			{
				variableDescriptorCounts.emplace_back(tableLayoutDesc.DescriptorRanges[irange].BindingCount);
			}
		}

		// allocate descriptor set
		
		this->vkDescriptorPool = grafDeviceVulkan->GetVkDescriptorPool();

		VkDescriptorSetVariableDescriptorCountAllocateInfo vkVariableDescriptorCountInfo = {};
		vkVariableDescriptorCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		vkVariableDescriptorCountInfo.descriptorSetCount = 1;
		vkVariableDescriptorCountInfo.pDescriptorCounts = variableDescriptorCounts.data();

		VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
		vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		vkDescriptorSetAllocateInfo.descriptorPool = this->vkDescriptorPool;
		VkDescriptorSetLayout vkDescritorSetLayouts[] = { tableLayoutVulkan->GetVkDescriptorSetLayout() };
		vkDescriptorSetAllocateInfo.pSetLayouts = vkDescritorSetLayouts;
		vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
		vkDescriptorSetAllocateInfo.pNext = (variableDescriptorCounts.empty() ? ur_null : &vkVariableDescriptorCountInfo);
		VkResult vkRes = vkAllocateDescriptorSets(vkDevice, &vkDescriptorSetAllocateInfo, &this->vkDescriptorSet);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafDescriptorTableVulkan: vkAllocateDescriptorSets failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs, ur_size bufferRange)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
		vkDescriptorBufferInfo.buffer = static_cast<GrafBufferVulkan*>(buffer)->GetVkBuffer();
		vkDescriptorBufferInfo.offset = (VkDeviceSize)bufferOfs;
		vkDescriptorBufferInfo.range = (VkDeviceSize)(bufferRange > 0 ? bufferRange : buffer->GetDesc().SizeInBytes);

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::ConstantBuffer);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vkWriteDescriptorSet.pImageInfo = ur_null;
		vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler)
	{
		GrafImageVulkan* grafImageVulkan = static_cast<GrafImageVulkan*>(image);
		return SetSampledImage(bindingIdx, grafImageVulkan->GetDefaultSubresource(), sampler);
	}

	Result GrafDescriptorTableVulkan::SetSampledImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource, GrafSampler* sampler)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorImageInfo vkDescriptorImageInfo = {};
		vkDescriptorImageInfo.sampler = static_cast<GrafSamplerVulkan*>(sampler)->GetVkSampler();
		vkDescriptorImageInfo.imageView = static_cast<GrafImageSubresourceVulkan*>(imageSubresource)->GetVkImageView();
		vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet vkWriteDescriptorSets[2] = { {}, {} };
		{
			VkWriteDescriptorSet &vkWriteDescriptorSet = vkWriteDescriptorSets[0];
			vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vkWriteDescriptorSet.pNext = ur_null;
			vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
			vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Sampler);
			vkWriteDescriptorSet.dstArrayElement = 0;
			vkWriteDescriptorSet.descriptorCount = 1;
			vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
			vkWriteDescriptorSet.pBufferInfo = ur_null;
			vkWriteDescriptorSet.pTexelBufferView = ur_null;
		}
		{
			VkWriteDescriptorSet &vkWriteDescriptorSet = vkWriteDescriptorSets[1];
			vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vkWriteDescriptorSet.pNext = ur_null;
			vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
			vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Texture);
			vkWriteDescriptorSet.dstArrayElement = 0;
			vkWriteDescriptorSet.descriptorCount = 1;
			vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
			vkWriteDescriptorSet.pBufferInfo = ur_null;
			vkWriteDescriptorSet.pTexelBufferView = ur_null;
		}

		vkUpdateDescriptorSets(vkDevice, 2, vkWriteDescriptorSets, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetSampler(ur_uint bindingIdx, GrafSampler* sampler)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorImageInfo vkDescriptorImageInfo = {};
		vkDescriptorImageInfo.sampler = static_cast<GrafSamplerVulkan*>(sampler)->GetVkSampler();
		vkDescriptorImageInfo.imageView = VK_NULL_HANDLE;
		vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Sampler);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
		vkWriteDescriptorSet.pBufferInfo = ur_null;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetImage(ur_uint bindingIdx, GrafImage* image)
	{
		GrafImageVulkan* grafImageVulkan = static_cast<GrafImageVulkan*>(image);
		return SetImage(bindingIdx, grafImageVulkan->GetDefaultSubresource());
	}

	Result GrafDescriptorTableVulkan::SetImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorImageInfo vkDescriptorImageInfo = {};
		vkDescriptorImageInfo.sampler = VK_NULL_HANDLE;
		vkDescriptorImageInfo.imageView = static_cast<GrafImageSubresourceVulkan*>(imageSubresource)->GetVkImageView();
		vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Texture);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
		vkWriteDescriptorSet.pBufferInfo = ur_null;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetImageArray(ur_uint bindingIdx, GrafImage** images, ur_uint imageCount)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		std::vector<VkDescriptorImageInfo> vkDescriptorImageInfoArray(imageCount);
		for (ur_uint imageIdx = 0; imageIdx < imageCount; ++imageIdx)
		{
			VkDescriptorImageInfo& vkDescriptorImageInfo = vkDescriptorImageInfoArray[imageIdx];
			vkDescriptorImageInfo.sampler = VK_NULL_HANDLE;
			vkDescriptorImageInfo.imageView = static_cast<GrafImageVulkan*>(images[imageIdx])->GetVkImageView();
			vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Texture);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = ur_uint32(imageCount);
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		vkWriteDescriptorSet.pImageInfo = vkDescriptorImageInfoArray.data();
		vkWriteDescriptorSet.pBufferInfo = ur_null;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
		vkDescriptorBufferInfo.buffer = static_cast<GrafBufferVulkan*>(buffer)->GetVkBuffer();
		vkDescriptorBufferInfo.offset = (VkDeviceSize)0;
		vkDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Buffer);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vkWriteDescriptorSet.pImageInfo = ur_null;
		vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetBufferArray(ur_uint bindingIdx, GrafBuffer** buffers, ur_uint bufferCount)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		std::vector<VkDescriptorBufferInfo> vkDescriptorBufferInfoArray(bufferCount);
		for (ur_uint bufferIdx = 0; bufferIdx < bufferCount; ++bufferIdx)
		{
			VkDescriptorBufferInfo& vkDescriptorBufferInfo = vkDescriptorBufferInfoArray[bufferIdx];
			vkDescriptorBufferInfo.buffer = static_cast<GrafBufferVulkan*>(buffers[bufferIdx])->GetVkBuffer();
			vkDescriptorBufferInfo.offset = (VkDeviceSize)0;
			vkDescriptorBufferInfo.range = VK_WHOLE_SIZE;
		}

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::Buffer);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = ur_uint32(bufferCount);
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vkWriteDescriptorSet.pImageInfo = ur_null;
		vkWriteDescriptorSet.pBufferInfo = vkDescriptorBufferInfoArray.data();
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetRWImage(ur_uint bindingIdx, GrafImage* image)
	{
		GrafImageVulkan* grafImageVulkan = static_cast<GrafImageVulkan*>(image);
		return SetRWImage(bindingIdx, grafImageVulkan->GetDefaultSubresource());
	}

	Result GrafDescriptorTableVulkan::SetRWImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorImageInfo vkDescriptorImageInfo = {};
		vkDescriptorImageInfo.sampler = VK_NULL_HANDLE;
		vkDescriptorImageInfo.imageView = static_cast<GrafImageSubresourceVulkan*>(imageSubresource)->GetVkImageView();
		vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::RWTexture);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		vkWriteDescriptorSet.pImageInfo = &vkDescriptorImageInfo;
		vkWriteDescriptorSet.pBufferInfo = ur_null;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();

		VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
		vkDescriptorBufferInfo.buffer = static_cast<GrafBufferVulkan*>(buffer)->GetVkBuffer();
		vkDescriptorBufferInfo.offset = (VkDeviceSize)0;
		vkDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = ur_null;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::RWBuffer);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vkWriteDescriptorSet.pImageInfo = ur_null;
		vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	}

	Result GrafDescriptorTableVulkan::SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();
		VkAccelerationStructureKHR vkAccelerationStructure = static_cast<GrafAccelerationStructureVulkan*>(accelerationStructure)->GetVkAccelerationStructure();

		VkWriteDescriptorSetAccelerationStructureKHR vkDescriptorAccelStructInfo = {};
		vkDescriptorAccelStructInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		vkDescriptorAccelStructInfo.pNext = ur_null;
		vkDescriptorAccelStructInfo.accelerationStructureCount = 1;
		vkDescriptorAccelStructInfo.pAccelerationStructures = &vkAccelerationStructure;

		VkWriteDescriptorSet vkWriteDescriptorSet = {};
		vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vkWriteDescriptorSet.pNext = &vkDescriptorAccelStructInfo;
		vkWriteDescriptorSet.dstSet = this->vkDescriptorSet;
		vkWriteDescriptorSet.dstBinding = bindingIdx + GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType::AccelerationStructure);
		vkWriteDescriptorSet.dstArrayElement = 0;
		vkWriteDescriptorSet.descriptorCount = 1;
		vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		vkWriteDescriptorSet.pImageInfo = ur_null;
		vkWriteDescriptorSet.pBufferInfo = ur_null;
		vkWriteDescriptorSet.pTexelBufferView = ur_null;

		vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafPipelineVulkan::GrafPipelineVulkan(GrafSystem &grafSystem) :
		GrafPipeline(grafSystem)
	{
		this->vkPipeline = VK_NULL_HANDLE;
		this->vkPipelineLayout = VK_NULL_HANDLE;
	}

	GrafPipelineVulkan::~GrafPipelineVulkan()
	{
		this->Deinitialize();
	}

	Result GrafPipelineVulkan::Deinitialize()
	{
		if (this->vkPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipelineLayout, ur_null);
			this->vkPipelineLayout = VK_NULL_HANDLE;
		}
		if (this->vkPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipeline, ur_null);
			this->vkPipeline = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafPipelineVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		if (ur_null == initParams.RenderPass)
			return Result(InvalidArgs);

		GrafPipeline::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafPipelineVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkRenderPass vkRenderPass = static_cast<GrafRenderPassVulkan*>(initParams.RenderPass)->GetVkRenderPass();
		VkResult vkRes = VK_SUCCESS;

		// shader stages

		std::vector<VkPipelineShaderStageCreateInfo> vkShaderStagesInfo(initParams.ShaderStageCount);
		for (ur_uint istage = 0; istage < initParams.ShaderStageCount; ++istage)
		{
			GrafShader* grafShader = initParams.ShaderStages[istage];
			VkPipelineShaderStageCreateInfo& vkShaderStageInfo = vkShaderStagesInfo[istage];
			vkShaderStageInfo = {};
			vkShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vkShaderStageInfo.flags = 0;
			vkShaderStageInfo.stage = GrafUtilsVulkan::GrafToVkShaderStage(grafShader->GetShaderType());
			vkShaderStageInfo.module = static_cast<GrafShaderVulkan*>(grafShader)->GetVkShaderModule();
			vkShaderStageInfo.pName = grafShader->GetEntryPoint().c_str();
		}

		// pipeline layout

		std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts(initParams.DescriptorTableLayoutCount);
		for (ur_uint ilayout = 0; ilayout < initParams.DescriptorTableLayoutCount; ++ilayout)
		{
			vkDescriptorSetLayouts[ilayout] = static_cast<GrafDescriptorTableLayoutVulkan*>(initParams.DescriptorTableLayouts[ilayout])->GetVkDescriptorSetLayout();
		}

		VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
		vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkPipelineLayoutInfo.flags = 0;
		vkPipelineLayoutInfo.setLayoutCount = (ur_uint32)vkDescriptorSetLayouts.size();
		vkPipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
		vkPipelineLayoutInfo.pushConstantRangeCount = 0;
		vkPipelineLayoutInfo.pPushConstantRanges = ur_null;

		vkRes = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutInfo, ur_null, &this->vkPipelineLayout);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafPipelineVulkan: vkCreatePipelineLayout failed with VkResult = ") + VkResultToString(vkRes));
		}

		// vertex input

		ur_uint vertexInputTotalElementCount = 0;
		for (ur_uint ivb = 0; ivb < initParams.VertexInputCount; ++ivb)
		{
			vertexInputTotalElementCount += initParams.VertexInputDesc[ivb].ElementCount;
		}

		std::vector<VkVertexInputAttributeDescription> vkVertexAttributes(vertexInputTotalElementCount);
		std::vector<VkVertexInputBindingDescription> vkVertexBindingDescs(initParams.VertexInputCount);
		ur_uint vertexAttributesOffset = 0;
		for (ur_uint ivb = 0; ivb < initParams.VertexInputCount; ++ivb)
		{
			const GrafVertexInputDesc& grafVertexInputDesc = initParams.VertexInputDesc[ivb];
			VkVertexInputBindingDescription& vkVertexBindingDesc = vkVertexBindingDescs[ivb];
			vkVertexBindingDesc.binding = (ur_uint32)grafVertexInputDesc.BindingIdx;
			vkVertexBindingDesc.stride = (ur_uint32)grafVertexInputDesc.Stride;
			vkVertexBindingDesc.inputRate = (GrafVertexInputType::PerInstance == grafVertexInputDesc.InputType ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);

			for (ur_uint ielem = 0; ielem < grafVertexInputDesc.ElementCount; ++ielem)
			{
				const GrafVertexElementDesc& grafVertexElement = grafVertexInputDesc.Elements[ielem];
				VkVertexInputAttributeDescription& vkVertexAttribute = vkVertexAttributes[vertexAttributesOffset + ielem];
				vkVertexAttribute.location = (ur_uint32)ielem;
				vkVertexAttribute.binding = (ur_uint32)ivb;
				vkVertexAttribute.format = GrafUtilsVulkan::GrafToVkFormat(grafVertexElement.Format);
				vkVertexAttribute.offset = (ur_uint32)grafVertexElement.Offset;
			}
			vertexAttributesOffset += grafVertexInputDesc.ElementCount;
		}

		VkPipelineVertexInputStateCreateInfo vkVertxInputStateInfo = {};
		vkVertxInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vkVertxInputStateInfo.flags = 0;
		vkVertxInputStateInfo.vertexBindingDescriptionCount = (ur_uint32)vkVertexBindingDescs.size();
		vkVertxInputStateInfo.pVertexBindingDescriptions = vkVertexBindingDescs.data();
		vkVertxInputStateInfo.vertexAttributeDescriptionCount = (ur_uint32)vkVertexAttributes.size();
		vkVertxInputStateInfo.pVertexAttributeDescriptions = vkVertexAttributes.data();

		// input assembly

		VkPipelineInputAssemblyStateCreateInfo vkInputAssemblyInfo = {};
		vkInputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		vkInputAssemblyInfo.flags = 0;
		vkInputAssemblyInfo.topology = GrafUtilsVulkan::GrafToVkPrimitiveTopology(initParams.PrimitiveTopology);
		vkInputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		//  tessellation state

		// VkPipelineTessellationStateCreateInfo;

		// viewport & scissors

		VkViewport vkViewport = {};
		vkViewport.x = initParams.ViewportDesc.X;
		vkViewport.y = initParams.ViewportDesc.Y;
		vkViewport.width = initParams.ViewportDesc.Width;
		vkViewport.height = initParams.ViewportDesc.Height;
		vkViewport.minDepth = initParams.ViewportDesc.Near;
		vkViewport.maxDepth = initParams.ViewportDesc.Far;

		VkRect2D vkScissorRect = { { (int32_t)vkViewport.x, (int32_t)vkViewport.y}, { (uint32_t)vkViewport.width, (uint32_t)vkViewport.height} };

		VkPipelineViewportStateCreateInfo vkViewportStateInfo = {};
		vkViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vkViewportStateInfo.flags = 0;
		vkViewportStateInfo.viewportCount = 1;
		vkViewportStateInfo.pViewports = &vkViewport;
		vkViewportStateInfo.scissorCount = 1;
		vkViewportStateInfo.pScissors = &vkScissorRect;

		// rasterizer state

		VkPipelineRasterizationStateCreateInfo vkRasterStateInfo = {};
		vkRasterStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		vkRasterStateInfo.flags = 0;
		vkRasterStateInfo.depthClampEnable = VK_FALSE;
		vkRasterStateInfo.rasterizerDiscardEnable = VK_FALSE;
		vkRasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		vkRasterStateInfo.cullMode = GrafUtilsVulkan::GrafToVkCullModeFlags(initParams.CullMode);
		vkRasterStateInfo.frontFace = GrafUtilsVulkan::GrafToVkFrontFace(initParams.FrontFaceOrder);
		vkRasterStateInfo.depthBiasEnable = VK_FALSE;
		vkRasterStateInfo.depthBiasConstantFactor = 0.0f;
		vkRasterStateInfo.depthBiasClamp = 0.0f;
		vkRasterStateInfo.depthBiasSlopeFactor = 0.0f;
		vkRasterStateInfo.lineWidth = 1.0f;

		// multisampling

		VkPipelineMultisampleStateCreateInfo vkMultisampleStateInfo = {};
		vkMultisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		vkMultisampleStateInfo.flags = 0;
		vkMultisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		vkMultisampleStateInfo.sampleShadingEnable = VK_FALSE;
		vkMultisampleStateInfo.minSampleShading = 1.0f;
		vkMultisampleStateInfo.pSampleMask = ur_null;
		vkMultisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
		vkMultisampleStateInfo.alphaToOneEnable = VK_FALSE;

		// depth stencil state

		VkPipelineDepthStencilStateCreateInfo vkDepthStencilStateInfo = {};
		vkDepthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		vkDepthStencilStateInfo.flags = 0;
		vkDepthStencilStateInfo.depthTestEnable = initParams.DepthTestEnable;
		vkDepthStencilStateInfo.depthWriteEnable = initParams.DepthWriteEnable;
		vkDepthStencilStateInfo.depthCompareOp = GrafUtilsVulkan::GrafToVkCompareOp(initParams.DepthCompareOp);
		vkDepthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
		vkDepthStencilStateInfo.stencilTestEnable = initParams.StencilTestEnable;
		vkDepthStencilStateInfo.front.failOp = GrafUtilsVulkan::GrafToVkStencilOp(initParams.StencilFront.FailOp);
		vkDepthStencilStateInfo.front.passOp = GrafUtilsVulkan::GrafToVkStencilOp(initParams.StencilFront.PassOp);
		vkDepthStencilStateInfo.front.depthFailOp = GrafUtilsVulkan::GrafToVkStencilOp(initParams.StencilFront.DepthFailOp);
		vkDepthStencilStateInfo.front.compareOp = GrafUtilsVulkan::GrafToVkCompareOp(initParams.StencilFront.CompareOp);
		vkDepthStencilStateInfo.front.compareMask = initParams.StencilFront.CompareMask;
		vkDepthStencilStateInfo.front.writeMask = initParams.StencilFront.WriteMask;
		vkDepthStencilStateInfo.front.reference = initParams.StencilFront.Reference;
		vkDepthStencilStateInfo.back.failOp = GrafUtilsVulkan::GrafToVkStencilOp(initParams.StencilBack.FailOp);
		vkDepthStencilStateInfo.back.passOp = GrafUtilsVulkan::GrafToVkStencilOp(initParams.StencilBack.PassOp);
		vkDepthStencilStateInfo.back.depthFailOp = GrafUtilsVulkan::GrafToVkStencilOp(initParams.StencilBack.DepthFailOp);
		vkDepthStencilStateInfo.back.compareOp = GrafUtilsVulkan::GrafToVkCompareOp(initParams.StencilBack.CompareOp);
		vkDepthStencilStateInfo.back.compareMask = initParams.StencilBack.CompareMask;
		vkDepthStencilStateInfo.back.writeMask = initParams.StencilBack.WriteMask;
		vkDepthStencilStateInfo.back.reference = initParams.StencilBack.Reference;
		vkDepthStencilStateInfo.minDepthBounds = 0.0f;
		vkDepthStencilStateInfo.maxDepthBounds = 0.0f;

		// color blend state

		std::vector<VkPipelineColorBlendAttachmentState> vkAttachmentBlendStates(initParams.ColorBlendOpDescCount);
		for (ur_uint iimg = 0; iimg < initParams.ColorBlendOpDescCount; ++iimg)
		{
			GrafColorBlendOpDesc& blendOpDesc = initParams.ColorBlendOpDesc[iimg];
			VkPipelineColorBlendAttachmentState& vkBlendState = vkAttachmentBlendStates[iimg];
			vkBlendState.blendEnable = blendOpDesc.BlendEnable;
			vkBlendState.srcColorBlendFactor = GrafUtilsVulkan::GrafToVkBlendFactor(blendOpDesc.SrcColorFactor);
			vkBlendState.dstColorBlendFactor = GrafUtilsVulkan::GrafToVkBlendFactor(blendOpDesc.DstColorFactor);
			vkBlendState.colorBlendOp = GrafUtilsVulkan::GrafToVkBlendOp(blendOpDesc.ColorOp);
			vkBlendState.srcAlphaBlendFactor = GrafUtilsVulkan::GrafToVkBlendFactor(blendOpDesc.SrcAlphaFactor);
			vkBlendState.dstAlphaBlendFactor = GrafUtilsVulkan::GrafToVkBlendFactor(blendOpDesc.DstAlphaFactor);
			vkBlendState.alphaBlendOp = GrafUtilsVulkan::GrafToVkBlendOp(blendOpDesc.AlphaOp);
			vkBlendState.colorWriteMask = VkColorComponentFlags(blendOpDesc.WriteMask);
		}

		VkPipelineColorBlendStateCreateInfo vkColorBlendStateInfo = {};
		vkColorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		vkColorBlendStateInfo.flags = 0;
		vkColorBlendStateInfo.logicOpEnable = VK_FALSE;
		vkColorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
		vkColorBlendStateInfo.attachmentCount = (ur_uint32)vkAttachmentBlendStates.size();
		vkColorBlendStateInfo.pAttachments = vkAttachmentBlendStates.data();
		vkColorBlendStateInfo.blendConstants[0] = 0.0f;
		vkColorBlendStateInfo.blendConstants[1] = 0.0f;
		vkColorBlendStateInfo.blendConstants[2] = 0.0f;
		vkColorBlendStateInfo.blendConstants[3] = 0.0f;

		// dynamic state

		VkDynamicState vkDynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			//VK_DYNAMIC_STATE_LINE_WIDTH, // disabled for abstraction compatibility with DirectX, line width is always 1.0
			VK_DYNAMIC_STATE_DEPTH_BIAS,
			VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VK_DYNAMIC_STATE_DEPTH_BOUNDS,
			//VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
			//VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
			//VK_DYNAMIC_STATE_STENCIL_REFERENCE
		};

		VkPipelineDynamicStateCreateInfo vkDynamicStateInfo = {};
		vkDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		vkDynamicStateInfo.flags = 0;
		vkDynamicStateInfo.dynamicStateCount = ur_array_size(vkDynamicStates);
		vkDynamicStateInfo.pDynamicStates = vkDynamicStates;

		// create pipeline object

		VkGraphicsPipelineCreateInfo vkPipelineInfo = {};
		vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		vkPipelineInfo.flags = 0; // NOTE: consider VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR for debug purpose
		vkPipelineInfo.stageCount = (ur_uint32)vkShaderStagesInfo.size();
		vkPipelineInfo.pStages = vkShaderStagesInfo.data();
		vkPipelineInfo.pVertexInputState = &vkVertxInputStateInfo;
		vkPipelineInfo.pInputAssemblyState = &vkInputAssemblyInfo;
		vkPipelineInfo.pTessellationState = ur_null;
		vkPipelineInfo.pViewportState = &vkViewportStateInfo;
		vkPipelineInfo.pRasterizationState = &vkRasterStateInfo;
		vkPipelineInfo.pMultisampleState = &vkMultisampleStateInfo;
		vkPipelineInfo.pDepthStencilState = &vkDepthStencilStateInfo;
		vkPipelineInfo.pColorBlendState = &vkColorBlendStateInfo;
		vkPipelineInfo.pDynamicState = &vkDynamicStateInfo;
		vkPipelineInfo.layout = this->vkPipelineLayout;
		vkPipelineInfo.renderPass = vkRenderPass;
		vkPipelineInfo.subpass = 0;
		vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		vkPipelineInfo.basePipelineIndex = -1;

		vkRes = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, ur_null, &this->vkPipeline);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafPipelineVulkan: vkCreateGraphicsPipelines failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafComputePipelineVulkan::GrafComputePipelineVulkan(GrafSystem &grafSystem) :
		GrafComputePipeline(grafSystem)
	{
		this->vkPipeline = VK_NULL_HANDLE;
		this->vkPipelineLayout = VK_NULL_HANDLE;
	}

	GrafComputePipelineVulkan::~GrafComputePipelineVulkan()
	{
		this->Deinitialize();
	}

	Result GrafComputePipelineVulkan::Deinitialize()
	{
		if (this->vkPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipelineLayout, ur_null);
			this->vkPipelineLayout = VK_NULL_HANDLE;
		}
		if (this->vkPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipeline, ur_null);
			this->vkPipeline = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafComputePipelineVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafComputePipeline::Initialize(grafDevice, initParams);

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafComputePipelineVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkResult vkRes = VK_SUCCESS;

		// shader stages

		VkPipelineShaderStageCreateInfo vkShaderStageInfo = {};
		GrafShader* grafShader = initParams.ShaderStage;
		vkShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vkShaderStageInfo.flags = 0;
		vkShaderStageInfo.stage = GrafUtilsVulkan::GrafToVkShaderStage(grafShader->GetShaderType());
		vkShaderStageInfo.module = static_cast<GrafShaderVulkan*>(grafShader)->GetVkShaderModule();
		vkShaderStageInfo.pName = grafShader->GetEntryPoint().c_str();

		// pipeline layout

		std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts(initParams.DescriptorTableLayoutCount);
		for (ur_uint ilayout = 0; ilayout < initParams.DescriptorTableLayoutCount; ++ilayout)
		{
			vkDescriptorSetLayouts[ilayout] = static_cast<GrafDescriptorTableLayoutVulkan*>(initParams.DescriptorTableLayouts[ilayout])->GetVkDescriptorSetLayout();
		}

		VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
		vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkPipelineLayoutInfo.flags = 0;
		vkPipelineLayoutInfo.setLayoutCount = (ur_uint32)vkDescriptorSetLayouts.size();
		vkPipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
		vkPipelineLayoutInfo.pushConstantRangeCount = 0;
		vkPipelineLayoutInfo.pPushConstantRanges = ur_null;

		vkRes = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutInfo, ur_null, &this->vkPipelineLayout);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafComputePipelineVulkan: vkCreatePipelineLayout failed with VkResult = ") + VkResultToString(vkRes));
		}

		// create pipeline object

		VkComputePipelineCreateInfo vkPipelineInfo = {};
		vkPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		vkPipelineInfo.pNext = ur_null;
		vkPipelineInfo.flags = 0;
		vkPipelineInfo.stage = vkShaderStageInfo;
		vkPipelineInfo.layout = this->vkPipelineLayout;
		vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		vkPipelineInfo.basePipelineIndex = -1;

		vkRes = vkCreateComputePipelines(vkDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, ur_null, &this->vkPipeline);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafComputePipelineVulkan: vkCreateComputePipelines failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRayTracingPipelineVulkan::GrafRayTracingPipelineVulkan(GrafSystem &grafSystem) :
		GrafRayTracingPipeline(grafSystem)
	{
		this->vkPipeline = VK_NULL_HANDLE;
		this->vkPipelineLayout = VK_NULL_HANDLE;
	}

	GrafRayTracingPipelineVulkan::~GrafRayTracingPipelineVulkan()
	{
		this->Deinitialize();
	}

	Result GrafRayTracingPipelineVulkan::Deinitialize()
	{
		if (this->vkPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipelineLayout, ur_null);
			this->vkPipelineLayout = VK_NULL_HANDLE;
		}
		if (this->vkPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipeline, ur_null);
			this->vkPipeline = VK_NULL_HANDLE;
		}

		return Result(Success);
	}

	Result GrafRayTracingPipelineVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafRayTracingPipeline::Initialize(grafDevice, initParams);
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)

		// validate logical device 

		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafRayTracingPipelineVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkResult vkRes = VK_SUCCESS;

		// shader stages

		std::vector<VkPipelineShaderStageCreateInfo> vkShaderStagesInfo(initParams.ShaderStageCount);
		for (ur_uint istage = 0; istage < initParams.ShaderStageCount; ++istage)
		{
			GrafShader* grafShader = initParams.ShaderStages[istage];
			VkPipelineShaderStageCreateInfo& vkShaderStageInfo = vkShaderStagesInfo[istage];
			vkShaderStageInfo = {};
			vkShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vkShaderStageInfo.flags = 0;
			vkShaderStageInfo.stage = GrafUtilsVulkan::GrafToVkShaderStage(grafShader->GetShaderType());
			vkShaderStageInfo.module = static_cast<GrafShaderVulkan*>(grafShader)->GetVkShaderModule();
			vkShaderStageInfo.pName = grafShader->GetEntryPoint().c_str();
		}

		// shader groups

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> vkShaderGroupsInfo(initParams.ShaderGroupCount);
		for (ur_uint igroup = 0; igroup < initParams.ShaderGroupCount; ++igroup)
		{
			GrafRayTracingShaderGroupDesc& grafShaderGroupDesc = initParams.ShaderGroups[igroup];
			VkRayTracingShaderGroupCreateInfoKHR& vkShaderGroupInfo = vkShaderGroupsInfo[igroup];
			vkShaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			vkShaderGroupInfo.pNext = ur_null;
			vkShaderGroupInfo.type = GrafUtilsVulkan::GrafToVkRayTracingShaderGroupType(grafShaderGroupDesc.Type);
			vkShaderGroupInfo.generalShader = (ur_uint32)grafShaderGroupDesc.GeneralShaderIdx;
			vkShaderGroupInfo.closestHitShader = (ur_uint32)grafShaderGroupDesc.ClosestHitShaderIdx;
			vkShaderGroupInfo.anyHitShader = (ur_uint32)grafShaderGroupDesc.AnyHitShaderIdx;
			vkShaderGroupInfo.intersectionShader = (ur_uint32)grafShaderGroupDesc.IntersectionShaderIdx;
			vkShaderGroupInfo.pShaderGroupCaptureReplayHandle = ur_null;
		}

		// pipeline layout

		std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts(initParams.DescriptorTableLayoutCount);
		for (ur_uint ilayout = 0; ilayout < initParams.DescriptorTableLayoutCount; ++ilayout)
		{
			vkDescriptorSetLayouts[ilayout] = static_cast<GrafDescriptorTableLayoutVulkan*>(initParams.DescriptorTableLayouts[ilayout])->GetVkDescriptorSetLayout();
		}

		VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
		vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		vkPipelineLayoutInfo.flags = 0;
		vkPipelineLayoutInfo.setLayoutCount = (ur_uint32)vkDescriptorSetLayouts.size();
		vkPipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
		vkPipelineLayoutInfo.pushConstantRangeCount = 0;
		vkPipelineLayoutInfo.pPushConstantRanges = ur_null;

		vkRes = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutInfo, ur_null, &this->vkPipelineLayout);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRayTracingPipelineVulkan: vkCreatePipelineLayout failed with VkResult = ") + VkResultToString(vkRes));
		}

		// create pipeline object

		VkRayTracingPipelineCreateInfoKHR vkPipelineInfo = {};
		vkPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		vkPipelineInfo.pNext = ur_null;
		vkPipelineInfo.flags = 0;
		vkPipelineInfo.stageCount = (ur_uint32)vkShaderStagesInfo.size();
		vkPipelineInfo.pStages = vkShaderStagesInfo.data();
		vkPipelineInfo.groupCount = (ur_uint32)vkShaderGroupsInfo.size();
		vkPipelineInfo.pGroups = vkShaderGroupsInfo.data();
		vkPipelineInfo.maxPipelineRayRecursionDepth = (ur_uint32)initParams.MaxRecursionDepth;
		vkPipelineInfo.pLibraryInfo = ur_null;
		vkPipelineInfo.pLibraryInterface = ur_null;
		vkPipelineInfo.pDynamicState = ur_null;
		vkPipelineInfo.layout = this->vkPipelineLayout;
		vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		vkPipelineInfo.basePipelineIndex = -1;

		vkRes = vkCreateRayTracingPipelinesKHR(vkDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &vkPipelineInfo, ur_null, &this->vkPipeline);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafRayTracingPipelineVulkan: vkCreateRayTracingPipelinesKHR failed with VkResult = ") + VkResultToString(vkRes));
		}

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	Result GrafRayTracingPipelineVulkan::GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)

		VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();
		
		VkResult vkRes = vkGetRayTracingShaderGroupHandlesKHR(vkDevice, this->vkPipeline, (ur_uint32)firstGroup, (ur_uint32)groupCount, dstBufferSize, dstBufferPtr);
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafRayTracingPipelineVulkan: vkCreateRayTracingPipelinesKHR failed with VkResult = ") + VkResultToString(vkRes));

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafAccelerationStructureVulkan::GrafAccelerationStructureVulkan(GrafSystem &grafSystem) :
		GrafAccelerationStructure(grafSystem)
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		this->vkAccelerationStructure = VK_NULL_HANDLE;
	#endif
		this->vmaAllocation = VK_NULL_HANDLE;
		this->vmaAllocationInfo = {};
	}

	GrafAccelerationStructureVulkan::~GrafAccelerationStructureVulkan()
	{
		this->Deinitialize();
	}

	Result GrafAccelerationStructureVulkan::Deinitialize()
	{
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		if (this->vkAccelerationStructure != VK_NULL_HANDLE)
		{
			vkDestroyAccelerationStructureKHR(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkAccelerationStructure, ur_null);
			this->vkAccelerationStructure = ur_null;
		}
	#endif
		if (this->vmaAllocation != VK_NULL_HANDLE)
		{
			vmaFreeMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVmaAllocator(), this->vmaAllocation);
			this->vmaAllocation = VK_NULL_HANDLE;
			this->vmaAllocationInfo = {};
		}
		this->grafScratchBuffer.reset();
		this->grafStorageBuffer.reset();
		this->structureType = GrafAccelerationStructureType::Undefined;
		this->structureBuildFlags = GrafAccelerationStructureBuildFlags(0);
		this->structureDeviceAddress = 0;

		return Result(Success);
	}

	Result GrafAccelerationStructureVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		this->Deinitialize();

		GrafAccelerationStructure::Initialize(grafDevice, initParams);
	#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)

		// validate logical device 

		GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
		if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
		{
			return ResultError(InvalidArgs, std::string("GrafPipelineVulkan: failed to initialize, invalid GrafDevice"));
		}
		VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
		VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(grafDeviceVulkan->GetDeviceId());
		VkResult vkRes = VK_SUCCESS;

		// gather geometry info

		std::vector<VkAccelerationStructureGeometryKHR> vkGeometryInfoArray(initParams.GeometryCount);
		std::vector<ur_uint32> geometryCount(initParams.GeometryCount);
		for (ur_uint32 igeom = 0; igeom < initParams.GeometryCount; ++igeom)
		{
			GrafAccelerationStructureGeometryDesc& geometryDesc = initParams.Geometry[igeom];
			VkAccelerationStructureGeometryKHR& vkGeometryInfo = vkGeometryInfoArray[igeom];
			vkGeometryInfo = {};
			vkGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vkGeometryInfo.geometryType = GrafUtilsVulkan::GrafToVkAccelerationStructureGeometryType(geometryDesc.GeometryType);
			vkGeometryInfo.flags = 0;
			if (GrafAccelerationStructureGeometryType::Instances == geometryDesc.GeometryType)
			{
				VkAccelerationStructureGeometryInstancesDataKHR& vkGeometryInstances = vkGeometryInfo.geometry.instances;
				vkGeometryInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
				vkGeometryInstances.arrayOfPointers = false;
				vkGeometryInstances.data = {};
			}
			else if (GrafAccelerationStructureGeometryType::Triangles == geometryDesc.GeometryType)
			{
				VkAccelerationStructureGeometryTrianglesDataKHR& vkGeometryTriangles = vkGeometryInfo.geometry.triangles;
				vkGeometryTriangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
				vkGeometryTriangles.vertexFormat = GrafUtilsVulkan::GrafToVkFormat(geometryDesc.VertexFormat);
				vkGeometryTriangles.vertexData = {};
				vkGeometryTriangles.vertexStride = geometryDesc.VertexStride;
				vkGeometryTriangles.maxVertex = geometryDesc.VertexCountMax;
				vkGeometryTriangles.indexType = GrafUtilsVulkan::GrafToVkIndexType(geometryDesc.IndexType);
				vkGeometryTriangles.indexData = {};
				vkGeometryTriangles.transformData = {};
			}
			else if (GrafAccelerationStructureGeometryType::AABBs == geometryDesc.GeometryType)
			{
				VkAccelerationStructureGeometryAabbsDataKHR& vkGeometryAabbs = vkGeometryInfo.geometry.aabbs;
				vkGeometryAabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
				vkGeometryAabbs.data = {};
				vkGeometryAabbs.stride = 0;
			}
			geometryCount[igeom] = geometryDesc.PrimitiveCountMax;
		}

		// calculate build sizes

		VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo = {};
		vkBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		vkBuildInfo.type = GrafUtilsVulkan::GrafToVkAccelerationStructureType(initParams.StructureType);
		vkBuildInfo.flags = GrafUtilsVulkan::GrafToVkAccelerationStructureBuildFlags(initParams.BuildFlags);
		vkBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		vkBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		vkBuildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
		vkBuildInfo.geometryCount = initParams.GeometryCount;
		vkBuildInfo.pGeometries = vkGeometryInfoArray.data();
		vkBuildInfo.ppGeometries = ur_null;
		vkBuildInfo.scratchData = {};

		VkAccelerationStructureBuildSizesInfoKHR vkBuildSizeInfo = {};
		vkBuildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vkBuildInfo, geometryCount.data(), &vkBuildSizeInfo);

		// storage buffer for acceleration structure

		Result res = grafSystemVulkan.CreateBuffer(this->grafStorageBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams storageBufferParams = {};
			storageBufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::AccelerationStructure) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress) | ur_uint(GrafBufferUsageFlag::StorageBuffer);
			storageBufferParams.BufferDesc.MemoryType = GrafDeviceMemoryFlags(GrafDeviceMemoryFlag::GpuLocal);
			storageBufferParams.BufferDesc.SizeInBytes = vkBuildSizeInfo.accelerationStructureSize;

			res = this->grafStorageBuffer->Initialize(grafDevice, storageBufferParams);
		}
		if (Failed(res))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafAccelerationStructureVulkan: failed to create storage buffer");
		}

		// scratch buffer

		res = grafSystemVulkan.CreateBuffer(this->grafScratchBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams scrathBufferParams = {};
			scrathBufferParams.BufferDesc.Usage = GrafBufferUsageFlags(ur_uint(GrafBufferUsageFlag::AccelerationStructure) | ur_uint(GrafBufferUsageFlag::ShaderDeviceAddress));
			scrathBufferParams.BufferDesc.MemoryType = GrafDeviceMemoryFlags(GrafDeviceMemoryFlag::GpuLocal);
			scrathBufferParams.BufferDesc.SizeInBytes = std::max(vkBuildSizeInfo.buildScratchSize, vkBuildSizeInfo.updateScratchSize);

			res = this->grafScratchBuffer->Initialize(grafDeviceVulkan, scrathBufferParams);
		}
		if (Failed(res))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafAccelerationStructureVulkan: failed to create scratch buffer");
		}

		// acceleration structure

		VkAccelerationStructureCreateInfoKHR vkAccelStructCreateInfo = {};
		vkAccelStructCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		vkAccelStructCreateInfo.pNext = ur_null;
		vkAccelStructCreateInfo.createFlags = 0;
		vkAccelStructCreateInfo.buffer = static_cast<GrafBufferVulkan*>(this->grafStorageBuffer.get())->GetVkBuffer();
		vkAccelStructCreateInfo.offset = 0;
		vkAccelStructCreateInfo.size = vkBuildSizeInfo.accelerationStructureSize;
		vkAccelStructCreateInfo.type = GrafUtilsVulkan::GrafToVkAccelerationStructureType(initParams.StructureType);
		vkAccelStructCreateInfo.deviceAddress = VkDeviceAddress(0);

		vkRes = vkCreateAccelerationStructureKHR(vkDevice, &vkAccelStructCreateInfo, ur_null, &this->vkAccelerationStructure);
		if (vkRes != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafAccelerationStructureVulkan: vkCreateAccelerationStructureKHR failed with VkResult = ") + VkResultToString(vkRes));
		}

		// retrieve device address

		VkAccelerationStructureDeviceAddressInfoKHR vkDeviceAddresInfo = {};
		vkDeviceAddresInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		vkDeviceAddresInfo.pNext = ur_null;
		vkDeviceAddresInfo.accelerationStructure = this->vkAccelerationStructure;

		this->structureDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vkDevice, &vkDeviceAddresInfo);

		return Result(Success);
	#else
		return Result(NotImplemented);
	#endif
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VkImageUsageFlags GrafUtilsVulkan::GrafToVkImageUsage(GrafImageUsageFlags usage)
	{
		VkImageUsageFlags vkImageUsage = 0;
		if (usage & (ur_uint)GrafImageUsageFlag::ColorRenderTarget)
			vkImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (usage & (ur_uint)GrafImageUsageFlag::DepthStencilRenderTarget)
			vkImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (usage & (ur_uint)GrafImageUsageFlag::TransferSrc)
			vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (usage & (ur_uint)GrafImageUsageFlag::TransferDst)
			vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (usage & (ur_uint)GrafImageUsageFlag::ShaderRead)
			vkImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (usage & (ur_uint)GrafImageUsageFlag::ShaderReadWrite)
			vkImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
		return vkImageUsage;
	}

	GrafImageUsageFlags GrafUtilsVulkan::VkToGrafImageUsage(VkImageUsageFlags usage)
	{
		GrafImageUsageFlags grafUsage = (ur_uint)GrafImageUsageFlag::Undefined;
		if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) || (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
			grafUsage |= (ur_uint)GrafImageUsageFlag::ColorRenderTarget;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			grafUsage |= (ur_uint)GrafImageUsageFlag::DepthStencilRenderTarget;
		if (VK_IMAGE_USAGE_TRANSFER_SRC_BIT & usage)
			grafUsage |= (ur_uint)GrafImageUsageFlag::TransferSrc;
		if (VK_IMAGE_USAGE_TRANSFER_DST_BIT & usage)
			grafUsage |= (ur_uint)GrafImageUsageFlag::TransferDst;
		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
			grafUsage |= (ur_uint)GrafImageUsageFlag::ShaderRead;
		if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
			grafUsage |= (ur_uint)GrafImageUsageFlag::ShaderReadWrite;
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

	VkImageAspectFlags GrafUtilsVulkan::GrafToVkImageUsageAspect(GrafImageUsageFlags usage)
	{
		VkImageAspectFlags vkImageAspectFlags = 0;
		if (usage & (ur_uint)GrafImageUsageFlag::DepthStencilRenderTarget)
			vkImageAspectFlags |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
		else
			vkImageAspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
		return vkImageAspectFlags;
	}

	VkImageLayout GrafUtilsVulkan::GrafToVkImageLayout(GrafImageState imageState)
	{
		VkImageLayout vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		switch (imageState)
		{
		case GrafImageState::Common: vkImageLayout = VK_IMAGE_LAYOUT_GENERAL; break;
		case GrafImageState::TransferSrc: vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
		case GrafImageState::TransferDst: vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case GrafImageState::Present: vkImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; break;
		case GrafImageState::ColorClear: vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case GrafImageState::ColorWrite: vkImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
		case GrafImageState::DepthStencilClear: vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case GrafImageState::DepthStencilWrite: vkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
		case GrafImageState::DepthStencilRead: vkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; break;
		case GrafImageState::ShaderRead: vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case GrafImageState::ShaderReadWrite: vkImageLayout = VK_IMAGE_LAYOUT_GENERAL; break;
		case GrafImageState::ComputeRead: vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case GrafImageState::ComputeReadWrite: vkImageLayout = VK_IMAGE_LAYOUT_GENERAL; break;
		case GrafImageState::RayTracingRead: vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case GrafImageState::RayTracingReadWrite: vkImageLayout = VK_IMAGE_LAYOUT_GENERAL; break;
		};
		return vkImageLayout;
	}

	VkBufferUsageFlags GrafUtilsVulkan::GrafToVkBufferUsage(GrafBufferUsageFlags usage)
	{
		VkBufferUsageFlags vkUsage = 0;
		if (usage & (ur_uint)GrafBufferUsageFlag::TransferSrc)
			vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (usage & (ur_uint)GrafBufferUsageFlag::TransferDst)
			vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (usage & (ur_uint)GrafBufferUsageFlag::VertexBuffer)
			vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (usage & (ur_uint)GrafBufferUsageFlag::IndexBuffer)
			vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (usage & (ur_uint)GrafBufferUsageFlag::ConstantBuffer)
			vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (usage & (ur_uint)GrafBufferUsageFlag::StorageBuffer)
			vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		if (usage & (ur_uint)GrafBufferUsageFlag::ShaderDeviceAddress)
			vkUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		if (usage & (ur_uint)GrafBufferUsageFlag::AccelerationStructure)
			vkUsage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
		#endif
		return vkUsage;
	}

	VkMemoryPropertyFlags GrafUtilsVulkan::GrafToVkMemoryProperties(GrafDeviceMemoryFlags memoryType)
	{
		VkMemoryPropertyFlags vkMemoryProperties = 0;
		if (memoryType & (ur_uint)GrafDeviceMemoryFlag::GpuLocal)
			vkMemoryProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		if (memoryType & (ur_uint)GrafDeviceMemoryFlag::CpuVisible)
			vkMemoryProperties |= (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		return vkMemoryProperties;
	}

	VkShaderStageFlags GrafUtilsVulkan::GrafToVkShaderStage(GrafShaderStageFlags shaderStages)
	{
		VkShaderStageFlags vkShaderStage = VkShaderStageFlagBits(0);
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Vertex)
			vkShaderStage |= VK_SHADER_STAGE_VERTEX_BIT;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Pixel)
			vkShaderStage |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Compute)
			vkShaderStage |= VK_SHADER_STAGE_COMPUTE_BIT;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		if (shaderStages & (ur_uint)GrafShaderStageFlag::RayGen)
			vkShaderStage |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::AnyHit)
			vkShaderStage |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::ClosestHit)
			vkShaderStage |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Miss)
			vkShaderStage |= VK_SHADER_STAGE_MISS_BIT_KHR;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Intersection)
			vkShaderStage |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Callable)
			vkShaderStage |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Task)
			vkShaderStage |= VK_SHADER_STAGE_TASK_BIT_NV;
		if (shaderStages & (ur_uint)GrafShaderStageFlag::Mesh)
			vkShaderStage |= VK_SHADER_STAGE_MESH_BIT_NV;
		#endif
		return vkShaderStage;
	}
	
	VkShaderStageFlagBits GrafUtilsVulkan::GrafToVkShaderStage(GrafShaderType shaderType)
	{
		return (VkShaderStageFlagBits)GrafUtilsVulkan::GrafToVkShaderStage(GrafShaderStageFlags(shaderType));
	}


	VkDescriptorType GrafUtilsVulkan::GrafToVkDescriptorType(GrafDescriptorType descriptorType)
	{
		VkDescriptorType vkDescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		switch (descriptorType)
		{
		case GrafDescriptorType::ConstantBuffer: vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
		case GrafDescriptorType::Sampler: vkDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; break;
		case GrafDescriptorType::Texture: vkDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
		case GrafDescriptorType::Buffer: vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		case GrafDescriptorType::RWTexture: vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
		case GrafDescriptorType::RWBuffer: vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		case GrafDescriptorType::TextureDynamicArray: vkDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
		case GrafDescriptorType::BufferDynamicArray: vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		case GrafDescriptorType::AccelerationStructure: vkDescriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; break;
		#endif
		};
		return vkDescriptorType;
	}

	static const ur_uint32 GrafToVkDescriptorBindingOffsetLUT[(ur_uint)GrafDescriptorType::Count] = {
		VulkanBindingOffsetConstantBuffer,
		VulkanBindingOffsetSampler,
		VulkanBindingOffsetTexture,
		VulkanBindingOffsetBuffer,
		VulkanBindingOffsetRWTexture,
		VulkanBindingOffsetRWBuffer,
		VulkanBindingOffsetAccelerationStructure,
		VulkanBindingOffsetTextureDynamicArray,
		VulkanBindingOffsetBufferDynamicArray
	};

	ur_uint32 GrafUtilsVulkan::GrafToVkDescriptorBindingOffset(GrafDescriptorType descriptorType)
	{
		return GrafToVkDescriptorBindingOffsetLUT[(ur_uint)descriptorType];
	}

	VkPrimitiveTopology GrafUtilsVulkan::GrafToVkPrimitiveTopology(GrafPrimitiveTopology topology)
	{
		VkPrimitiveTopology vkTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
		switch (topology)
		{
		case GrafPrimitiveTopology::PointList: vkTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
		case GrafPrimitiveTopology::LineList: vkTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
		case GrafPrimitiveTopology::LineStrip: vkTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
		case GrafPrimitiveTopology::TriangleList: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
		case GrafPrimitiveTopology::TriangleStrip: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
		case GrafPrimitiveTopology::TriangleFan: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
		};
		return vkTopology;
	}

	VkFrontFace GrafUtilsVulkan::GrafToVkFrontFace(GrafFrontFaceOrder frontFaceOrder)
	{
		VkFrontFace vkFrontFace = VK_FRONT_FACE_MAX_ENUM;
		switch (frontFaceOrder)
		{
		case GrafFrontFaceOrder::CounterClockwise: vkFrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
		case GrafFrontFaceOrder::Clockwise: vkFrontFace = VK_FRONT_FACE_CLOCKWISE; break;
		};
		return vkFrontFace;
	}

	VkCullModeFlags GrafUtilsVulkan::GrafToVkCullModeFlags(GrafCullMode cullMode)
	{
		VkCullModeFlags vkCullModeFlags = VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
		switch (cullMode)
		{
		case GrafCullMode::None: vkCullModeFlags = VK_CULL_MODE_NONE; break;
		case GrafCullMode::Front: vkCullModeFlags = VK_CULL_MODE_FRONT_BIT; break;
		case GrafCullMode::Back: vkCullModeFlags = VK_CULL_MODE_BACK_BIT; break;
		};
		return vkCullModeFlags;
	}

	VkCompareOp GrafUtilsVulkan::GrafToVkCompareOp(GrafCompareOp compareOp)
	{
		VkCompareOp vkCompareOp = VK_COMPARE_OP_MAX_ENUM;
		switch (compareOp)
		{
		case GrafCompareOp::Never: vkCompareOp = VK_COMPARE_OP_NEVER; break;
		case GrafCompareOp::Less: vkCompareOp = VK_COMPARE_OP_LESS; break;
		case GrafCompareOp::Equal: vkCompareOp = VK_COMPARE_OP_EQUAL; break;
		case GrafCompareOp::LessOrEqual: vkCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; break;
		case GrafCompareOp::Greater: vkCompareOp = VK_COMPARE_OP_GREATER; break;
		case GrafCompareOp::NotEqual: vkCompareOp = VK_COMPARE_OP_NOT_EQUAL; break;
		case GrafCompareOp::GreaterOrEqual: vkCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
		case GrafCompareOp::Always: vkCompareOp = VK_COMPARE_OP_ALWAYS; break;
		};
		return vkCompareOp;
	}

	VkStencilOp GrafUtilsVulkan::GrafToVkStencilOp(GrafStencilOp stencilOp)
	{
		VkStencilOp vkStencilOp = VK_STENCIL_OP_MAX_ENUM;
		switch (stencilOp)
		{
		case GrafStencilOp::Keep: vkStencilOp = VK_STENCIL_OP_KEEP; break;
		case GrafStencilOp::Zero: vkStencilOp = VK_STENCIL_OP_ZERO; break;
		case GrafStencilOp::Replace: vkStencilOp = VK_STENCIL_OP_REPLACE; break;
		};
		return vkStencilOp;
	}

	VkBlendOp GrafUtilsVulkan::GrafToVkBlendOp(GrafBlendOp blendOp)
	{
		VkBlendOp vkBlendOp = VK_BLEND_OP_MAX_ENUM;
		switch (blendOp)
		{
		case GrafBlendOp::Add: vkBlendOp = VK_BLEND_OP_ADD; break;
		case GrafBlendOp::Subtract: vkBlendOp = VK_BLEND_OP_SUBTRACT; break;
		case GrafBlendOp::ReverseSubtract: vkBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT; break;
		case GrafBlendOp::Min: vkBlendOp = VK_BLEND_OP_MIN; break;
		case GrafBlendOp::Max: vkBlendOp = VK_BLEND_OP_MAX; break;
		};
		return vkBlendOp;
	}

	VkBlendFactor GrafUtilsVulkan::GrafToVkBlendFactor(GrafBlendFactor blendFactor)
	{
		VkBlendFactor vkBlendFactor = VK_BLEND_FACTOR_MAX_ENUM;
		switch (blendFactor)
		{
		case GrafBlendFactor::Zero: vkBlendFactor = VK_BLEND_FACTOR_ZERO; break;
		case GrafBlendFactor::One: vkBlendFactor = VK_BLEND_FACTOR_ONE; break;
		case GrafBlendFactor::SrcColor: vkBlendFactor = VK_BLEND_FACTOR_SRC_COLOR; break;
		case GrafBlendFactor::InvSrcColor: vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
		case GrafBlendFactor::DstColor: vkBlendFactor = VK_BLEND_FACTOR_DST_COLOR; break;
		case GrafBlendFactor::InvDstColor: vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
		case GrafBlendFactor::SrcAlpha: vkBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; break;
		case GrafBlendFactor::InvSrcAlpha: vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
		case GrafBlendFactor::DstAlpha: vkBlendFactor = VK_BLEND_FACTOR_DST_ALPHA; break;
		case GrafBlendFactor::InvDstAlpha: vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
		};
		return vkBlendFactor;
	}

	VkFilter GrafUtilsVulkan::GrafToVkFilter(GrafFilterType filter)
	{
		VkFilter vkFilter = VK_FILTER_MAX_ENUM;
		switch (filter)
		{
		case GrafFilterType::Nearest: vkFilter = VK_FILTER_NEAREST; break;
		case GrafFilterType::Linear: vkFilter = VK_FILTER_LINEAR; break;
		};
		return vkFilter;
	}

	VkSamplerAddressMode GrafUtilsVulkan::GrafToVkAddressMode(GrafAddressMode address)
	{
		VkSamplerAddressMode vkAddressMode = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
		switch (address)
		{
		case GrafAddressMode::Wrap: vkAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
		case GrafAddressMode::Mirror: vkAddressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
		case GrafAddressMode::Clamp: vkAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
		case GrafAddressMode::Border: vkAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; break;
		case GrafAddressMode::MirrorOnce: vkAddressMode = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE; break;
		};
		return vkAddressMode;
	}
	
	VkIndexType GrafUtilsVulkan::GrafToVkIndexType(GrafIndexType indexType)
	{
		VkIndexType vkIndexType = VK_INDEX_TYPE_MAX_ENUM;
		switch (indexType)
		{
		case GrafIndexType::UINT16: vkIndexType = VK_INDEX_TYPE_UINT16; break;
		case GrafIndexType::UINT32: vkIndexType = VK_INDEX_TYPE_UINT32; break;
		#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)
		case GrafIndexType::None: vkIndexType = VK_INDEX_TYPE_NONE_KHR; break;
		#endif
		};
		return vkIndexType;
	}

	VkAttachmentLoadOp GrafUtilsVulkan::GrafToVkLoadOp(GrafRenderPassDataOp dataOp)
	{
		VkAttachmentLoadOp vkOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		switch (dataOp)
		{
		case GrafRenderPassDataOp::DontCare: vkOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; break;
		case GrafRenderPassDataOp::Clear: vkOp = VK_ATTACHMENT_LOAD_OP_CLEAR; break;
		case GrafRenderPassDataOp::Load: vkOp = VK_ATTACHMENT_LOAD_OP_LOAD; break;
		};
		return vkOp;
	}

	VkAttachmentStoreOp GrafUtilsVulkan::GrafToVkStoreOp(GrafRenderPassDataOp dataOp)
	{
		VkAttachmentStoreOp vkOp = VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		switch (dataOp)
		{
		case GrafRenderPassDataOp::DontCare: vkOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; break;
		case GrafRenderPassDataOp::Store: vkOp = VK_ATTACHMENT_STORE_OP_STORE; break;
		};
		return vkOp;
	}

#if (UR_GRAF_VULKAN_RAY_TRACING_KHR)

	VkRayTracingShaderGroupTypeKHR GrafUtilsVulkan::GrafToVkRayTracingShaderGroupType(GrafRayTracingShaderGroupType shaderGroupType)
	{
		VkRayTracingShaderGroupTypeKHR vkShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_MAX_ENUM_KHR;
		switch (shaderGroupType)
		{
		case GrafRayTracingShaderGroupType::General: vkShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR; break;
		case GrafRayTracingShaderGroupType::TrianglesHit: vkShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR; break;
		case GrafRayTracingShaderGroupType::ProceduralHit: vkShaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR; break;
		};
		return vkShaderGroupType;
	}

	VkGeometryTypeKHR GrafUtilsVulkan::GrafToVkAccelerationStructureGeometryType(GrafAccelerationStructureGeometryType geometryType)
	{
		VkGeometryTypeKHR vkGeometryType = VK_GEOMETRY_TYPE_MAX_ENUM_KHR;
		switch (geometryType)
		{
		case GrafAccelerationStructureGeometryType::Triangles: vkGeometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; break;
		case GrafAccelerationStructureGeometryType::AABBs: vkGeometryType = VK_GEOMETRY_TYPE_AABBS_KHR; break;
		case GrafAccelerationStructureGeometryType::Instances: vkGeometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR; break;
		};
		return vkGeometryType;
	}

	VkGeometryFlagsKHR GrafUtilsVulkan::GrafToVkAccelerationStructureGeometryFlags(GrafAccelerationStructureGeometryFlags geometryFlags)
	{
		VkGeometryFlagsKHR vkGeometryFlags = VkGeometryFlagsKHR(0);
		if (ur_uint(GrafAccelerationStructureGeometryFlag::Opaque) & geometryFlags)
			vkGeometryFlags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
		if (ur_uint(GrafAccelerationStructureGeometryFlag::NoDuplicateAnyHit) & geometryFlags)
			vkGeometryFlags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
		return vkGeometryFlags;
	}

	VkAccelerationStructureTypeKHR GrafUtilsVulkan::GrafToVkAccelerationStructureType(GrafAccelerationStructureType structureType)
	{
		VkAccelerationStructureTypeKHR vkAccelStructType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
		switch (structureType)
		{
		case GrafAccelerationStructureType::TopLevel: vkAccelStructType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR; break;
		case GrafAccelerationStructureType::BottomLevel: vkAccelStructType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR; break;
		};
		return vkAccelStructType;
	}

	VkBuildAccelerationStructureFlagsKHR GrafUtilsVulkan::GrafToVkAccelerationStructureBuildFlags(GrafAccelerationStructureBuildFlags buildFlags)
	{
		VkBuildAccelerationStructureFlagsKHR vkAccelStructBuildFlags = VkBuildAccelerationStructureFlagsKHR(0);
		if (ur_uint(GrafAccelerationStructureBuildFlag::AllowUpdate) & buildFlags)
			vkAccelStructBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		if (ur_uint(GrafAccelerationStructureBuildFlag::AllowCompaction) & buildFlags)
			vkAccelStructBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		if (ur_uint(GrafAccelerationStructureBuildFlag::PreferFastTrace) & buildFlags)
			vkAccelStructBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		if (ur_uint(GrafAccelerationStructureBuildFlag::PreferFastBuild) & buildFlags)
			vkAccelStructBuildFlags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		return vkAccelStructBuildFlags;
	}

#endif

	static const VkFormat GrafToVkFormatLUT[ur_uint(GrafFormat::Count)] = {
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
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
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R64_UINT,
		VK_FORMAT_R64_SINT,
		VK_FORMAT_R64_SFLOAT,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_BC1_RGB_UNORM_BLOCK,
		VK_FORMAT_BC1_RGB_SRGB_BLOCK,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC3_SRGB_BLOCK
	};
	VkFormat GrafUtilsVulkan::GrafToVkFormat(GrafFormat grafFormat)
	{
		return GrafToVkFormatLUT[ur_uint(grafFormat)];
	}

	static const GrafFormat VkToGrafFormatLUT[] = {
		GrafFormat::Undefined,
		GrafFormat::Unsupported,			// VK_FORMAT_R4G4_UNORM_PACK8 = 1,
		GrafFormat::Unsupported,			// VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
		GrafFormat::Unsupported,			// VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
		GrafFormat::Unsupported,			// VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
		GrafFormat::Unsupported,			// VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
		GrafFormat::Unsupported,			// VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
		GrafFormat::Unsupported,			// VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
		GrafFormat::Unsupported,			// VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
		GrafFormat::R8_UNORM,				// VK_FORMAT_R8_UNORM = 9,
		GrafFormat::R8_SNORM,				// VK_FORMAT_R8_SNORM = 10,
		GrafFormat::Unsupported,			// VK_FORMAT_R8_USCALED = 11,
		GrafFormat::Unsupported,			// VK_FORMAT_R8_SSCALED = 12,
		GrafFormat::R8_UINT,				// VK_FORMAT_R8_UINT = 13,
		GrafFormat::R8_SINT,				// VK_FORMAT_R8_SINT = 14,
		GrafFormat::Unsupported,			// VK_FORMAT_R8_SRGB = 15,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8_UNORM = 16,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SNORM = 17,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8_USCALED = 18,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SSCALED = 19,
		GrafFormat::R8G8_UINT,				// VK_FORMAT_R8G8_UINT = 20,
		GrafFormat::R8G8_SINT,				// VK_FORMAT_R8G8_SINT = 21,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SRGB = 22,
		GrafFormat::R8G8B8_UNORM,			// VK_FORMAT_R8G8B8_UNORM = 23,
		GrafFormat::R8G8B8_SNORM,			// VK_FORMAT_R8G8B8_SNORM = 24,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8_USCALED = 25,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8_SSCALED = 26,
		GrafFormat::R8G8B8_UINT,			// VK_FORMAT_R8G8B8_UINT = 27,
		GrafFormat::R8G8B8_SINT,			// VK_FORMAT_R8G8B8_SINT = 28,
		GrafFormat::R8G8B8_SRGB,			// VK_FORMAT_R8G8B8_SRGB = 29,
		GrafFormat::B8G8R8_UNORM,			// VK_FORMAT_B8G8R8_UNORM = 30,
		GrafFormat::B8G8R8_SNORM,			// VK_FORMAT_B8G8R8_SNORM = 31,
		GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8_USCALED = 32,
		GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8_SSCALED = 33,
		GrafFormat::B8G8R8_UINT,			// VK_FORMAT_B8G8R8_UINT = 34,
		GrafFormat::B8G8R8_SINT,			// VK_FORMAT_B8G8R8_SINT = 35,
		GrafFormat::B8G8R8_SRGB,			// VK_FORMAT_B8G8R8_SRGB = 36,
		GrafFormat::R8G8B8A8_UNORM,			// VK_FORMAT_R8G8B8A8_UNORM = 37,
		GrafFormat::R8G8B8A8_SNORM,			// VK_FORMAT_R8G8B8A8_SNORM = 38,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8A8_USCALED = 39,
		GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8A8_SSCALED = 40,
		GrafFormat::R8G8B8A8_UINT,			// VK_FORMAT_R8G8B8A8_UINT = 41,
		GrafFormat::R8G8B8A8_SINT,			// VK_FORMAT_R8G8B8A8_SINT = 42,
		GrafFormat::R8G8B8A8_SRGB,			// VK_FORMAT_R8G8B8A8_SRGB = 43,
		GrafFormat::B8G8R8A8_UNORM,			// VK_FORMAT_B8G8R8A8_UNORM = 44,
		GrafFormat::B8G8R8A8_SNORM,			// VK_FORMAT_B8G8R8A8_SNORM = 45,
		GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8A8_USCALED = 46,
		GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8A8_SSCALED = 47,
		GrafFormat::B8G8R8A8_UINT,			// VK_FORMAT_B8G8R8A8_UINT = 48,
		GrafFormat::B8G8R8A8_SINT,			// VK_FORMAT_B8G8R8A8_SINT = 49,
		GrafFormat::B8G8R8A8_SRGB,			// VK_FORMAT_B8G8R8A8_SRGB = 50,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
		GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
		GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
		GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
		GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
		GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
		GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
		GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
		GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
		GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
		GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
		GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
		GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
		GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
		GrafFormat::Unsupported,			// VK_FORMAT_R16_UNORM = 70,
		GrafFormat::Unsupported,			// VK_FORMAT_R16_SNORM = 71,
		GrafFormat::Unsupported,			// VK_FORMAT_R16_USCALED = 72,
		GrafFormat::Unsupported,			// VK_FORMAT_R16_SSCALED = 73,
		GrafFormat::R16_UINT,				// VK_FORMAT_R16_UINT = 74,
		GrafFormat::R16_SINT,				// VK_FORMAT_R16_SINT = 75,
		GrafFormat::R16_SFLOAT,				// VK_FORMAT_R16_SFLOAT = 76,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16_UNORM = 77,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16_SNORM = 78,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16_USCALED = 79,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16_SSCALED = 80,
		GrafFormat::R16G16_UINT,			// VK_FORMAT_R16G16_UINT = 81,
		GrafFormat::R16G16_SINT,			// VK_FORMAT_R16G16_SINT = 82,
		GrafFormat::R16G16_SFLOAT,			// VK_FORMAT_R16G16_SFLOAT = 83,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_UNORM = 84,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SNORM = 85,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_USCALED = 86,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SSCALED = 87,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_UINT = 88,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SINT = 89,
		GrafFormat::R16G16B16_SFLOAT,		// VK_FORMAT_R16G16B16_SFLOAT = 90,
		GrafFormat::R16G16B16A16_UNORM,		// VK_FORMAT_R16G16B16A16_UNORM = 91,
		GrafFormat::R16G16B16A16_SNORM,		// VK_FORMAT_R16G16B16A16_SNORM = 92,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_USCALED = 93,
		GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_SSCALED = 94,
		GrafFormat::R16G16B16A16_UINT,		// VK_FORMAT_R16G16B16A16_UINT = 95,
		GrafFormat::R16G16B16A16_SINT,		// VK_FORMAT_R16G16B16A16_SINT = 96,
		GrafFormat::R16G16B16A16_SFLOAT,	// VK_FORMAT_R16G16B16A16_SFLOAT = 97,
		GrafFormat::R32_UINT,				// VK_FORMAT_R32_UINT = 98,
		GrafFormat::R32_SINT,				// VK_FORMAT_R32_SINT = 99,
		GrafFormat::R32_SFLOAT,				// VK_FORMAT_R32_SFLOAT = 100,
		GrafFormat::Unsupported,			// VK_FORMAT_R32G32_UINT = 101,
		GrafFormat::Unsupported,			// VK_FORMAT_R32G32_SINT = 102,
		GrafFormat::R32G32_SFLOAT,			// VK_FORMAT_R32G32_SFLOAT = 103,
		GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32_UINT = 104,
		GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32_SINT = 105,
		GrafFormat::R32G32B32_SFLOAT,		// VK_FORMAT_R32G32B32_SFLOAT = 106,
		GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32A32_UINT = 107,
		GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32A32_SINT = 108,
		GrafFormat::R32G32B32A32_SFLOAT,	// VK_FORMAT_R32G32B32A32_SFLOAT = 109,
		GrafFormat::R64_UINT,				// VK_FORMAT_R64_UINT = 110,
		GrafFormat::R64_SINT,				// VK_FORMAT_R64_SINT = 111,
		GrafFormat::R64_SFLOAT,				// VK_FORMAT_R64_SFLOAT = 112,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64_UINT = 113,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64_SINT = 114,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64_SFLOAT = 115,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64_UINT = 116,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64_SINT = 117,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64_SFLOAT = 118,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64A64_UINT = 119,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64A64_SINT = 120,
		GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64A64_SFLOAT = 121,
		GrafFormat::Unsupported,			// VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
		GrafFormat::Unsupported,			// VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
		GrafFormat::D16_UNORM,				// VK_FORMAT_D16_UNORM = 124,
		GrafFormat::X8_D24_UNORM_PACK32,	// VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
		GrafFormat::D32_SFLOAT,				// VK_FORMAT_D32_SFLOAT = 126,
		GrafFormat::S8_UINT,				// VK_FORMAT_S8_UINT = 127,
		GrafFormat::D16_UNORM_S8_UINT,		// VK_FORMAT_D16_UNORM_S8_UINT = 128,
		GrafFormat::D24_UNORM_S8_UINT,		// VK_FORMAT_D24_UNORM_S8_UINT = 129,
		GrafFormat::D32_SFLOAT_S8_UINT,		// VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
		GrafFormat::BC1_RGB_UNORM_BLOCK,	// VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
		GrafFormat::BC1_RGB_SRGB_BLOCK,		// VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
		GrafFormat::BC1_RGBA_UNORM_BLOCK,	// VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
		GrafFormat::BC1_RGBA_SRGB_BLOCK,	// VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
		GrafFormat::Unsupported,			// VK_FORMAT_BC2_UNORM_BLOCK = 135,
		GrafFormat::Unsupported,			// VK_FORMAT_BC2_SRGB_BLOCK = 136,
		GrafFormat::BC3_UNORM_BLOCK,		// VK_FORMAT_BC3_UNORM_BLOCK = 137,
		GrafFormat::BC3_SRGB_BLOCK,			// VK_FORMAT_BC3_SRGB_BLOCK = 138,
		GrafFormat::Unsupported,			// VK_FORMAT_BC4_UNORM_BLOCK = 139,
		GrafFormat::Unsupported,			// VK_FORMAT_BC4_SNORM_BLOCK = 140,
		GrafFormat::Unsupported,			// VK_FORMAT_BC5_UNORM_BLOCK = 141,
		GrafFormat::Unsupported,			// VK_FORMAT_BC5_SNORM_BLOCK = 142,
		GrafFormat::Unsupported,			// VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
		GrafFormat::Unsupported,			// VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
		GrafFormat::Unsupported,			// VK_FORMAT_BC7_UNORM_BLOCK = 145,
		GrafFormat::Unsupported,			// VK_FORMAT_BC7_SRGB_BLOCK = 146,
		GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
		GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
		GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
		GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
		GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
		GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
		GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
		GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
		GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
		GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
		GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
	};
	GrafFormat GrafUtilsVulkan::VkToGrafFormat(VkFormat vkFormat)
	{
		return VkToGrafFormatLUT[ur_uint(vkFormat)];
	}

} // end namespace UnlimRealms