#pragma once

#include "SDL3/SDL_video.h"

struct Platform {
	SDL_Window* window = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;

	bool Init(const char* title, uint32_t width, uint32_t height);

	void Destroy();
};
