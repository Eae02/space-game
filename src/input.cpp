#include "input.hpp"
#include <SDL_scancode.h>

void InputState::keyStateChanged(int scancode, bool newState) {
	if (scancode == SDL_SCANCODE_A || scancode == SDL_SCANCODE_LEFT) {
		leftKey = newState;
	} else if (scancode == SDL_SCANCODE_D || scancode == SDL_SCANCODE_RIGHT) {
		rightKey = newState;
	} else if (scancode == SDL_SCANCODE_S || scancode == SDL_SCANCODE_DOWN) {
		downKey = newState;
	} else if (scancode == SDL_SCANCODE_W || scancode == SDL_SCANCODE_UP) {
		upKey = newState;
	} else if (scancode == SDL_SCANCODE_Q) {
		rollLeftKey = newState;
	} else if (scancode == SDL_SCANCODE_E) {
		rollRightKey = newState;
	}
}
