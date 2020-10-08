#include "asteroids.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "shadows.hpp"
#include "../resources.hpp"
#include "../utils.hpp"

#include <chrono>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <noise/noise.h>

static std::vector<glm::vec3> sphereVertices[ASTEROID_NUM_LOD_LEVELS];
static std::vector<glm::uvec3> sphereTriangles[ASTEROID_NUM_LOD_LEVELS];

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
	float innerRadius = std::uniform_real_distribution<float>(0.2f, 0.3f)(rng);
	
	AsteroidVariant variant;
	variant.firstLodFirstVertex = vertices.size();
	
	noise::module::Perlin mainNoise;
	mainNoise.SetOctaveCount(2);
	mainNoise.SetPersistence(0.5f);
	mainNoise.SetLacunarity(1.5f);
	mainNoise.SetFrequency(0.02f);
	mainNoise.SetSeed(rng());
	
	noise::module::Perlin ridgeNoise;
	ridgeNoise.SetOctaveCount(5);
	ridgeNoise.SetPersistence(0.5f);
	ridgeNoise.SetLacunarity(1.5f);
	ridgeNoise.SetFrequency(0.05f);
	ridgeNoise.SetSeed(rng());
	
	constexpr float MIN_SIZE = 20;
	constexpr float MAX_SIZE = 30;
	
	float size = std::uniform_real_distribution<float>(MIN_SIZE, MAX_SIZE)(rng);
	
	if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) > 0.9f)
		size *= 2;
	
	variant.size = size;
	
	for (uint32_t lod = 0; lod < ASTEROID_NUM_LOD_LEVELS; lod++) {
		size_t firstVertex = vertices.size();
		
		for (const glm::vec3& vertex : sphereVertices[lod]) {
			glm::vec3 scaledVertex = vertex * size;
			float noiseValue = (float)mainNoise.GetValue(scaledVertex.x, scaledVertex.y, scaledVertex.z);
			float ridgeNoiseValue = 1 - std::abs((float)ridgeNoise.GetValue(scaledVertex.x, scaledVertex.y, scaledVertex.z));
			float radius = glm::mix(innerRadius, 1.0f, noiseValue * 0.5f + 0.5f) * glm::mix(0.8f, 1.0f, ridgeNoiseValue);
			vertices.push_back(AsteroidVertex { scaledVertex * radius, 0 });
		}
		
		calculateNormals(std::span<AsteroidVertex>(&vertices[firstVertex], vertices.size() - firstVertex), sphereTriangles[lod]);
	}
	
	return variant;
}

struct AsteroidSettings {
	glm::vec3 pos;
	float scale;
	float initialRotation;
	float rotationSpeed;
	uint rotationAxis;
	uint firstVertex;
};

static_assert(sizeof(AsteroidSettings) == 4 * 8);

AsteroidVariant asteroidVariants[ASTEROID_NUM_VARIANTS];

static GLuint asteroidVao;
static GLuint asteroidVertexBuffer;
static GLuint asteroidIndexBuffer;

static constexpr float DISTANCE_PER_LOD = ASTEROID_BOX_SIZE / (2 * ASTEROID_NUM_LOD_LEVELS);

static GLuint asteroidsSettingsBuffer;
static GLuint asteroidsMatrixBuffer;
static GLuint asteroidsDrawDataBuffer;

static uint32_t lodLevelFirstIndex[ASTEROID_NUM_LOD_LEVELS];
static uint32_t lodLevelVertexOffset[ASTEROID_NUM_LOD_LEVELS];

static Shader asteroidComputeShader;
static Shader asteroidShader;
static Shader asteroidShadowShader;

static uint64_t bytesPerDrawDataRange;

struct {
	GLuint wrappingOffset;
	GLuint globalOffset;
	GLuint frustumPlanes;
	GLuint frustumPlanesShadow;
} uniformLocs;

static uint32_t numAsteroids = 0;

