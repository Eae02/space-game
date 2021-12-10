#pragma once

#include "opengl.hpp"

#include <functional>

extern GLuint shadowMap;

static constexpr uint32_t NUM_SHADOW_CASCADES = 3;

void initializeShadowMapping();

struct ShadowMapMatrices {
	glm::mat4 matrices[NUM_SHADOW_CASCADES];
	glm::mat4 inverseMatrices[NUM_SHADOW_CASCADES];
	std::array<glm::vec4, 4> frustumPlanes[NUM_SHADOW_CASCADES];
};

ShadowMapMatrices calculateShadowMapMatrices(const glm::mat4& vpMatrixInv, const glm::vec3& sunDir);

void renderShadows(const std::function<void(uint32_t)>& renderCallback);
