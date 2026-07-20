#include "VulkanContext.h"

#include "Utils/Log.h"
#include "Utils/VkUtils.h"
#include "Config.h"

constexpr static std::array ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

static bool VulkanContext_CreateInstance(VulkanContext& context);

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

	return true;
}

void VulkanContext::Shutdown() {
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
