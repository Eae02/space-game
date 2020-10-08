#pragma once

#include "shadows.hpp"

struct RenderSettings {
	glm::mat4 vpMatrix;
	glm::mat4 vpMatrixInverse;
	glm::mat4 shadowMatrices[NUM_SHADOW_CASCADES];
	glm::vec3 cameraPos;
	float     gameTime;
	glm::vec3 sunColor;
	float     _padding1;
	glm::vec3 sunDir;
	float     _padding2;
};

namespace renderer {
	constexpr uint32_t frameCycleLen = 4;
	extern uint32_t frameCycleIndex;
	extern int uboAlignment;
	
	void initialize();
	
	void updateFramebuffers(uint32_t width, uint32_t height);
	
	void updateRenderSettings(const RenderSettings& renderSettings);
	
	void beginMainPass();
	
	extern bool enableVolumetricLighting;
	
	void endMainPass();
}
