#include "Game.h"

#include "SDL3/SDL_events.h"
#include "Utils/Log.h"

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
	constexpr WindowDesc windowDesc {
		.title = "Khronos Vulkan Tutorial",
		.width = 800,
		.height = 600,
		.isResizable = false,
		.isFullscreen = false,
	};

	if (!mPlatform.Init(windowDesc)) {
		LOG_ERROR("Failed to initialize platform.");
		return false;
	}

	if (!mVulkanContext.Init(mPlatform)) {
		LOG_ERROR("Failed to initialize Vulkan context.");
		return false;
	}

	mIsRunning = true;

	LOG_INFO("Initialized game successfully!");
	return true;
}

void Game::Shutdown() {
	LOG_INFO("Shutting down game...");

	mVulkanContext.Shutdown();
	mPlatform.Destroy();
}

void Game::Render() {

}
