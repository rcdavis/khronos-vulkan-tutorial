#pragma once

#include "volk.h"
#include "Utils/VkUtils.h"

struct VulkanContext {
	constexpr static uint32_t VkApiVersion = VK_API_VERSION_1_3;

	VkInstance instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	uint32_t graphicsQueueFamilyIndex = VkUtils::InvalidQueueFamilyIndex;

	bool Init();

	void Shutdown();
};
