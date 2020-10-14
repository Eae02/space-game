#pragma once

constexpr uint32_t ASTEROID_NUM_LOD_LEVELS = 5;
constexpr uint32_t ASTEROID_NUM_VARIANTS = 50;
constexpr float ASTEROID_BOX_SIZE = 4000;

struct AsteroidVariant {
	uint32_t firstLodFirstVertex;
	float size;
	std::vector<glm::vec3> collisionVertices;
};

extern uint32_t numAsteroids;

extern AsteroidVariant asteroidVariants[ASTEROID_NUM_VARIANTS];

void initializeAsteroids();

void clearAsteroidWrapping();

void updateAsteroidWrapping(const glm::vec3& cameraPos);
void prepareAsteroids(const glm::vec4 frustumPlanes[6], const std::array<glm::vec4, 4>* frustumPlanesShadow);

void drawAsteroidsShadow(uint32_t cascade, const glm::mat4& shadowMatrix);

void drawAsteroids(bool wireframe);

bool anyAsteroidIntersects(const glm::vec3& position, float sphereRadius);

bool anyAsteroidIntersects(const glm::vec3& rectMin, const glm::vec3& rectMax,
	const glm::mat4& boxTransform, const glm::mat4& boxTransformInv);
