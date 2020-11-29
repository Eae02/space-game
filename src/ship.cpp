#include "ship.hpp"
#include "input.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "graphics/asteroids.hpp"
#include "graphics/shader.hpp"

constexpr float MAX_SPEED_LRUP = 10;
constexpr float ACCEL_TIME_LRUP = 0.3f;
constexpr float DEACCEL_TIME_LRUP = 0.5f;

constexpr float MAX_ROLL_SPEED = 1.0f;
constexpr float ACCEL_TIME_ROLL = 0.2f;
constexpr float DEACCEL_TIME_ROLL = 0.4f;

constexpr float PITCH_SPEED = 0.5f;
constexpr float YAW_SPEED = 0.4f;
constexpr float YAW_ROLL_SPEED = 0.1f;

constexpr float MOVE_LR_MAX_ROLL = 0.4f;

static inline float calculateAcceleration(float move, float& vel, float deaccelRate) {
	if (abs(move) < 0.01f) {
		if (vel > 0) {
			vel = std::max(vel - dt * deaccelRate, 0.0f);
		} else {
			vel = std::min(vel + dt * deaccelRate, 0.0f);
		}
		return 0;
	}
	return move;
}

constexpr float MIN_SPEED = 70;
constexpr float MAX_SPEED = 750;
constexpr float FWD_ACCEL_TIME = 7;
constexpr float FWD_DEACCEL_TIME = 1;

constexpr float MOUSE_MOVE_SENSITIVITY = 0.02f;
constexpr float MAX_MOUSE_ACCEL = 50.0f;

void Ship::update(const InputState& curInput, const InputState& prevInput) {
	float moveX = (float)curInput.leftKey - (float)curInput.rightKey;
	float moveY = (float)curInput.upKey - (float)curInput.downKey;
	if (settings::mouseInput) {
		moveX = glm::clamp(moveX - curInput.mouseDX * MOUSE_MOVE_SENSITIVITY, -MAX_MOUSE_ACCEL, MAX_MOUSE_ACCEL);
		moveY = glm::clamp(moveY + curInput.mouseDY * MOUSE_MOVE_SENSITIVITY, -MAX_MOUSE_ACCEL, MAX_MOUSE_ACCEL);
	}
	
	float accelX = calculateAcceleration(moveX, vel.x, MAX_SPEED_LRUP / DEACCEL_TIME_LRUP);
	float accelY = calculateAcceleration(moveY, vel.y, MAX_SPEED_LRUP / DEACCEL_TIME_LRUP);
	vel.x += accelX * dt * (MAX_SPEED_LRUP / ACCEL_TIME_LRUP);
	vel.y += accelY * dt * (MAX_SPEED_LRUP / ACCEL_TIME_LRUP);
	float velLen = std::hypot(vel.x, vel.y);
	if (velLen > MAX_SPEED_LRUP) {
		vel.x *= MAX_SPEED_LRUP / velLen;
		vel.y *= MAX_SPEED_LRUP / velLen;
	}
	
	float accelRoll = calculateAcceleration((int)curInput.rollRightKey - (int)curInput.rollLeftKey, rollVelocity, MAX_ROLL_SPEED / DEACCEL_TIME_ROLL);
	rollVelocity = glm::clamp(rollVelocity + accelRoll * dt, -MAX_ROLL_SPEED, MAX_ROLL_SPEED);
	
	
	glm::vec3 pitchAxis = rotation * glm::vec3(1, 0, 0);
	rotation = glm::angleAxis(accelY * PITCH_SPEED * dt, pitchAxis) * rotation;
	
	glm::vec3 yawAxis = rotation * glm::vec3(0, 1, 0);
	rotation = glm::angleAxis(accelX * YAW_SPEED * dt, yawAxis) * rotation;
	
	glm::vec3 rollAxis = rotation * glm::vec3(0, 0, 1);
	rotation = glm::angleAxis(accelX * YAW_ROLL_SPEED * dt + rollVelocity * dt, rollAxis) * rotation;
	
	float desiredRollOffset = -vel.x * MOVE_LR_MAX_ROLL / MAX_SPEED_LRUP;
	rollOffset += (desiredRollOffset - rollOffset) * std::min(3 * dt, 1.0f);
	
	engineIntensity += dt * 2.0f * (curInput.moreSpeedKey ? 1 : -1);
	engineIntensity = glm::clamp(engineIntensity, 0.0f, 1.0f);
	
	if (curInput.moreSpeedKey)
		stopped = false;
	
	if (stopped) {
		forwardVel = std::max(forwardVel - dt * (MAX_SPEED - MIN_SPEED) / FWD_DEACCEL_TIME, 0.0f);
	} else if (curInput.moreSpeedKey || forwardVel < MIN_SPEED) {
		forwardVel = std::min(forwardVel + dt * (MAX_SPEED - MIN_SPEED) / FWD_ACCEL_TIME, MAX_SPEED);
	} else if (curInput.lessSpeedKey) {
		forwardVel = std::max(forwardVel - dt * (MAX_SPEED - MIN_SPEED) / FWD_DEACCEL_TIME, MIN_SPEED);
	}
	speed01 = std::max((forwardVel - MIN_SPEED) / (MAX_SPEED - MIN_SPEED), 0.0f);
	
	glm::vec3 moveVector = rotation * (glm::vec3(vel.x, 0, forwardVel) * dt);
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1), rollOffset, rollAxis) * glm::mat4_cast(rotation);
	glm::mat4 invRotationMatrix = glm::transpose(rotationMatrix);
	
	float moveDist = glm::length(moveVector);
	if (moveDist > 50) {
		moveVector *= 50.0f / moveDist;
		moveDist = 50;
	}
	
	//Updates the camera
	cameraRotation = glm::slerp(cameraRotation, rotation, std::min(2 * dt, 1.0f));
	viewMatrix =
		glm::lookAt(glm::vec3(0, 5, -14), glm::vec3(0, 3, 0), glm::vec3(0, 1, 0)) *
		glm::transpose(glm::mat4_cast(cameraRotation)) *
		glm::translate(glm::mat4(1), -pos - moveVector);
	viewMatrixInv = glm::inverse(viewMatrix);
	cameraPosition = glm::vec3(viewMatrixInv[3]);
	
	updateAsteroidWrapping(cameraPosition);
	
	//Collision detection
	intersected = false;
	constexpr float COLLISION_DETECT_MAX_STEP_LEN = 4;
	const int numColDetectSteps = std::max((int)std::ceil(moveDist / COLLISION_DETECT_MAX_STEP_LEN), 1);
	for (int step = 1; step <= numColDetectSteps; step++) {
		const glm::vec3 intPos = pos + moveVector * ((float)step / (float)numColDetectSteps);
		const glm::mat4 colCheckWorldMatrix = glm::translate(glm::mat4(1), intPos) * rotationMatrix;
		const glm::mat4 colCheckWorldMatrixInv = invRotationMatrix * glm::translate(glm::mat4(1), -intPos);
		intersected |= anyAsteroidIntersects(
			res::shipModel.minPos * glm::vec3(0.8f, 0.7f, 1),
			res::shipModel.maxPos * glm::vec3(0.8f, 0.7f, 1),
			colCheckWorldMatrix, colCheckWorldMatrixInv);
	}
	
	pos += moveVector;
	glm::ivec3 boxOffset(glm::floor(pos / ASTEROID_BOX_SIZE));
	boxIndex += boxOffset;
	glm::vec3 boxOffsetShift = glm::vec3(boxOffset) * ASTEROID_BOX_SIZE;
	pos -= boxOffsetShift;
	
	worldMatrix = glm::translate(glm::mat4(1), pos) * rotationMatrix;
}

