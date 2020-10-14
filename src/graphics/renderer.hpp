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
	glm::vec3 plPosition;
	float     _padding3;
	glm::vec3 plColor;
	float     _padding4;
};

extern const glm::vec3 SUN_DIR;
extern const glm::vec3 SUN_COLOR;

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
	
	void endMainPass(const glm::vec3& vignetteColor, const glm::vec3& colorScale);
}
