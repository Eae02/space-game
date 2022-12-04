#pragma once

constexpr uint32_t NUM_SPHERE_LODS = 5;

struct SphereVertex {
	glm::vec3 pos;
	int prevLodV1 = -1;
	int prevLodV2 = -1;
};

extern std::vector<SphereVertex> sphereVertices[NUM_SPHERE_LODS];
extern std::vector<glm::uvec3> sphereTriangles[NUM_SPHERE_LODS];

void generateSphereMeshes();
