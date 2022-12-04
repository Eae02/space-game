#include "sphere.hpp"
#include "../utils.hpp"

#include <unordered_map>

std::vector<SphereVertex> sphereVertices[NUM_SPHERE_LODS];
std::vector<glm::uvec3> sphereTriangles[NUM_SPHERE_LODS];

static const glm::vec3 baseSphereVertices[] = {
	glm::vec3(0.000000, -1.000000, 0.000000),
	glm::vec3(0.723600, -0.447215, 0.525720),
	glm::vec3(-0.276385, -0.447215, 0.850640),
	glm::vec3(-0.894425, -0.447215, 0.000000),
	glm::vec3(-0.276385, -0.447215, -0.850640),
	glm::vec3(0.723600, -0.447215, -0.525720),
	glm::vec3(0.276385, 0.447215, 0.850640),
	glm::vec3(-0.723600, 0.447215, 0.525720),
	glm::vec3(-0.723600, 0.447215, -0.525720),
	glm::vec3(0.276385, 0.447215, -0.850640),
	glm::vec3(0.894425, 0.447215, 0.000000),
	glm::vec3(0.000000, 1.000000, 0.000000)
};

static const uint32_t baseSphereIndices[] = {
	0, 1, 2, 1, 0, 5, 0, 2, 3, 0, 3, 4, 0, 4, 5, 1, 5, 10, 2, 1, 6, 3, 2, 7, 4, 3, 8, 5, 4, 9, 1, 10, 6, 2, 6, 7, 3, 7, 8, 4, 8, 9, 5, 9, 10, 6, 10, 11, 7, 6, 11, 8, 7, 11, 9, 8, 11, 10, 9, 11
};

static void generateNextSphereLod(uint32_t lod) {
	sphereVertices[lod] = sphereVertices[lod - 1];
	for (SphereVertex& vertex : sphereVertices[lod])
		vertex.prevLodV1 = vertex.prevLodV2 = -1;
	std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, PairIntIntHash> midVertices;
	
	auto getMiddleVertex = [&] (uint32_t v1, uint32_t v2) {
		std::pair<uint32_t, uint32_t> key(std::min(v1, v2), std::max(v1, v2));
		auto mvIt = midVertices.find(key);
		if (mvIt != midVertices.end())
			return mvIt->second;
		
		const uint32_t vertexId = sphereVertices[lod].size();
		midVertices.emplace(key, vertexId);
		sphereVertices[lod].push_back(SphereVertex {
			glm::normalize(sphereVertices[lod - 1][v1].pos + sphereVertices[lod - 1][v2].pos),
			(int)v1, (int)v2
		});
		return vertexId;
	};
	
	for (const glm::uvec3& triangle : sphereTriangles[lod - 1]) {
		uint32_t m1 = getMiddleVertex(triangle.x, triangle.y);
		uint32_t m2 = getMiddleVertex(triangle.y, triangle.z);
		uint32_t m3 = getMiddleVertex(triangle.z, triangle.x);
		sphereTriangles[lod].push_back({ triangle.x, m1, m3 });
		sphereTriangles[lod].push_back({ m1, triangle.y, m2 });
		sphereTriangles[lod].push_back({ m3, m2, triangle.z });
		sphereTriangles[lod].push_back({ m1, m2, m3 });
	}
}

void generateSphereMeshes() {
	sphereVertices[0].resize(std::size(baseSphereVertices));
	for (size_t i = 0; i < std::size(baseSphereVertices); i++) {
		sphereVertices[0][i].pos = baseSphereVertices[i];
	}
	
	for (size_t i = 0; i < std::size(baseSphereIndices); i += 3) {
		sphereTriangles[0].emplace_back(baseSphereIndices[i], baseSphereIndices[i + 1], baseSphereIndices[i + 2]);
	}
	for (uint32_t i = 1; i < NUM_SPHERE_LODS; i++) {
		generateNextSphereLod(i);
	}
}
