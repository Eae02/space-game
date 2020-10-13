#pragma once

constexpr int BLUE_BASE_PTS = 10;
constexpr int BLUE_TIME_PTS = 2;
constexpr int YELLOW_BASE_PTS = 50;
constexpr int YELLOW_TIME_PTS = 5;
constexpr int RED_BASE_PTS = 100;
constexpr int RED_TIME_PTS = 5;

constexpr float TARGET_RADIUS = 30;

void initializeTargetShader();

struct Target {
	glm::vec3 pos;
	glm::vec3 truePos;
	glm::vec3 color;
};

extern bool shouldHideTargetInfo;

void beginDrawTargets(float gameTime, float dt);
void endDrawTargets();

void drawTarget(const Target& target);

void drawTargetUI(const Target& target, const glm::mat4& viewProj, const glm::vec2& screenSize, const struct Ship& ship, int remTime);
