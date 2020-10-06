#pragma once

extern GLuint shadowMap;

void initializeShadowMapping();

glm::mat4 calculateShadowMapMatrix(const glm::mat4& vpMatrixInv, const glm::vec3& sunDir);

void beginRenderShadows();
