#include "ship.hpp"
#include "input.hpp"

constexpr float MAX_SPEED_LRUP = 10;
constexpr float ACCEL_TIME_LRUP = 0.5f;
constexpr float DEACCEL_TIME_LRUP = 0.5f;
constexpr float ROLL_SPEED = 0.5f;
constexpr float PITCH_SPEED = 0.5f;

constexpr float MOVE_LR_MAX_ROLL = 0.4f;

static inline float calculateAcceleration(float dt, float move, float& vel) {
	if (abs(move) < 0.01f) {
		if (vel > 0) {
			vel = std::max(vel - dt * MAX_SPEED_LRUP / DEACCEL_TIME_LRUP, 0.0f);
		} else {
			vel = std::min(vel + dt * MAX_SPEED_LRUP / DEACCEL_TIME_LRUP, 0.0f);
		}
		return 0;
	}
	return move;
}

static inline glm::mat4 makeRotationMatrix(float roll, float pitch) {
	return glm::rotate(glm::mat4(1), roll, glm::vec3(0, 0, 1)) * glm::rotate(glm::mat4(1), pitch, glm::vec3(1, 0, 0));
}

void Ship::update(float dt, const InputState& curInput, const InputState& prevInput) {
	float accelX = calculateAcceleration(dt, (int)curInput.rightKey - (int)curInput.leftKey, vel.x);
	float accelY = calculateAcceleration(dt, (int)curInput.upKey - (int)curInput.downKey, vel.y);
	float accelLen = std::hypot(accelX, accelY);
	if (accelLen > 1) {
		accelX /= accelLen;
		accelY /= accelLen;
	}
	vel.x += accelX * dt * (MAX_SPEED_LRUP / ACCEL_TIME_LRUP);
	vel.y += accelY * dt * (MAX_SPEED_LRUP / ACCEL_TIME_LRUP);
	float velLen = std::hypot(vel.x, vel.y);
	if (velLen > MAX_SPEED_LRUP) {
		vel.x *= MAX_SPEED_LRUP / velLen;
		vel.y *= MAX_SPEED_LRUP / velLen;
	}
	
	glm::vec3 pitchAxis = rotation * glm::vec3(1, 0, 0);
	rotation = glm::angleAxis(accelY * PITCH_SPEED * dt, pitchAxis) * rotation;
	
	glm::vec3 rollAxis = rotation * glm::vec3(0, 0, 1);
	rotation = glm::angleAxis(((int)curInput.rollRightKey - (int)curInput.rollLeftKey) * ROLL_SPEED * dt, rollAxis) * rotation;
	
	float desiredRollOffset = vel.x * MOVE_LR_MAX_ROLL / MAX_SPEED_LRUP;
	rollOffset += (desiredRollOffset - rollOffset) * std::min(3 * dt, 1.0f);
	
	pos += rotation * (vel * dt);
	
	worldMatrix =
		glm::translate(glm::mat4(1), pos) *
		glm::rotate(glm::mat4(1), rollOffset, rollAxis) *
		glm::mat4_cast(rotation);
	
	cameraRotation = glm::slerp(cameraRotation, rotation, std::min(1 * dt, 1.0f));
	
	viewMatrix =
		glm::lookAt(glm::vec3(0, 5, -14), glm::vec3(0, 3, 0), glm::vec3(0, 1, 0)) *
		glm::transpose(glm::mat4_cast(cameraRotation)) *
		glm::translate(glm::mat4(1), -pos);
	viewMatrixInv = glm::inverse(viewMatrix); //glm::translate(glm::mat4(1), cameraOffset) * glm::transpose(makeRotationMatrix(cameraRoll, pitch)) * glm::translate(glm::mat4(1), -pos);
}
