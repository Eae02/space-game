#pragma once

constexpr int TARGET_BASE_PTS = 5;
constexpr int TARGET_TIME_PTS = 1;

constexpr float TARGET_RADIUS = 30;

void initializeTargetShader();

struct Target {
	glm::vec3 pos;
	glm::vec3 truePos;
	glm::vec3 color;
};

extern bool shouldHideTargetInfo;

void beginDrawTargets();
void endDrawTargets();

void drawTarget(const Target& target, float alpha);

void drawTargetUI(const Target& target, const glm::mat4& viewProj, const glm::vec2& screenSize, const struct Ship& ship, int remTime);
