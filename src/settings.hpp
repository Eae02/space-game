#pragma once

namespace settings {
	extern bool fullscreen;
	extern bool bloom;
	extern bool vsync;
	extern bool mouseInput;
	extern uint32_t shadowRes;
	extern uint32_t worldSize;
	extern uint32_t lodDist;
	
	void parse();
}
