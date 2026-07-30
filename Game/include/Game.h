#pragma once

#include "Platform.h"
#include "VulkanContext.h"

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
	Platform mPlatform;
	VulkanContext mVulkanContext;

	bool mIsRunning = false;
};
