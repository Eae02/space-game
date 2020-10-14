#pragma once

#include "ship.hpp"
#include "target.hpp"

struct Game {
	Ship ship;
	
	int score;
	bool fadingTargets;
	float targetsAlpha;
	
	glm::vec3 vignetteColor;
	float vignetteColorFade;
	
	float blueRequiredSpeed;
	
	Target targets[3];
	float remTime;
	float gameTime;
	
	float invincibleTime;
	
	bool isGameOver;
	
	Game();
	
	void newGame();
	
	void runFrame(const struct InputState& curInput, const struct InputState& prevInput);
	
	void initRenderSettings(uint32_t drawableWidth, uint32_t drawableHeight, struct RenderSettings& renderSettings) const;
	
	glm::vec3 getTargetPosition(float speed) const;
};
