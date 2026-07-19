#include "VulkanContext.h"

#include "Utils/Log.h"

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
		.apiVersion = VulkanContext::VkApiVersion
	};

	const VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};

	const VkResult result = vkCreateInstance(&createInfo, nullptr, &context.instance);
	if (result != VK_SUCCESS) {
		LOG_ERROR("Failed to create Vulkan instance: {}", (uint32_t)result);
		return false;
	}

	volkLoadInstance(context.instance);

	return true;
}

bool VulkanContext::Init() {
	if (!VulkanContext_CreateInstance(*this)) {
		LOG_ERROR("Failed to create Vulkan instance.");
		return false;
	}

	return true;
}

void VulkanContext::Shutdown() {
	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}
}
