#pragma once

struct Ship {
	void update(float dt, const struct InputState& curInput, const struct InputState& prevInput);
	
	void draw() const;
	
	static void initShaders();
	
	glm::mat4 worldMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 viewMatrixInv;
	
	bool stopped = false;
	
	glm::ivec3 boxIndex;
	glm::vec3 pos { 0, 0, 0 };
	glm::vec3 vel { 0, 0, 5 };
	
	glm::quat rotation;
	glm::quat cameraRotation;
	
	float rollVelocity = 0;
	float rollOffset = 0;
	
	float forwardVel = 0;
	float engineIntensity = 0;
	float speed01 = 0;
};
