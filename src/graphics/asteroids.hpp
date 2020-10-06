#pragma once

constexpr float ASTEROID_BOX_SIZE = 4000;

struct AsteroidVariant {
	uint32_t firstLodFirstVertex;
	float size;
};

void initializeAsteroids();

void prepareAsteroids(const glm::vec3& cameraPos, const glm::vec4 frustumPlanes[6], const glm::vec4 frustumPlanesShadow[4]);

void drawAsteroidsShadow();

void drawAsteroids(bool wireframe);
