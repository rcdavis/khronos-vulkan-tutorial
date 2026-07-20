#pragma once

namespace Config {
#ifdef NDEBUG
	constexpr bool EnableValidationLayers = false;
#else
	constexpr bool EnableValidationLayers = true;
#endif
}
