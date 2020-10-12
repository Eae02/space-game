#pragma once

#include "shadows.hpp"
#include "texture.hpp"

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
	
	extern Texture mainPassColorAttachment;
	extern Texture mainPassDepthAttachment;
	extern GLuint mainPassFbo;
	
	extern Texture targetsPassColorAttachment;
	extern Texture targetsPassDepthAttachment;
	extern GLuint targetsPassFbo;
	
	void initialize();
	
	void updateFramebuffers(uint32_t width, uint32_t height);
	
	void updateRenderSettings(const RenderSettings& renderSettings);
	
	void drawSkybox();
	
	void beginMainPass();
	
	void endMainPass(const glm::mat4& prevViewProj, float dt);
}
