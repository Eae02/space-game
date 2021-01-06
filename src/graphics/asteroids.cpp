#include "asteroids.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "shadows.hpp"
#include "sphere.hpp"
#include "collision_debug.hpp"
#include "../settings.hpp"
#include "../resources.hpp"
#include "../utils.hpp"

#include <chrono>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <noise/noise.h>

static_assert(NUM_SPHERE_LODS >= ASTEROID_NUM_LOD_LEVELS);

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
			vertices[i].normal = packVectorS(glm::normalize(normals[i]));
		}
	}
	std::free(normals);
}

constexpr uint32_t COLLISION_LOD = 4;
static_assert(COLLISION_LOD < ASTEROID_NUM_LOD_LEVELS);

AsteroidVariant generateSingleAsteroidVariant(std::mt19937& rng, std::vector<AsteroidVertex>& vertices) {
	float innerRadius = std::uniform_real_distribution<float>(0.4f, 0.5f)(rng);
	
	AsteroidVariant variant;
	variant.firstLodFirstVertex = vertices.size();
	
	noise::module::Perlin mainNoise;
	mainNoise.SetOctaveCount(3);
	mainNoise.SetPersistence(0.5f);
	mainNoise.SetLacunarity(1.5f);
	mainNoise.SetFrequency(0.02f);
	mainNoise.SetSeed(rng());
	
	noise::module::RidgedMulti ridgeNoise;
	ridgeNoise.SetOctaveCount(6);
	ridgeNoise.SetLacunarity(1.5f);
	ridgeNoise.SetFrequency(0.04f);
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
			float noiseValue = (float)mainNoise.GetValue(scaledVertex.x, scaledVertex.y, scaledVertex.z) * 0.5f + 0.5f;
			float ridgeNoiseValue = (float)ridgeNoise.GetValue(scaledVertex.x, scaledVertex.y, scaledVertex.z) * 0.5f + 0.5f;
			float radius = glm::mix(innerRadius, 1.0f, noiseValue) * glm::mix(0.8f, 1.0f, ridgeNoiseValue);
			glm::vec3 pos = scaledVertex * radius;
			vertices.push_back(AsteroidVertex { pos, 0 });
			if (lod == COLLISION_LOD) {
				variant.collisionVertices.push_back(pos);
			}
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

uint32_t numAsteroids = 0;

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
		asteroidComputeShader.findUniform("distancePerLod"), (float)settings::lodDist);
	glProgramUniform1f(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("wrappingModulo"), ASTEROID_BOX_SIZE);
	glProgramUniform1ui(asteroidComputeShader.program,
		asteroidComputeShader.findUniform("numAsteroids"), numAsteroids);
	
	uniformLocs.wrappingOffset = asteroidComputeShader.findUniform("wrappingOffset");
	uniformLocs.globalOffset = asteroidComputeShader.findUniform("globalOffset");
	uniformLocs.frustumPlanes = asteroidComputeShader.findUniform("frustumPlanes");
	uniformLocs.frustumPlanesShadow = asteroidComputeShader.findUniform("frustumPlanesShadow");
}

struct AsteroidInstance {
	glm::vec3 pos;
	glm::vec3 rotationAxis;
	float initialRotation;
	float rotationSpeed;
	float radius;
	uint32_t variant;
};

std::vector<AsteroidInstance> asteroids;

constexpr float ASTEROIDS_CELL_SIZE = 50;
constexpr int ASTEROIDS_GRID_SIZE = ASTEROID_BOX_SIZE / ASTEROIDS_CELL_SIZE;
std::vector<uint32_t> asteroidGrid[ASTEROIDS_GRID_SIZE][ASTEROIDS_GRID_SIZE][ASTEROIDS_GRID_SIZE];

std::vector<std::pair<glm::vec3, uint32_t>> generateAsteroids(uint32_t seed);

