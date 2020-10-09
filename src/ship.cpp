#include "ship.hpp"
#include "input.hpp"
#include "resources.hpp"
#include "graphics/shader.hpp"

constexpr float MAX_SPEED_LRUP = 10;
constexpr float ACCEL_TIME_LRUP = 0.5f;
constexpr float DEACCEL_TIME_LRUP = 0.5f;

constexpr float MAX_ROLL_SPEED = 1.0f;
constexpr float ACCEL_TIME_ROLL = 0.2f;
constexpr float DEACCEL_TIME_ROLL = 0.4f;

constexpr float PITCH_SPEED = 0.5f;
constexpr float YAW_SPEED = 0.4f;
constexpr float YAW_ROLL_SPEED = 0.1f;

constexpr float MOVE_LR_MAX_ROLL = 0.4f;

static inline float calculateAcceleration(float dt, float move, float& vel, float deaccelRate) {
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

static inline glm::mat4 makeRotationMatrix(float roll, float pitch) {
	return glm::rotate(glm::mat4(1), roll, glm::vec3(0, 0, 1)) * glm::rotate(glm::mat4(1), pitch, glm::vec3(1, 0, 0));
}

constexpr float MIN_SPEED = 200;
constexpr float MAX_SPEED = 500;
constexpr float FWD_ACCEL_TIME = 6;
constexpr float FWD_DEACCEL_TIME = 2;

void Ship::update(float dt, const InputState& curInput, const InputState& prevInput) {
	float accelX = calculateAcceleration(dt, (int)curInput.leftKey - (int)curInput.rightKey, vel.x, MAX_SPEED_LRUP / DEACCEL_TIME_LRUP);
	float accelY = calculateAcceleration(dt, (int)curInput.upKey - (int)curInput.downKey, vel.y, MAX_SPEED_LRUP / DEACCEL_TIME_LRUP);
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
	
	float accelRoll = calculateAcceleration(dt, (int)curInput.rollRightKey - (int)curInput.rollLeftKey, rollVelocity, MAX_ROLL_SPEED / DEACCEL_TIME_ROLL);
	rollVelocity = glm::clamp(rollVelocity + accelRoll * dt, -MAX_ROLL_SPEED, MAX_ROLL_SPEED);
	
	
	glm::vec3 pitchAxis = rotation * glm::vec3(1, 0, 0);
	rotation = glm::angleAxis(accelY * PITCH_SPEED * dt, pitchAxis) * rotation;
	
	glm::vec3 yawAxis = rotation * glm::vec3(0, 1, 0);
	rotation = glm::angleAxis(accelX * YAW_SPEED * dt, yawAxis) * rotation;
	
	glm::vec3 rollAxis = rotation * glm::vec3(0, 0, 1);
	rotation = glm::angleAxis(accelX * YAW_ROLL_SPEED * dt + rollVelocity * dt, rollAxis) * rotation;
	
	float desiredRollOffset = -vel.x * MOVE_LR_MAX_ROLL / MAX_SPEED_LRUP;
	rollOffset += (desiredRollOffset - rollOffset) * std::min(3 * dt, 1.0f);
	
	engineIntensity = glm::mix((float)curInput.moreSpeedKey, engineIntensity, std::min(0.05f * dt, 1.0f));
	
	if (curInput.moreSpeedKey || forwardVel < MIN_SPEED) {
		forwardVel = std::min(forwardVel + dt * (MAX_SPEED - MIN_SPEED) / FWD_ACCEL_TIME, MAX_SPEED);
	} else if (curInput.lessSpeedKey) {
		forwardVel = std::max(forwardVel - dt * (MAX_SPEED - MIN_SPEED) / FWD_DEACCEL_TIME, MIN_SPEED);
	}
	speed01 = std::max((forwardVel - MIN_SPEED) / (MAX_SPEED - MIN_SPEED), 0.0f);
	
	pos += rotation * (glm::vec3(0, 0, forwardVel) * dt);
	
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

static Shader modelShader, modelShaderShadow, emissiveShader;

static const glm::vec3 EMISSIVE_COLOR = glm::convertSRGBToLinear(glm::vec3(153, 196, 233) / 255.0f);

static constexpr float SHIP_SPEC_LO = 3;
static constexpr float SHIP_SPEC_HI = 20;
static constexpr float SHIP_SPEC_EXP = 100;

static constexpr float WINDOW_SPEC_LO = 3;
static constexpr float WINDOW_SPEC_HI = 20;
static constexpr float WINDOW_SPEC_EXP = 100;

constexpr float LOW_ENGINE_COLOR = 4;
constexpr float HIGH_ENGINE_COLOR = 10;

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
	float emissiveScale = glm::mix(LOW_ENGINE_COLOR, HIGH_ENGINE_COLOR, speed01);
	glm::vec3 scaledEmissive = emissiveScale * EMISSIVE_COLOR;
	glUniform3fv(1, 1, (const float*)&scaledEmissive);
	glUniformMatrix4fv(0, 1, false, (const float*)&worldMatrix);
	res::shipModel.drawMesh(res::shipModel.findMesh("BlueL"));
	res::shipModel.drawMesh(res::shipModel.findMesh("BlueS"));
}

void Ship::drawShadow(const glm::mat4& shadowMatrix) const {
	glBindVertexArray(Model::vao);
	modelShaderShadow.use();
	glm::mat4 shadowWorldMatrix = shadowMatrix * worldMatrix;
	glUniformMatrix4fv(0, 1, false, (const float*)&shadowWorldMatrix);
	res::shipModel.bind();
	res::shipModel.drawAllMeshes();
}

void Ship::initShaders() {
	modelShader.attachStage(GL_VERTEX_SHADER, "model.vs.glsl");
	modelShader.attachStage(GL_FRAGMENT_SHADER, "model.fs.glsl");
	modelShader.link("model");

	modelShaderShadow.attachStage(GL_VERTEX_SHADER, "model_shadow.vs.glsl");
	modelShaderShadow.link("model_shadow");

	emissiveShader.attachStage(GL_VERTEX_SHADER, "model.vs.glsl");
	emissiveShader.attachStage(GL_FRAGMENT_SHADER, "emissive.fs.glsl");
	emissiveShader.link("emissive");
}
