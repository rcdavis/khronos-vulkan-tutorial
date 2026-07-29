#include "VulkanContext.h"

#include "Utils/Log.h"
#include "Config.h"
#include "vulkan/vulkan_core.h"

constexpr static std::array ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

constexpr static std::array RequiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static bool VulkanContext_CreateInstance(VulkanContext& context);
static bool VulkanContext_CreateDevice(VulkanContext& context);

static int VulkanContext_GetDeviceScore(VkPhysicalDevice device);

static bool CheckValidationLayerSupport() {
	const auto availableLayers = VkUtils::GetInstanceLayerProperties();
	for (const char* layerName : ValidationLayers) {
		bool layerFound = false;
		for (const auto& layerProps : availableLayers) {
			if (strcmp(layerName, layerProps.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

static bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	const auto availableExtensions = VkUtils::GetDeviceExtensionProperties(device);

	for (const char* requiredExtension : RequiredDeviceExtensions) {
		bool extensionFound = false;
		for (const auto& extensionProps : availableExtensions) {
			if (strcmp(requiredExtension, extensionProps.extensionName) == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)
			return false;
	}

	return true;
}

bool VulkanContext::Init() {
	if (!VulkanContext_CreateInstance(*this)) {
		LOG_ERROR("Failed to create Vulkan instance.");
		return false;
	}

	if constexpr (Config::EnableValidationLayers) {
		constexpr auto debugCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
		if (vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			LOG_ERROR("Failed to create debug utils messenger");
			return false;
		}
	}

	if (!VulkanContext_CreateDevice(*this)) {
		LOG_ERROR("Failed to create Vulkan device.");
		return false;
	}

	return true;
}

void VulkanContext::Shutdown() {
	if (device != VK_NULL_HANDLE) {
		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
	}

	if constexpr (Config::EnableValidationLayers) {
		if (debugMessenger != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
			debugMessenger = VK_NULL_HANDLE;
		}
	}

	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}

	physicalDevice = VK_NULL_HANDLE;
	graphicsQueueFamilyIndex = VkUtils::InvalidQueueFamilyIndex;
}

static bool VulkanContext_CreateInstance(VulkanContext& context) {
	if (volkInitialize() != VK_SUCCESS) {
		LOG_ERROR("Failed to initialize volk.");
		return false;
	}

	constexpr VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Khronos Vulkan Tutorial",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VulkanContext::VkApiVersion,
	};

	const auto requiredExtensions = VkUtils::GetRequiredVulkanExtensions();

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = (uint32_t)std::size(requiredExtensions),
		.ppEnabledExtensionNames = std::data(requiredExtensions),
	};

	constexpr auto debugMessengerCreateInfo = VkUtils::CreateDebugMessengerCreateInfo();
	if constexpr (Config::EnableValidationLayers) {
		if (!CheckValidationLayerSupport()) {
			LOG_ERROR("Validation layers requested, but not available.");
			return false;
		}

		createInfo.pNext = &debugMessengerCreateInfo;
		createInfo.enabledLayerCount = (uint32_t)std::size(ValidationLayers);
		createInfo.ppEnabledLayerNames = std::data(ValidationLayers);
	}

	const VkResult result = vkCreateInstance(&createInfo, nullptr, &context.instance);
	if (result != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan instance: {}", (uint32_t)result);
		return false;
	}

	volkLoadInstance(context.instance);

	return true;
}

static bool VulkanContext_CreateDevice(VulkanContext& context) {
	const auto physicalDevices = VkUtils::GetPhysicalDevices(context.instance);
	if (std::empty(physicalDevices)) {
		LOG_ERROR("Failed to find any Vulkan physical devices.");
		return false;
	}

	int bestScore = 0;
	VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
	for (VkPhysicalDevice device : physicalDevices) {
		const int score = VulkanContext_GetDeviceScore(device);
		if (score > bestScore) {
			bestScore = score;
			bestDevice = device;
		}
	}

	if (bestDevice == VK_NULL_HANDLE) {
		LOG_ERROR("Failed to find a suitable Vulkan physical device.");
		return false;
	}

	uint32_t queueFamilyIndex = VkUtils::FindGraphicsPresentQueueFamilyIndex(context.instance, bestDevice);
	if (queueFamilyIndex == VkUtils::InvalidQueueFamilyIndex) {
		LOG_ERROR("Failed to find a suitable Vulkan queue family index.");
		return false;
	}

	constexpr float queuePriority = 1.0f;
	const VkDeviceQueueCreateInfo queueCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	VkPhysicalDeviceVulkan12Features features12 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
	};

	VkPhysicalDeviceVulkan13Features features13 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &features12,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};

	constexpr VkPhysicalDeviceFeatures enabledFeatures {
		.samplerAnisotropy = VK_TRUE,
	};

	const VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features13,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = (uint32_t)std::size(RequiredDeviceExtensions),
		.ppEnabledExtensionNames = std::data(RequiredDeviceExtensions),
		.pEnabledFeatures = &enabledFeatures,
	};

	if (vkCreateDevice(bestDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan device.");
		return false;
	}

	volkLoadDevice(context.device);

	context.physicalDevice = bestDevice;
	context.graphicsQueueFamilyIndex = queueFamilyIndex;

	return true;
}

static int VulkanContext_GetDeviceScore(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	if (deviceProperties.apiVersion < VulkanContext::VkApiVersion) {
		LOG_WARN("Vulkan physical device does not support required API version: {}", deviceProperties.deviceName);
		return -1;
	}

	int score = 0;
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
		score += 500;
	} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
		score += 100;
	} else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
		score += 50;
	} else {
		LOG_WARN("Vulkan physical device is an unknown type: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!CheckDeviceExtensionSupport(device)) {
		LOG_WARN("Vulkan physical device does not support required extensions: {}", deviceProperties.deviceName);
		return -1;
	}

	VkPhysicalDeviceVulkan12Features features12 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	};

	VkPhysicalDeviceVulkan13Features features13 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &features12,
	};

	VkPhysicalDeviceFeatures2 features2 {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13,
	};
	vkGetPhysicalDeviceFeatures2(device, &features2);

	if (!features2.features.samplerAnisotropy) {
		LOG_WARN("Vulkan physical device does not support sampler anisotropy: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.bufferDeviceAddress) {
		LOG_WARN("Vulkan physical device does not support buffer device address: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.descriptorIndexing) {
		LOG_WARN("Vulkan physical device does not support descriptor indexing: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.shaderSampledImageArrayNonUniformIndexing) {
		LOG_WARN("Vulkan physical device does not support shader sampled image array non uniform indexing: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.descriptorBindingVariableDescriptorCount) {
		LOG_WARN("Vulkan physical device does not support descriptor binding variable descriptor count: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features12.runtimeDescriptorArray) {
		LOG_WARN("Vulkan physical device does not support runtime descriptor array: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features13.synchronization2) {
		LOG_WARN("Vulkan physical device does not support synchronization2: {}", deviceProperties.deviceName);
		return -1;
	}

	if (!features13.dynamicRendering) {
		LOG_WARN("Vulkan physical device does not support dynamic rendering: {}", deviceProperties.deviceName);
		return -1;
	}

	return score;
}
