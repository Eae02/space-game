#pragma once

struct RenderSettings {
	glm::mat4 vpMatrix;
	glm::mat4 vpMatrixInverse;
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
	
	void beginGeometryPass();
	
	void beginLightPass();
	
	void renderPointLight();
	
	void beginEmissive();
	
	void endLightPass();
}
