#include "Utils/VkUtils.h"

#include "volk.h"
#include "SDL3/SDL_vulkan.h"
#include "Utils/Log.h"

namespace VkUtils {
	std::vector<const char*> GetRequiredVulkanExtensions(bool addValidationLayerExtensions) {
		uint32_t extensionCount = 0;
		auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
		if (!extensions) {
			LOG_ERROR("Failed to get required Vulkan extensions from SDL: {}.", SDL_GetError());
			return {};
		}

		std::vector<const char*> result(extensions, extensions + extensionCount);
		if (addValidationLayerExtensions) {
			result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return result;
	}
} // namespace VkUtils
