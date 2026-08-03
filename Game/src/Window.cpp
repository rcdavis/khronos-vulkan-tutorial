#include "Window.h"

#include "SDL3/SDL_error.h"
#include "Utils/Log.h"

bool Window::Init(const WindowDesc& desc) {
	SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;
	if (desc.isResizable)
		windowFlags |= SDL_WINDOW_RESIZABLE;
	if (desc.isFullscreen)
		windowFlags |= SDL_WINDOW_FULLSCREEN;

	window = SDL_CreateWindow(desc.title, desc.width, desc.height, windowFlags);
	if (!window) {
		LOG_ERROR("Failed to create window: {}", SDL_GetError());
		return false;
	}

	width = desc.width;
	height = desc.height;

	return true;
}

void Window::Destroy() {
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	width = 0;
	height = 0;
}
