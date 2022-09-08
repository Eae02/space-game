#include "settings.hpp"
#include "utils.hpp"

#include <fstream>
#include <cmath>

namespace settings {
	bool fullscreen         = false;
	bool bloom              = true;
	bool vsync              = false;
	bool mouseInput         = false;
	uint32_t shadowRes      = 1024;
	uint32_t worldSize      = 4;
	uint32_t lodDist        = 300;
	
	void parse() {
		std::vector<std::pair<std::string, std::string>> settings;
		
		std::ifstream stream(exeDirPath + "settings.txt");
		std::string line;
		while (std::getline(stream, line)) {
			size_t split = line.find(':');
			if (split != std::string::npos) {
				settings.emplace_back(line.substr(0, split), line.substr(split + 1));
			}
		}
		
		auto getBool = [&] (const std::string& name, bool& value) {
			for (const auto& setting : settings) {
				if (setting.first == name) {
					if (setting.second == "true") value = true;
					if (setting.second == "false") value = false;
				}
			}
		};
		auto getUInt = [&] (const std::string& name, uint32_t& value, uint32_t minValue) {
			for (const auto& setting : settings) {
				if (setting.first == name) {
					try {
						int vs = std::stoi(setting.second);
						if (vs >= 0 && (uint32_t)vs >= minValue)
							value = vs;
					} catch (...) { }
				}
			}
		};
		
		getBool("fullscreen", fullscreen);
		getBool("bloom", bloom);
		getBool("vsync", vsync);
		getBool("mouseInput", mouseInput);
		getUInt("shadowRes", shadowRes, 128);
		worldSize = glm::clamp(worldSize, 1U, 5U);
		getUInt("lodDist", lodDist, 100);
	}
}
