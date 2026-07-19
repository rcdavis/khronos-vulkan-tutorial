#pragma once

#include <vector>

namespace Utils {
	// TODO: Move Validation Layer boolean to Config file
	std::vector<const char*> GetRequiredVulkanExtensions(bool addValidationLayerExtensions = true);
} // namespace Utils
