#include "game.hpp"
#include "input.hpp"
#include "utils.hpp"
#include "resources.hpp"
#include "graphics/renderer.hpp"
#include "graphics/asteroids.hpp"

#include <random>

std::mt19937 globalRng(time(nullptr));

glm::vec3 Game::getTargetPosition(float speed) const {
	float dist = speed * 40;
	glm::vec3 pos;
	do {
		pos = ship.pos + dist * randomDirection(globalRng);
	} while (anyAsteroidIntersects(pos, TARGET_RADIUS));
	return pos;
}

Game::Game() {
	targets[0].color = glm::convertSRGBToLinear(glm::vec3(0.4f, 0.7f, 1.0f));
	targets[1].color = glm::convertSRGBToLinear(glm::vec3(1.0f, 1.0f, 0.4f));
	targets[2].color = glm::convertSRGBToLinear(glm::vec3(1.0f, 0.4f, 0.4f));
}

void Game::newGame() {
	std::fill_n(score, 3, 0);
	fadingTargets = true;
	targetsAlpha = 0;
	vignetteColor = glm::vec3(0);
	vignetteColorFade = 0;
	blueRequiredSpeed = 100;
	remTime = 60;
	
	std::uniform_real_distribution<float> startPosGen(0, ASTEROID_BOX_SIZE);
	clearAsteroidWrapping();
	do {
		ship.pos = glm::vec3(startPosGen(globalRng), startPosGen(globalRng), startPosGen(globalRng));
	} while (anyAsteroidIntersects(ship.pos, 10));
	ship.rollOffset = 0;
	ship.rollVelocity = 0;
	ship.vel = glm::vec3(0);
	ship.forwardVel = 0;
	invincibleTime = 3;
	isGameOver = false;
}

void Game::runFrame(const InputState& curInput, const InputState& prevInput) {
	if (isGameOver)
		return;
	
	gameTime += dt;
	
	ship.update(curInput, prevInput);
	
	glm::vec3 boxPositionOffset = glm::vec3(ship.boxIndex) * ASTEROID_BOX_SIZE;
	for (Target& target : targets) {
		target.truePos = target.pos - boxPositionOffset;
	}
	
	if (invincibleTime > 0) {
		invincibleTime -= dt;
	} else if (ship.intersected) {
		isGameOver = true;
	}
	
	if (!fadingTargets) {
		for (int t = 0; t < 3; t++) {
			if (glm::distance(ship.pos, targets[t].truePos) < (TARGET_RADIUS * 1.2f + res::shipModel.sphereRadius)) {
				targetsAlpha = 1;
				fadingTargets = true;
				vignetteColor = targets[t].color * 0.5f;
				vignetteColorFade = 1.0f;
				int addScore = TARGET_BASE_PTS + (int)std::round(TARGET_TIME_PTS * remTime);
				score[t] += addScore;
				blueRequiredSpeed += 50;
				invincibleTime = 2;
				break;
			}
		}
		if (ship.forwardVel > 1) {
			remTime -= dt;
		}
		if (remTime < 0 && !isGameOver) {
			isGameOver = true;
			remTime = 0;
		}
	} else {
		targetsAlpha -= dt * 5;
		if (targetsAlpha < 0) {
			targets[0].pos = getTargetPosition(std::min(blueRequiredSpeed, 400.0f));
			targets[1].pos = getTargetPosition(500);
			targets[2].pos = getTargetPosition(650);
			ship.boxIndex = glm::ivec3(0);
			remTime = 60;
			targetsAlpha = 1;
			fadingTargets = false;
		}
	}
	
	vignetteColorFade = std::max(vignetteColorFade - dt * 3.0f, 0.0f);
}

void Game::initRenderSettings(uint32_t drawableWidth, uint32_t drawableHeight, RenderSettings& renderSettings) const {
	glm::vec3 targetPlColor(0);
	glm::vec3 targetPlPos(0);
	float closestTargetDist2 = INFINITY;
	for (const Target& target : targets) {
		float dist2 = glm::distance2(target.truePos, ship.pos);
		if (dist2 < closestTargetDist2) {
			closestTargetDist2 = dist2;
			targetPlColor = target.color * 2.0f * targetsAlpha;
			targetPlPos = target.truePos;
		}
	}
	
	constexpr float LOW_FOV = 75.0f;
	constexpr float HIGH_FOV = 100.0f;
	
	float fov = glm::radians(glm::mix(LOW_FOV, HIGH_FOV, ship.speed01));
	glm::mat4 projMatrix = glm::perspectiveFov(fov, (float)drawableWidth, (float)drawableHeight, Z_NEAR, Z_FAR);
	glm::mat4 inverseProjMatrix = glm::inverse(projMatrix);
	glm::mat4 vpMatrixInv = ship.viewMatrixInv * inverseProjMatrix;
	
	renderSettings.vpMatrix = projMatrix * ship.viewMatrix;
	renderSettings.vpMatrixInverse = vpMatrixInv;
	renderSettings.cameraPos = ship.cameraPosition;
	renderSettings.gameTime = gameTime;
	renderSettings.sunColor = SUN_COLOR;
	renderSettings.sunDir = SUN_DIR;
	renderSettings.plColor = targetPlColor;
	renderSettings.plPosition = targetPlPos;
}

ColoredStringBuilder Game::buildScoreString() {
	constexpr float POINT_COLOR_WHITE_FADE = 0.5f;
	ColoredStringBuilder scoreStringBuilder;
	for (int t = 2; t >= 0; t--) {
		std::string scoreString = std::to_string(score[t]);
		scoreStringBuilder.push(scoreString, glm::vec4(glm::mix(targets[t].color, glm::vec3(1), POINT_COLOR_WHITE_FADE), 1));
		if (t != 0) {
			scoreStringBuilder.push(" ", glm::vec4(1));
		}
	}
	return scoreStringBuilder;
}
