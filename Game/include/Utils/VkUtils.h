#pragma once

#include <vector>

namespace VkUtils {
	// TODO: Move Validation Layer boolean to Config file
	std::vector<const char*> GetRequiredVulkanExtensions(bool addValidationLayerExtensions = true);
} // namespace VkUtils