static void loadAsteroidShaders() {
	asteroidShader.attachStage(GL_VERTEX_SHADER, "asteroid.vs.glsl");
	asteroidShader.attachStage(GL_FRAGMENT_SHADER, "asteroid.fs.glsl");
	asteroidShader.link("asteroids");
	
	asteroidShadowShader.attachStage(GL_VERTEX_SHADER, "asteroid_shadow.vs.glsl");
	asteroidShadowShader.link("asteroids_shadow");
	
	asteroidComputeShader.attachStage(GL_COMPUTE_SHADER, "asteroids.cs.glsl");
	asteroidComputeShader.link("asteroids_compute");
	
	uint32_t lodNumIndices[ASTEROID_NUM_LOD_LEVELS];
	for (uint32_t i = 0; i < ASTEROID_NUM_LOD_LEVELS; i++) {
		lodNumIndices[i] = sphereTriangles[i].size() * 3;
	}
	
	glProgramUniform1uiv(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("lodVertexOffsets"), ASTEROID_NUM_LOD_LEVELS, lodLevelVertexOffset);
	glProgramUniform1uiv(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("lodFirstIndex"), ASTEROID_NUM_LOD_LEVELS, lodLevelFirstIndex);
	glProgramUniform1uiv(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("lodNumIndices"), ASTEROID_NUM_LOD_LEVELS, lodNumIndices);
	glProgramUniform1f(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("distancePerLod"), DISTANCE_PER_LOD);
	glProgramUniform1f(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("wrappingModulo"), ASTEROID_BOX_SIZE);
	glProgramUniform1ui(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("numAsteroids"), numAsteroids);
	
	uniformLocs.wrappingOffset = asteroidComputeShader.findUniform("wrappingOffset");
	uniformLocs.globalOffset = asteroidComputeShader.findUniform("globalOffset");
	uniformLocs.frustumPlanes = asteroidComputeShader.findUniform("frustumPlanes");
	uniformLocs.frustumPlanesShadow = asteroidComputeShader.findUniform("frustumPlanesShadow");
}

std::vector<std::pair<glm::vec3, uint32_t>> generateAsteroids(uint32_t seed);

