#pragma once

#include <random>

template <typename T, typename U>
constexpr inline auto roundToNextMul(T value, U multiple) {
	auto valModMul = value % multiple;
	return valModMul == 0 ? value : (value + multiple - valModMul);
}

inline uint32_t packVec3(const glm::vec3& vec) {
	uint32_t ret = 0;
	for (int i = 0; i < 3; i++) {
		reinterpret_cast<int8_t*>(&ret)[i] = (int8_t)glm::clamp((int)(vec[i] * 127), -127, 127);
	}
	return ret;
}

glm::vec3 randomDirection(std::mt19937& rng);

std::array<glm::vec4, 6> createFrustumPlanes(const glm::mat4& inverseViewProj);

struct PairIntIntHash {
	size_t operator()(const std::pair<int, int>& p) const {
		return (size_t)p.first | ((size_t)p.second << (size_t)32);
	}
	size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
		return (size_t)p.first | ((size_t)p.second << (size_t)32);
	}
};
