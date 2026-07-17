
#include "Utils/Log.h"

int main(int argc, char** argv) {
	Log::Init("Vulkan");

	LOG_INFO("Hello, Vulkan!");

	return 0;
}
