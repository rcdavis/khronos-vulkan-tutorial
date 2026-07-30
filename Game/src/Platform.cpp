#include "Platform.h"

#include "SDL3/SDL_init.h"
#include "SDL3/SDL_error.h"
#include "Utils/Log.h"

bool Platform::Init(const char* title, uint32_t width, uint32_t height) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
	if (!window) {
		LOG_ERROR("Failed to create window: {}", SDL_GetError());
		return false;
	}

	return true;
}

void Platform::Destroy() {
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	SDL_Quit();

	width = 0;
	height = 0;
}
