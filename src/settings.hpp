#pragma once

namespace settings {
	extern bool fullscreen;
	extern bool bloom;
	extern bool vsync;
	extern bool volumetricLighting;
	extern uint32_t shadowRes;
	
	void parse();
}
