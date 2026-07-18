#pragma once

#include "SDL3/SDL_video.h"

class Game {
public:
	Game() = default;
	~Game();

	void Run();

private:
	bool Init();
	void Shutdown();

	void Render();

private:
	SDL_Window* mWindow = nullptr;

	uint32_t mWindowWidth = 800;
	uint32_t mWindowHeight = 600;

	bool mIsRunning = false;
};