static Shader modelShader, emissiveShader;

static const glm::vec3 EMISSIVE_COLOR = glm::convertSRGBToLinear(glm::vec3(153, 196, 233) / 255.0f);

static constexpr float SHIP_SPEC_LO = 3;
static constexpr float SHIP_SPEC_HI = 20;
static constexpr float SHIP_SPEC_EXP = 100;

static constexpr float WINDOW_SPEC_LO = 3;
static constexpr float WINDOW_SPEC_HI = 20;
static constexpr float WINDOW_SPEC_EXP = 100;

constexpr float LOW_ENGINE_COLOR = 4;
constexpr float HIGH_ENGINE_COLOR = 7;

void Ship::draw() const {
	glBindVertexArray(Model::vao);
	res::shipModel.bind();
	
	modelShader.use();
	
	res::shipAlbedo.bind(0);
	res::shipNormals.bind(1);
	glUniformMatrix4fv(0, 1, false, (const float*)&worldMatrix);
	
	glUniform3f(1, SHIP_SPEC_LO, SHIP_SPEC_HI, SHIP_SPEC_EXP);
	res::shipModel.drawMesh(res::shipModel.findMesh("Aluminum"));
	
	glUniform3f(1, WINDOW_SPEC_LO, WINDOW_SPEC_HI, WINDOW_SPEC_EXP);
	res::shipModel.drawMesh(res::shipModel.findMesh("Window"));
	
	emissiveShader.use();
	float emissiveScale = glm::mix(LOW_ENGINE_COLOR, HIGH_ENGINE_COLOR, engineIntensity);
	glm::vec3 scaledEmissive = emissiveScale * EMISSIVE_COLOR;
	glUniform3fv(1, 1, (const float*)&scaledEmissive);
	glUniformMatrix4fv(0, 1, false, (const float*)&worldMatrix);
	res::shipModel.drawMesh(res::shipModel.findMesh("BlueL"));
	res::shipModel.drawMesh(res::shipModel.findMesh("BlueS"));
}

void Ship::initShaders() {
	modelShader.attachStage(GL_VERTEX_SHADER, "model.vs.glsl");
	modelShader.attachStage(GL_FRAGMENT_SHADER, "model.fs.glsl");
	modelShader.link("model");

	emissiveShader.attachStage(GL_VERTEX_SHADER, "model.vs.glsl");
	emissiveShader.attachStage(GL_FRAGMENT_SHADER, "emissive.fs.glsl");
	emissiveShader.link("emissive");
}
