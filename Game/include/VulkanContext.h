#pragma once

#include "volk.h"

struct VulkanContext {
	constexpr static uint32_t VkApiVersion = VK_API_VERSION_1_3;

	VkInstance instance = VK_NULL_HANDLE;

	bool Init();

	void Shutdown();
};