void initializeAsteroids() {
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
	asteroids.resize(generatedAsteroids.size());
	for (size_t i = 0; i < generatedAsteroids.size(); i++) {
		glm::vec3 rotationAxis = randomDirection(rng);
		
		auto [pos, variant] = generatedAsteroids[i];
		AsteroidSettings& st = asteroidSettings[i];
		st.firstVertex = asteroidVariants[variant].firstLodFirstVertex;
		st.scale = asteroidVariants[variant].size;
		st.initialRotation = std::uniform_real_distribution<float>(0, (float)M_PI * 2)(rng);
		st.rotationSpeed = std::uniform_real_distribution<float>(0.1f, 0.4f)(rng);
		st.rotationAxis = glm::packSnorm4x8(glm::vec4(rotationAxis, 0.0f));
		st.pos = pos;
		
		asteroids[i].pos = pos;
		asteroids[i].radius = asteroidVariants[variant].size;
		asteroids[i].variant = variant;
		asteroids[i].initialRotation = st.initialRotation;
		asteroids[i].rotationSpeed = st.rotationSpeed;
		asteroids[i].rotationAxis = glm::normalize(glm::unpackSnorm4x8(st.rotationAxis));
		
		glm::ivec3 minCell = glm::ivec3(glm::floor((pos - asteroidVariants[variant].size) / ASTEROIDS_CELL_SIZE)) + ASTEROIDS_GRID_SIZE;
		glm::ivec3 maxCell = glm::ivec3(glm::floor((pos + asteroidVariants[variant].size) / ASTEROIDS_CELL_SIZE)) + ASTEROIDS_GRID_SIZE;
		for (int cx = minCell.x; cx <= maxCell.x; cx++) {
			for (int cy = minCell.y; cy <= maxCell.y; cy++) {
				for (int cz = minCell.z; cz <= maxCell.z; cz++) {
					asteroidGrid[cx % ASTEROIDS_GRID_SIZE][cy % ASTEROIDS_GRID_SIZE][cz % ASTEROIDS_GRID_SIZE].push_back(i);
				}
			}
		}
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

static glm::vec3 asteroidWrappingOffset;
static glm::vec3 asteroidGlobalOffset;

void clearAsteroidWrapping() {
	asteroidWrappingOffset = glm::vec3(0);
	asteroidGlobalOffset = glm::vec3(0);
}

void updateAsteroidWrapping(const glm::vec3& cameraPos) {
	glm::vec3 boxOffset = glm::floor(cameraPos / ASTEROID_BOX_SIZE) * ASTEROID_BOX_SIZE;
	glm::vec3 posInBox = cameraPos - boxOffset;
	asteroidWrappingOffset = ASTEROID_BOX_SIZE * 1.5f - posInBox;
	asteroidGlobalOffset = cameraPos - ASTEROID_BOX_SIZE / 2;
}

void prepareAsteroids(const glm::vec4 frustumPlanes[6], const std::array<glm::vec4, 4>* frustumPlanesShadow) {
	constexpr uint32_t COMPUTE_SHADER_LOCAL_SIZE_X = 64;
	
	asteroidComputeShader.use();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, asteroidsSettingsBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, asteroidsMatrixBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, asteroidsDrawDataBuffer);
	
	static_assert(sizeof(*frustumPlanesShadow) == sizeof(glm::vec4) * 4);
	
	glUniform3fv(uniformLocs.wrappingOffset, 1, (float*)&asteroidWrappingOffset);
	glUniform3fv(uniformLocs.globalOffset, 1, (float*)&asteroidGlobalOffset);
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

bool anyAsteroidIntersects(const glm::vec3& position, float sphereRadius) {
	for (const AsteroidInstance& asteroid : asteroids) {
		glm::vec3 pos = glm::mod(asteroid.pos + asteroidWrappingOffset, ASTEROID_BOX_SIZE) + asteroidGlobalOffset;
		float sphereSum = asteroid.radius + sphereRadius;
		if (glm::distance2(pos, position) < sphereSum * sphereSum) {
			return true;
		}
	}
	return false;
}

static inline glm::mat3 getAsteroidRotation(const AsteroidInstance& asteroid) {
	float rotation = asteroid.initialRotation + asteroid.rotationSpeed * gameTime;
	float sinr = sin(rotation);
	float cosr = cos(rotation);
	glm::vec3 raxis = asteroid.rotationAxis;
	glm::vec3 rx(cosr + raxis.x * raxis.x * (1 - cosr), raxis.x * raxis.y * (1 - cosr) - raxis.z * sinr, raxis.x * raxis.z * (1 - cosr) + raxis.y * sinr);
	glm::vec3 ry(raxis.y * raxis.x * (1 - cosr) + raxis.z * sinr, cosr + raxis.y * raxis.y * (1 - cosr), raxis.y * raxis.z * (1 - cosr) - raxis.x * sinr);
	glm::vec3 rz(raxis.z * raxis.x * (1 - cosr) - raxis.y * sinr, raxis.z * raxis.y * (1 - cosr) + raxis.x * sinr, cosr + raxis.z * raxis.z * (1 - cosr));
	return glm::mat3(rx, ry, rz);
}

bool anyAsteroidIntersects(const glm::vec3& rectMin, const glm::vec3& rectMax,
	const glm::mat4& boxTransform, const glm::mat4& boxTransformInv) {
	
	glm::vec3 sphereCenter(boxTransform * glm::vec4(((rectMax + rectMin) / 2.0f), 1));
	float sphereRadius = glm::distance(sphereCenter, glm::vec3(boxTransform * glm::vec4(rectMax, 1)));
	
	const glm::vec3 corners[] = { rectMin, rectMax };
	glm::vec3 cornersWorld[2][2][2];
	glm::vec3 worldMin(INFINITY);
	glm::vec3 worldMax(-INFINITY);
	for (int x = 0; x < 2; x++) {
		for (int y = 0; y < 2; y++) {
			for (int z = 0; z < 2; z++) {
				glm::vec3 worldPos(boxTransform * glm::vec4(corners[x].x, corners[y].y, corners[z].z, 1));
				worldMin = glm::min(worldMin, worldPos);
				worldMax = glm::max(worldMax, worldPos);
				cornersWorld[x][y][z] = worldPos;
			}
		}
	}
	
	if (collisionDebug::enabled) {
		const glm::vec4 playerDebugColor(0.2f, 1, 0.2f, 0.5f);
		collisionDebug::addLine(cornersWorld[0][0][0], cornersWorld[1][0][0], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][1][0], cornersWorld[1][1][0], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][0][1], cornersWorld[1][0][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][1][1], cornersWorld[1][1][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][0][0], cornersWorld[0][1][0], playerDebugColor);
		collisionDebug::addLine(cornersWorld[1][0][0], cornersWorld[1][1][0], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][0][1], cornersWorld[0][1][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[1][0][1], cornersWorld[1][1][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][0][0], cornersWorld[0][0][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[1][0][0], cornersWorld[1][0][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[0][1][0], cornersWorld[0][1][1], playerDebugColor);
		collisionDebug::addLine(cornersWorld[1][1][0], cornersWorld[1][1][1], playerDebugColor);
	}
	
	auto checkAsteroid = [&] (const AsteroidInstance& asteroid) -> bool {
		glm::vec3 pos = glm::mod(asteroid.pos + asteroidWrappingOffset, ASTEROID_BOX_SIZE) + asteroidGlobalOffset;
		
		float radSum = asteroid.radius + sphereRadius;
		if (glm::distance2(pos, sphereCenter) > radSum * radSum)
			return false;
		
		glm::mat3 rotationMatrix = getAsteroidRotation(asteroid);
		
		glm::mat4 inverseTransform = boxTransformInv * glm::translate(glm::mat4(1), pos) * glm::mat4(rotationMatrix);
		for (const glm::vec3& vertex : asteroidVariants[asteroid.variant].collisionVertices) {
			if (collisionDebug::enabled) {
				collisionDebug::addPoint(rotationMatrix * vertex + pos, glm::vec4(1, 0.2f, 0.2f, 0.5f));
			}
			
			glm::vec4 vertexBoxLocal = inverseTransform * glm::vec4(vertex, 1);
			if (vertexBoxLocal.x > rectMin.x && vertexBoxLocal.x < rectMax.x &&
					vertexBoxLocal.y > rectMin.y && vertexBoxLocal.y < rectMax.y &&
					vertexBoxLocal.z > rectMin.z && vertexBoxLocal.z < rectMax.z) {
				return true;
			}
		}
		return false;
	};
	
	glm::vec3 aboxMin = worldMin - asteroidGlobalOffset - asteroidWrappingOffset - 50.0f;
	glm::vec3 aboxMax = worldMax - asteroidGlobalOffset - asteroidWrappingOffset + 50.0f;
	glm::ivec3 cellMin(glm::floor(aboxMin / ASTEROIDS_CELL_SIZE));
	glm::ivec3 cellMax(glm::floor(aboxMax / ASTEROIDS_CELL_SIZE));
	
	static std::vector<uint32_t> lastCheckCallId;
	static uint32_t callId = 0;
	if (lastCheckCallId.empty()) {
		lastCheckCallId.resize(numAsteroids, 0);
	}
	callId++;
	
	for (int cx = cellMin.x; cx <= cellMax.x; cx++) {
		int cxm = ((cx % ASTEROIDS_GRID_SIZE) + ASTEROIDS_GRID_SIZE) % ASTEROIDS_GRID_SIZE;
		for (int cy = cellMin.y; cy <= cellMax.y; cy++) {
			int cym = ((cy % ASTEROIDS_GRID_SIZE) + ASTEROIDS_GRID_SIZE) % ASTEROIDS_GRID_SIZE;
			for (int cz = cellMin.z; cz <= cellMax.z; cz++) {
				int czm = ((cz % ASTEROIDS_GRID_SIZE) + ASTEROIDS_GRID_SIZE) % ASTEROIDS_GRID_SIZE;
				for (uint32_t asteroid : asteroidGrid[cxm][cym][czm]) {
					if (lastCheckCallId[asteroid] != callId) {
						lastCheckCallId[asteroid] = callId;
						if (checkAsteroid(asteroids[asteroid])) {
							return true;
						}
					}
				}
			}
		}
	}
	
	return false;
}
