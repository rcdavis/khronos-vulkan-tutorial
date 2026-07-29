#include "Utils/VkUtils.h"

#include "SDL3/SDL_vulkan.h"
#include "Utils/Log.h"
#include "Config.h"

namespace VkUtils {
	std::vector<VkLayerProperties> GetInstanceLayerProperties() {
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (count == 0)
			return {};

		std::vector<VkLayerProperties> layers(count);
		vkEnumerateInstanceLayerProperties(&count, std::data(layers));

		return layers;
	}

	std::vector<VkPhysicalDevice> GetPhysicalDevices(VkInstance instance) {
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(instance, &count, nullptr);
		if (count == 0)
			return {};

		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(instance, &count, std::data(devices));

		return devices;
	}

	std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice device) {
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
		if (count == 0)
			return {};

		std::vector<VkQueueFamilyProperties> queueFamilies(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, std::data(queueFamilies));

		return queueFamilies;
	}

	std::vector<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice device) {
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		if (count == 0)
			return {};

		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, std::data(extensions));

		return extensions;
	}

	bool CheckInstanceLayerSupport(std::span<const char* const> requiredLayers) {
		const auto availableLayers = GetInstanceLayerProperties();

		for (const char* requiredLayer : requiredLayers) {
			bool layerFound = false;
			for (const auto& layerProps : availableLayers) {
				if (strcmp(requiredLayer, layerProps.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device, std::span<const char* const> requiredExtensions) {
		const auto availableExtensions = GetDeviceExtensionProperties(device);

		for (const char* requiredExtension : requiredExtensions) {
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

	std::vector<const char*> GetRequiredVulkanExtensions() {
		uint32_t extensionCount = 0;
		auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
		if (!extensions) {
			LOG_ERROR("Failed to get required Vulkan extensions from SDL: {}.", SDL_GetError());
			return {};
		}

		std::vector<const char*> result(extensions, extensions + extensionCount);
		if constexpr (Config::EnableValidationLayers) {
			result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return result;
	}

	uint32_t FindGraphicsPresentQueueFamilyIndex(VkInstance instance, VkPhysicalDevice device) {
		const auto queueFamilies = GetQueueFamilyProperties(device);
		for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
			if (!(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				continue;

			if (SDL_Vulkan_GetPresentationSupport(instance, device, i))
				return i;
		}

		return InvalidQueueFamilyIndex;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData
	) {
		switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			LOG_INFO("Vulkan validation: {}", callbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			LOG_WARN("Vulkan validation: {}", callbackData->pMessage);
			break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			LOG_ERROR("Vulkan validation: {}", callbackData->pMessage);
			break;

		default:
			LOG_TRACE("Vulkan validation: {}", callbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}
} // namespace VkUtils
