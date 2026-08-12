#include "Utils/FileUtils.h"

#include <fstream>
#include "Utils/Log.h"

namespace FileUtils {
	std::vector<char> ReadBytes(const char* const filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file) {
			LOG_ERROR("Failed to open file to read bytes: {}", filename);
			return {};
		}

		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(std::data(buffer), std::size(buffer));
		return buffer;
	}
}
