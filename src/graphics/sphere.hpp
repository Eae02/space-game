#pragma once

constexpr uint32_t NUM_SPHERE_LODS = 5;

extern std::vector<glm::vec3> sphereVertices[NUM_SPHERE_LODS];
extern std::vector<glm::uvec3> sphereTriangles[NUM_SPHERE_LODS];

void generateSphereMeshes();
