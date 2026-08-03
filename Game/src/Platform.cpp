#include "Platform.h"

#include "SDL3/SDL_init.h"
#include "SDL3/SDL_error.h"
#include "Utils/Log.h"

bool Platform::Init(const WindowDesc& desc) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	if (!window.Init(desc)) {
		LOG_ERROR("Failed to init window");
		return false;
	}

	return true;
}

void Platform::Destroy() {
	window.Destroy();

	SDL_Quit();
}
