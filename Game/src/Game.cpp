#include "Game.h"

#include "Utils/Log.h"

#include "SDL3/SDL_init.h"

Game::~Game() {
	Shutdown();
}

void Game::Run() {
	if (!Init()) {
		LOG_ERROR("Failed to initialize game.");
		return;
	}

	SDL_Event event {};
	while (mIsRunning) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				mIsRunning = false;
				break;
			}
		}

		Render();
	}
}

bool Game::Init() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		LOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	mWindow = SDL_CreateWindow("Khronos Vulkan Tutorial", mWindowWidth, mWindowHeight, SDL_WINDOW_VULKAN);
	if (!mWindow) {
		LOG_ERROR("Failed to create window: {}", SDL_GetError());
		return false;
	}

	mIsRunning = true;

	LOG_INFO("Initialized game successfully!");
	return true;
}

void Game::Shutdown() {
	LOG_INFO("Shutting down game...");

	if (mWindow) {
		SDL_DestroyWindow(mWindow);
		mWindow = nullptr;
	}

	SDL_Quit();
}

void Game::Render() {

}
