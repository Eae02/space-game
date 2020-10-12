#pragma once

constexpr uint32_t ASTEROID_NUM_LOD_LEVELS = 5;
constexpr uint32_t ASTEROID_NUM_VARIANTS = 50;

struct AsteroidVariant {
	uint32_t firstLodFirstVertex;
	float size;
};

extern uint32_t numAsteroids;
extern float asteroidBoxSize;

extern AsteroidVariant asteroidVariants[ASTEROID_NUM_VARIANTS];

void initializeAsteroids();

void prepareAsteroids(const glm::vec3& cameraPos, const glm::vec4 frustumPlanes[6], const std::array<glm::vec4, 4>* frustumPlanesShadow);

void drawAsteroidsShadow(uint32_t cascade, const glm::mat4& shadowMatrix);

void drawAsteroids(bool wireframe);