void initializeAsteroids() {
	sphereVertices[0] = std::vector<glm::vec3>(std::begin(baseSphereVertices), std::end(baseSphereVertices));
	for (size_t i = 0; i < std::size(baseSphereIndices); i += 3) {
		sphereTriangles[0].emplace_back(baseSphereIndices[i], baseSphereIndices[i + 1], baseSphereIndices[i + 2]);
	}
	for (uint32_t i = 1; i < ASTEROID_NUM_LOD_LEVELS; i++) {
		generateNextSphereLod(i);
	}
	
	lodLevelVertexOffset[0] = 0;
	
	std::vector<uint16_t> asteroidIndices;
	for (uint32_t i = 0; i < ASTEROID_NUM_LOD_LEVELS; i++) {
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
	auto varGenStartTime = std::chrono::high_resolution_clock::now();
#endif
	
	std::mt19937 rng(42);
	for (uint32_t i = 0; i < ASTEROID_NUM_VARIANTS; i++) {
		asteroidVariants[i] = generateSingleAsteroidVariant(rng, asteroidVertices);
	}
	
#ifdef DEBUG
	auto varGenEndTime = std::chrono::high_resolution_clock::now();
	double varGenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(varGenEndTime - varGenStartTime).count() / 1000.0;
	
	std::cout << "all asteroids use " << asteroidVertices.size() << " vertices and " << asteroidIndices.size() << " indices, "
		"the highest lod uses " << sphereTriangles[ASTEROID_NUM_LOD_LEVELS - 1].size() << " triangles, "
		"variant generation took " << std::setprecision(3) << varGenElapsed << "s" << std::endl;
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
	
#ifdef DEBUG
	auto placeGenStartTime = std::chrono::high_resolution_clock::now();
#endif
	
	std::vector<std::pair<glm::vec3, uint32_t>> generatedAsteroids = generateAsteroids(rng());
	std::vector<AsteroidSettings> asteroidSettings(generatedAsteroids.size());
	for (size_t i = 0; i < generatedAsteroids.size(); i++) {
		auto [pos, variant] = generatedAsteroids[i];
		AsteroidSettings& st = asteroidSettings.emplace_back();
		st.firstVertex = asteroidVariants[variant].firstLodFirstVertex;
		st.scale = asteroidVariants[variant].size;
		st.initialRotation = std::uniform_real_distribution<float>(0, (float)M_PI * 2)(rng);
		st.rotationSpeed = std::uniform_real_distribution<float>(0.1f, 0.3f)(rng);
		st.rotationAxis = glm::packSnorm4x8(glm::vec4(randomDirection(rng), 0.0f));
		st.pos = pos;
	}
	
#ifdef DEBUG
	auto placeGenEndTime = std::chrono::high_resolution_clock::now();
	double placeGenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(placeGenEndTime - placeGenStartTime).count() / 1000.0;
	std::cout << "generated " << asteroidSettings.size() << " asteroids in " << placeGenElapsed << "s" << std::endl;
#endif
	
	glCreateBuffers(1, &asteroidsSettingsBuffer);
	glNamedBufferStorage(asteroidsSettingsBuffer, sizeof(AsteroidSettings) * asteroidSettings.size(), asteroidSettings.data(), 0);
	numAsteroids = asteroidSettings.size();
	
	glCreateBuffers(1, &asteroidsMatrixBuffer);
	glNamedBufferStorage(asteroidsMatrixBuffer, sizeof(glm::mat4) * numAsteroids, nullptr, 0);
	
	bytesPerDrawDataRange = 5 * sizeof(uint32_t) * numAsteroids;
	glCreateBuffers(1, &asteroidsDrawDataBuffer);
	glNamedBufferStorage(asteroidsDrawDataBuffer, bytesPerDrawDataRange * (1 + NUM_SHADOW_CASCADES), nullptr, 0);
	
	loadAsteroidShaders();
}

void prepareAsteroids(const glm::vec3& cameraPos, const glm::vec4 frustumPlanes[6], const std::array<glm::vec4, 4>* frustumPlanesShadow) {
	constexpr uint32_t COMPUTE_SHADER_LOCAL_SIZE_X = 64;
	
	asteroidComputeShader.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, asteroidsSettingsBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, asteroidsMatrixBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, asteroidsDrawDataBuffer);
	
	glm::vec3 boxOffset = glm::floor(cameraPos / ASTEROID_BOX_SIZE) * ASTEROID_BOX_SIZE;
	glm::vec3 posInBox = cameraPos - boxOffset;
	
	glm::vec3 wrappingOffset = ASTEROID_BOX_SIZE * 1.5f - posInBox;
	glm::vec3 globalOffset = cameraPos - ASTEROID_BOX_SIZE / 2;
	
	wrappingOffset.y = 0;
	globalOffset.y = 0;
	
	static_assert(sizeof(*frustumPlanesShadow) == sizeof(glm::vec4) * 4);
	
	glUniform3fv(uniformLocs.wrappingOffset, 1, (float*)&wrappingOffset);
	glUniform3fv(uniformLocs.globalOffset, 1, (float*)&globalOffset);
	glUniform4fv(uniformLocs.frustumPlanes, 6, (const float*)frustumPlanes);
	glUniform4fv(uniformLocs.frustumPlanesShadow, 4 * NUM_SHADOW_CASCADES, (const float*)frustumPlanesShadow);
	
	glDispatchCompute((numAsteroids + COMPUTE_SHADER_LOCAL_SIZE_X - 1) / COMPUTE_SHADER_LOCAL_SIZE_X, 1, 1);
}

void drawAsteroidsShadow(uint32_t cascade, const glm::mat4& shadowMatrix) {
	glBindVertexArray(asteroidVao);
	asteroidShadowShader.use();
	
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, asteroidsDrawDataBuffer);
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, asteroidsMatrixBuffer);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	
	glUniformMatrix4fv(0, 1, false, (const float*)&shadowMatrix);
	
	uintptr_t commandsOffset = bytesPerDrawDataRange * (cascade + 1);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)commandsOffset, numAsteroids, 0);
}

void drawAsteroids(bool wireframe) {
	glBindVertexArray(asteroidVao);
	asteroidShader.use();
	
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, asteroidsDrawDataBuffer);
	
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, asteroidsMatrixBuffer);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	
	res::asteroidAlbedo.bind(0);
	res::asteroidNormals.bind(1);
	
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, nullptr, numAsteroids, 0);
	
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}
