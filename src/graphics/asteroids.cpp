#include "asteroids.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "../resources.hpp"
#include "../utils.hpp"

#include <chrono>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <noise/noise.h>

static constexpr uint32_t NUM_LOD_LEVELS = 5;
static constexpr uint32_t NUM_VARIANTS = 32;

static std::vector<glm::vec3> sphereVertices[NUM_LOD_LEVELS];
static std::vector<glm::uvec3> sphereTriangles[NUM_LOD_LEVELS];

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
	std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, PairIntIntHash> midVertices;
	
	auto getMiddleVertex = [&] (uint32_t v1, uint32_t v2) {
		std::pair<uint32_t, uint32_t> key(std::min(v1, v2), std::max(v1, v2));
		auto mvIt = midVertices.find(key);
		if (mvIt != midVertices.end())
			return mvIt->second;
		
		const uint32_t vertexId = sphereVertices[lod].size();
		midVertices.emplace(key, vertexId);
		sphereVertices[lod].push_back(glm::normalize(sphereVertices[lod - 1][v1] + sphereVertices[lod - 1][v2]));
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

struct AsteroidVertex {
	glm::vec3 pos;
	uint32_t normal;
};

struct AsteroidVariant {
	uint32_t firstLodFirstVertex;
};

static inline void calculateNormals(std::span<AsteroidVertex> vertices, std::span<const glm::uvec3> triangles) {
	glm::vec3* normals = (glm::vec3*)std::calloc(1, vertices.size() * sizeof(glm::vec3));
	for (const glm::ivec3& triangle : triangles) {
		glm::vec3 d1 = glm::normalize(vertices[triangle.y].pos - vertices[triangle.x].pos);
		glm::vec3 d2 = glm::normalize(vertices[triangle.z].pos - vertices[triangle.x].pos);
		glm::vec3 normal = glm::normalize(glm::cross(d1, d2));
		for (int i = 0; i < 3; i++) {
			normals[triangle[i]] += normal;
		}
	}
	for (size_t i = 0; i < vertices.size(); i++) {
		if (glm::length2(normals[i]) > 1E-6f) {
			vertices[i].normal = packVec3(glm::normalize(normals[i]));
		}
	}
	std::free(normals);
}

AsteroidVariant generateSingleAsteroidVariant(std::mt19937& rng, std::vector<AsteroidVertex>& vertices) {
	float innerRadius = std::uniform_real_distribution<float>(0.7f, 0.9f)(rng);
	
	AsteroidVariant variant;
	variant.firstLodFirstVertex = vertices.size();
	
	noise::module::Perlin mainNoise;
	mainNoise.SetOctaveCount(4);
	mainNoise.SetPersistence(0.5f);
	mainNoise.SetLacunarity(2.0f);
	
	for (uint32_t lod = 0; lod < NUM_LOD_LEVELS; lod++) {
		size_t firstVertex = vertices.size();
		
		for (const glm::vec3& vertex : sphereVertices[lod]) {
			glm::vec3 scaledVertex = vertex;
			float noiseValue = (float)mainNoise.GetValue(scaledVertex.x, scaledVertex.y, scaledVertex.z);
			float radius = glm::mix(innerRadius, 1.0f, noiseValue * 0.5f + 0.5f);
			vertices.push_back(AsteroidVertex { vertex * radius, 0 });
		}
		
		calculateNormals(std::span<AsteroidVertex>(&vertices[firstVertex], vertices.size() - firstVertex), sphereTriangles[lod]);
	}
	
	return variant;
}

static AsteroidVariant asteroidVariants[NUM_VARIANTS];

static GLuint asteroidVao;
static GLuint asteroidVertexBuffer;
static GLuint asteroidIndexBuffer;

static uint32_t lodLevelFirstIndex[NUM_LOD_LEVELS];
static uint32_t lodLevelVertexOffset[NUM_LOD_LEVELS];

static Shader asteroidShader;

void initializeAsteroids() {
	sphereVertices[0] = std::vector<glm::vec3>(std::begin(baseSphereVertices), std::end(baseSphereVertices));
	for (size_t i = 0; i < std::size(baseSphereIndices); i += 3) {
		sphereTriangles[0].emplace_back(baseSphereIndices[i], baseSphereIndices[i + 1], baseSphereIndices[i + 2]);
	}
	for (uint32_t i = 1; i < NUM_LOD_LEVELS; i++) {
		generateNextSphereLod(i);
	}
	
	lodLevelVertexOffset[0] = 0;
	
	std::vector<uint16_t> asteroidIndices;
	for (uint32_t i = 0; i < NUM_LOD_LEVELS; i++) {
		if (i != 0) {
			lodLevelVertexOffset[i] = lodLevelVertexOffset[i - 1] + sphereVertices[i - 1].size();
		}
		
		lodLevelFirstIndex[i] = asteroidIndices.size();
		for (const glm::uvec3& triangle : sphereTriangles[i]) {
			for (int j = 0; j < 3; j++) {
				assert(triangle[j] <= UINT16_MAX);
				asteroidIndices.push_back(triangle[j]);
			}
		}
	}
	
	std::vector<AsteroidVertex> asteroidVertices;
	
#ifdef DEBUG
	auto startTime = std::chrono::high_resolution_clock::now();
#endif
	
	std::mt19937 rng(42);
	for (uint32_t i = 0; i < NUM_VARIANTS; i++) {
		asteroidVariants[i] = generateSingleAsteroidVariant(rng, asteroidVertices);
	}
	
#ifdef DEBUG
	auto endTime = std::chrono::high_resolution_clock::now();
	double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
	
	std::cout << "all asteroids use " << asteroidVertices.size() << " vertices and " << asteroidIndices.size() << " indices, "
		"the highest lod uses " << sphereTriangles[NUM_LOD_LEVELS - 1].size() << " triangles, "
		"variant generation took " << std::setprecision(3) << elapsed << "s" << std::endl;
#endif
	
	glCreateBuffers(1, &asteroidVertexBuffer);
	glNamedBufferStorage(asteroidVertexBuffer, asteroidVertices.size() * sizeof(AsteroidVertex), asteroidVertices.data(), 0);
	
	glCreateBuffers(1, &asteroidIndexBuffer);
	glNamedBufferStorage(asteroidIndexBuffer, asteroidIndices.size() * sizeof(uint16_t), asteroidIndices.data(), 0);
	
	glCreateVertexArrays(1, &asteroidVao);
	glEnableVertexArrayAttrib(asteroidVao, 0);
	glVertexArrayAttribFormat(asteroidVao, 0, 3, GL_FLOAT, false, offsetof(AsteroidVertex, pos));
	glVertexArrayAttribBinding(asteroidVao, 0, 0);
	glEnableVertexArrayAttrib(asteroidVao, 1);
	glVertexArrayAttribFormat(asteroidVao, 1, 3, GL_BYTE, true, offsetof(AsteroidVertex, normal));
	glVertexArrayAttribBinding(asteroidVao, 1, 0);
	
	glVertexArrayVertexBuffer(asteroidVao, 0, asteroidVertexBuffer, 0, sizeof(AsteroidVertex));
	glVertexArrayElementBuffer(asteroidVao, asteroidIndexBuffer);
	
	asteroidShader.attachStage(GL_VERTEX_SHADER, "asteroid.vs.glsl");
	asteroidShader.attachStage(GL_FRAGMENT_SHADER, "asteroid.fs.glsl");
	asteroidShader.link("asteroids");
}

void drawAsteroids() {
	glBindVertexArray(asteroidVao);
	asteroidShader.use();
	glm::mat4 transform =
		glm::translate(glm::mat4(1), glm::vec3(0, -15, 0)) *
		glm::scale(glm::mat4(1), glm::vec3(10));
		
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
	glUniformMatrix4fv(0, 1, false, (float*)&transform);
	res::asteroidAlbedo.bind(0);
	res::asteroidNormals.bind(1);
	glDrawElementsBaseVertex(GL_TRIANGLES,
		sphereTriangles[4].size() * 3,
		GL_UNSIGNED_SHORT,
		(void*)(lodLevelFirstIndex[4] * sizeof(uint16_t)),
		lodLevelVertexOffset[4]);
	
//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
