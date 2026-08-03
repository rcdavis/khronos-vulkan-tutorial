#pragma once

#include "volk.h"
#include <SDL3/SDL_video.h>

struct WindowDesc {
	const char* title = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	bool isResizable = false;
	bool isFullscreen = false;
};

struct Window {
	SDL_Window* window = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	bool Init(const WindowDesc& desc);

	void Destroy();

	VkSurfaceKHR CreateVulkanSurface(VkInstance instance) const;
};
