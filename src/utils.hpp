#pragma once

#include <random>

constexpr float Z_NEAR = 0.1f;
constexpr float Z_FAR = 5000.0f;

extern float dt;
extern float gameTime;

extern glm::ivec3 maxComputeWorkGroupSize;
extern int maxComputeWorkGroupInvocations;

template <typename T, typename U>
constexpr inline auto roundToNextMul(T value, U multiple) {
	auto valModMul = value % multiple;
	return valModMul == 0 ? value : (value + multiple - valModMul);
}

template<int S>
inline uint32_t packVectorS(const glm::vec<S, float, glm::defaultp>& vec) {
	uint32_t ret = 0;
	for (int i = 0; i < S; i++) {
		reinterpret_cast<int8_t*>(&ret)[i] = (int8_t)glm::clamp((int)(vec[i] * 127), -127, 127);
	}
	return ret;
}

template<int S>
inline uint32_t packVectorU(const glm::vec<S, float, glm::defaultp>& vec) {
	uint32_t ret = 0;
	for (int i = 0; i < S; i++) {
		reinterpret_cast<uint8_t*>(&ret)[i] = (uint8_t)glm::clamp((int)(vec[i] * 256), 0, 255);
	}
	return ret;
}

inline glm::vec3 randomDirection(std::mt19937& rng) {
	const float theta = std::uniform_real_distribution<float>(0, (float)M_PI * 2)(rng);
	const float cosPitch = std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng);
	const float sinPitch = std::sin(std::acos(cosPitch));
	return glm::vec3(std::cos(theta) * sinPitch, std::sin(theta) * sinPitch, cosPitch);
}

inline glm::vec3 multiplyAndWDivide(const glm::mat4& vp, const glm::vec3& pos) {
	glm::vec4 pos4 = vp * glm::vec4(pos, 1);
	return glm::vec3(pos4) / pos4.w;
}

void unprojectFrustumCorners(const glm::mat4& inverseViewProj, glm::vec3* cornersOut);

std::array<glm::vec4, 6> createFrustumPlanes(const glm::mat4& inverseViewProj);

extern std::string exeDirPath;
void initExeDirPath();

struct PairIntIntHash {
	size_t operator()(const std::pair<int, int>& p) const {
		return (size_t)p.first | ((size_t)p.second << (size_t)32);
	}
	size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
		return (size_t)p.first | ((size_t)p.second << (size_t)32);
	}
};
