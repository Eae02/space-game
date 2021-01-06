#pragma once

#include "ship.hpp"
#include "target.hpp"
#include "graphics/ui.hpp"

struct Game {
	Ship ship;
	
	int score[3];
	bool fadingTargets;
	float targetsAlpha;
	
	glm::vec3 vignetteColor;
	float vignetteColorFade;
	
	float blueRequiredSpeed;
	
	Target targets[3];
	float remTime;
	
	float invincibleTime;
	bool invincibleOverride;
	
	bool isGameOver;
	
	Game();
	
	void newGame();
	
	void runFrame(const struct InputState& curInput, const struct InputState& prevInput);
	
	void initRenderSettings(uint32_t drawableWidth, uint32_t drawableHeight, struct RenderSettings& renderSettings) const;
	
	glm::vec3 getTargetPosition(float speed) const;
	
	ColoredStringBuilder buildScoreString();
};
