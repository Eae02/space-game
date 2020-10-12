#pragma once

constexpr float TARGET_RADIUS = 30;

void initializeTargetShader();

struct Target {
	glm::vec3 pos;
	glm::vec3 truePos;
	glm::vec3 color;
};

void beginDrawTargets(float gameTime);
void endDrawTargets();

void drawTarget(const Target& target);
