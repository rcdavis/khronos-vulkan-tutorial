#pragma once

#include "Window.h"

struct Platform {
	Window window;

	bool Init(const WindowDesc& desc);

	void Destroy();
};
