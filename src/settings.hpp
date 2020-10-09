#pragma once

namespace settings {
	extern bool fullscreen;
	extern bool bloom;
	extern bool vsync;
	extern bool volumetricLighting;
	extern bool motionBlur;
	extern uint32_t shadowRes;
	extern uint32_t worldSize;
	extern uint32_t lodDist;
	
	void parse();
}