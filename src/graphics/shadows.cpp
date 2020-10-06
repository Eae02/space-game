#include "shadows.hpp"
#include "../utils.hpp"

GLuint shadowMap;

static GLuint shadowMapFbo;

static constexpr uint32_t SHADOW_MAP_RES = 2048;

void initializeShadowMapping() {
	glCreateTextures(GL_TEXTURE_2D, 1, &shadowMap);
	glTextureStorage2D(shadowMap, 1, GL_DEPTH_COMPONENT32F, SHADOW_MAP_RES, SHADOW_MAP_RES);
	
	glTextureParameteri(shadowMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(shadowMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(shadowMap, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	glTextureParameteri(shadowMap, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTextureParameteri(shadowMap, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(shadowMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(shadowMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glCreateFramebuffers(1, &shadowMapFbo);
	glNamedFramebufferTexture(shadowMapFbo, GL_DEPTH_ATTACHMENT, shadowMap, 0);
}

static constexpr float SHADOW_DIST = 20;

glm::mat4 calculateShadowMapMatrix(const glm::mat4& vpMatrixInv, const glm::vec3& sunDir) {
	glm::vec3 forward = glm::normalize(glm::vec3(vpMatrixInv * glm::vec4(0, 0, -1, 0)));
	
	glm::vec3 corners[8];
	unprojectFrustumCorners(vpMatrixInv, corners);
	
	for (int i = 0; i < 4; i++) {
		corners[i + 4] = glm::mix(corners[i], corners[i + 4], SHADOW_DIST / (Z_FAR - Z_NEAR));
	}
	
	glm::vec3 xdir = forward;//glm::normalize(glm::cross(sunDir, glm::vec3(0, 1, 0)));
	glm::vec3 ydir = glm::normalize(glm::cross(xdir, sunDir));
	glm::mat3 shadowRotationInv(xdir, ydir, sunDir);
	glm::mat3 shadowRotation = glm::transpose(shadowRotationInv);
	
	glm::vec3 minEdge(INFINITY);
	glm::vec3 maxEdge(-INFINITY);
	for (const glm::vec3& corner : corners) {
		glm::vec3 cornerRotated = shadowRotation * corner;
		minEdge = glm::min(minEdge, cornerRotated);
		maxEdge = glm::max(maxEdge, cornerRotated);
	}
	
	glm::vec3 shadowTranslate = -(minEdge + maxEdge) / 2.0f;
	glm::vec3 shadowScale = glm::vec3(0.5f) / (maxEdge - minEdge);
	
	return 
		glm::scale(glm::mat4(1), shadowScale) *
		glm::translate(glm::mat4(1), shadowTranslate) *
		glm::mat4(shadowRotation);
}

void beginRenderShadows() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFbo);
	glViewport(0, 0, SHADOW_MAP_RES, SHADOW_MAP_RES);
	const float clearValue = 1;
	glClearBufferfv(GL_DEPTH, 0, &clearValue);
	
	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
}
