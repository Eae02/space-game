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
	
	void keyStateChanged(int scancode, bool newState);
};
