#pragma once

struct InputState {
	bool leftKey      = false;
	bool rightKey     = false;
	bool upKey        = false;
	bool downKey      = false;
	bool rollLeftKey  = false;
	bool rollRightKey = false;
	bool moreSpeedKey = false;
	bool lessSpeedKey = false;
	int mouseX = 0;
	int mouseY = 0;
	int mouseDX = 0;
	int mouseDY = 0;
	
	void keyStateChanged(int scancode, bool newState);
};
