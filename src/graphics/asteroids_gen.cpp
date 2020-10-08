#include "asteroids.hpp"
#include "../utils.hpp"

#include <span>
#include <random>

static constexpr float SPACING = 20;

struct ActiveSetEntry {
	glm::vec3 pos;
	float radius;
	uint32_t variant;
	int remAttempts;
};

std::vector<std::pair<glm::vec3, uint32_t>> generateAsteroids(uint32_t seed) {
	std::vector<ActiveSetEntry> activeSet;
	
	std::mt19937 rng(seed);
	
	std::uniform_int_distribution<int> variantDist(0, (int)ASTEROID_NUM_VARIANTS - 1);
	
	constexpr int CELL_SIZE = 10;
	constexpr int NUM_CELLS = ASTEROID_BOX_SIZE / CELL_SIZE;
	constexpr int NUM_CELLS_Y = ASTEROID_BOX_HEIGHT / CELL_SIZE;
	struct CellData {
		int data[NUM_CELLS + 1][NUM_CELLS_Y + 1][NUM_CELLS + 1];
	};
	CellData* cellData = new CellData;
	memset(cellData->data, -1, sizeof(CellData));
	
	auto iterateAround = [&] (const glm::vec3& pos, float radius, const auto& callback) {
		glm::ivec3 loCheckCell(glm::floor((pos - radius) / (float)CELL_SIZE));
		glm::ivec3 hiCheckCell(glm::ceil((pos + radius) / (float)CELL_SIZE));
		loCheckCell = glm::clamp(loCheckCell, glm::ivec3(0), glm::ivec3(NUM_CELLS, NUM_CELLS_Y, NUM_CELLS));
		hiCheckCell = glm::clamp(hiCheckCell, glm::ivec3(0), glm::ivec3(NUM_CELLS, NUM_CELLS_Y, NUM_CELLS));
		for (int x = loCheckCell.x; x <= hiCheckCell.x; x++) {
			for (int y = loCheckCell.y; y <= hiCheckCell.y; y++) {
				for (int z = loCheckCell.z; z <= hiCheckCell.z; z++) {
					if (!callback(x, y, z)) return false;
				}
			}
		}
		return true;
	};
	
	std::vector<std::pair<glm::vec3, uint32_t>> asteroids;
	auto addAsteroid = [&] (const glm::vec3& pos, uint32_t variant) -> bool {
		if (pos.x < 0 || pos.y < 0 || pos.z < 0 || pos.x >= ASTEROID_BOX_SIZE || pos.y >= ASTEROID_BOX_HEIGHT || pos.z >= ASTEROID_BOX_SIZE)
			return false;
		
		float thisVarRadius = asteroidVariants[variant].size;
		bool ok = iterateAround(pos, thisVarRadius + CELL_SIZE, [&] (int x, int y, int z) {
			if (cellData->data[x][y][z] != -1) {
				auto [otherPos, otherVariant] = asteroids[cellData->data[x][y][z]];
				float maxRad = asteroidVariants[otherVariant].size + thisVarRadius + SPACING;
				if (glm::distance2(pos, otherPos) < maxRad * maxRad) {
					return false;
				}
			}
			return true;
		});
		if (!ok)
			return false;
		
		float radiusAndSpacing = thisVarRadius + SPACING;
		float radiusAndSpacingSq = radiusAndSpacing * radiusAndSpacing;
		iterateAround(pos, radiusAndSpacing, [&] (int x, int y, int z) {
			if (glm::distance2(glm::vec3(x, y, z) * (float)CELL_SIZE, pos) <= radiusAndSpacingSq) {
				cellData->data[x][y][z] = asteroids.size();
			}
			return true;
		});
		
		constexpr int MAX_ATTEMPTS = 8;
		activeSet.push_back({ pos, asteroidVariants[variant].size, variant, MAX_ATTEMPTS });
		asteroids.emplace_back(pos, variant);
		
		return true;
	};
	
	uint32_t firstVariant = variantDist(rng);
	float firstRadius = asteroidVariants[firstVariant].size;
	std::uniform_real_distribution<float> startDist(firstRadius, ASTEROID_BOX_SIZE - firstRadius);
	addAsteroid(glm::vec3(startDist(rng), startDist(rng) * (ASTEROID_BOX_HEIGHT / ASTEROID_BOX_SIZE), startDist(rng)), firstVariant);
	
	while (!activeSet.empty()) {
		uint32_t variant = variantDist(rng);
		size_t idx = std::uniform_int_distribution<size_t>(0, activeSet.size() - 1)(rng);
		glm::vec3 pos = activeSet.at(idx).pos + randomDirection(rng) * (activeSet.at(idx).radius + SPACING + asteroidVariants[variant].size);
		addAsteroid(pos, variant);
		
		activeSet[idx].remAttempts--;
		if (activeSet[idx].remAttempts == 0) {
			activeSet[idx] = activeSet.back();
			activeSet.pop_back();
		}
	}
	
	delete cellData;
	
	return asteroids;
}