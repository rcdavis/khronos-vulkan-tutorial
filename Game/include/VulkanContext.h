#pragma once

#include "volk.h"
#include "vk_mem_alloc.h"

#include "Utils/VkUtils.h"

#include <vector>

struct Platform;

struct VulkanContext {
	constexpr static uint32_t VkApiVersion = VK_API_VERSION_1_3;

	VkInstance instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	VmaAllocator vmaAllocator = VK_NULL_HANDLE;

	VkQueue graphicsQueue = VK_NULL_HANDLE;

	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkExtent2D swapchainExtent {};

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkImage depthImage = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;
	VmaAllocation depthImageAllocation = VK_NULL_HANDLE;

	uint32_t graphicsQueueFamilyIndex = VkUtils::InvalidQueueFamilyIndex;

	VkFormat depthFormat = VK_FORMAT_UNDEFINED;

	bool Init(Platform& platform);

	void Shutdown();
};
