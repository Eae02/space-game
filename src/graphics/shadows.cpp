#include "shadows.hpp"
#include "../settings.hpp"
#include "../utils.hpp"

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

GLuint shadowMap;

static GLuint shadowMapFbos[NUM_SHADOW_CASCADES];

void initializeShadowMapping() {
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &shadowMap);
	glTextureStorage3D(shadowMap, 1, GL_DEPTH_COMPONENT32F,
		settings::shadowRes, settings::shadowRes, NUM_SHADOW_CASCADES);
	
	glTextureParameteri(shadowMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(shadowMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(shadowMap, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	glTextureParameteri(shadowMap, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTextureParameteri(shadowMap, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(shadowMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(shadowMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
		glCreateFramebuffers(1, &shadowMapFbos[i]);
		glNamedFramebufferTextureLayer(shadowMapFbos[i], GL_DEPTH_ATTACHMENT, shadowMap, 0, i);
	}
}

static constexpr float CASCADE_DISTS[NUM_SHADOW_CASCADES] = { 40, 200, 500 };

ShadowMapMatrices calculateShadowMapMatrices(const glm::mat4& vpMatrixInv, const glm::vec3& sunDir) {
	glm::vec3 corners[8];
	unprojectFrustumCorners(vpMatrixInv, corners);

	glm::vec3 forward = glm::normalize(
		multiplyAndWDivide(vpMatrixInv, glm::vec3(0, 0, 1)) -
		multiplyAndWDivide(vpMatrixInv, glm::vec3(0, 0, -1))
	);
	
	glm::vec3 ydir = glm::normalize(glm::cross(forward, sunDir));
	glm::vec3 xdir = glm::normalize(glm::cross(sunDir, ydir));
	glm::mat3 shadowRotationInv(xdir, ydir, sunDir);
	glm::mat3 shadowRotation = glm::transpose(shadowRotationInv);
	
	ShadowMapMatrices matrices;
	
	float prevCascadeEndDst = 0;
	for (size_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
		glm::vec3 minEdge(INFINITY);
		glm::vec3 maxEdge(-INFINITY);
		for (int c = 0; c < 4; c++) {
			glm::vec3 farDir = (corners[c + 4] - corners[c]) / (Z_FAR - Z_NEAR);
			
			glm::vec3 near = shadowRotation * (corners[c] + farDir * prevCascadeEndDst);
			minEdge = glm::min(minEdge, near);
			maxEdge = glm::max(maxEdge, near);
			
			glm::vec3 far = shadowRotation * (corners[c] + farDir * CASCADE_DISTS[i]);
			minEdge = glm::min(minEdge, far);
			maxEdge = glm::max(maxEdge, far);
		}
		
		glm::vec3 shadowTranslate = -(minEdge + maxEdge) / 2.0f;
		glm::vec3 shadowScale = glm::vec3(0.5f, 0.5f, 0.5f) / (maxEdge - minEdge);
		
		matrices.matrices[i] = 
			glm::scale(glm::mat4(1), shadowScale) *
			glm::translate(glm::mat4(1), shadowTranslate) *
			glm::mat4(shadowRotation);
		matrices.inverseMatrices[i] = 
			glm::mat4(shadowRotationInv) *
			glm::translate(glm::mat4(1), -shadowTranslate) *
			glm::scale(glm::mat4(1), 1.0f / shadowScale);
		
		auto frustumPlanes6 = createFrustumPlanes(matrices.inverseMatrices[i]);
		std::copy_n(frustumPlanes6.begin(), 4, matrices.frustumPlanes[i].begin());
		
		prevCascadeEndDst = CASCADE_DISTS[i];
	}
	
	return matrices;
}

void renderShadows(const std::function<void(uint32_t)>& renderCallback) {
	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	
	glCullFace(GL_FRONT);
	
	for (uint32_t cascade = 0; cascade < NUM_SHADOW_CASCADES; cascade++) {
		glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFbos[cascade]);
		glViewport(0, 0, settings::shadowRes, settings::shadowRes);
		
		const float clearValue = 1;
		glClearBufferfv(GL_DEPTH, 0, &clearValue);
		
		renderCallback(cascade);
	}
	
	glCullFace(GL_BACK);
	glDisable(GL_DEPTH_CLAMP);
}
