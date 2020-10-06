#include "utils.hpp"

glm::vec3 randomDirection(std::mt19937& rng) {
	const float theta = std::uniform_real_distribution<float>(0, (float)M_PI * 2)(rng);
	const float cosPitch = std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng);
	const float sinPitch = std::sin(std::acos(cosPitch));
	return glm::vec3(std::cos(theta) * sinPitch, std::sin(theta) * sinPitch, cosPitch);
}

static inline glm::vec4 createFrustumPlane(const glm::vec3& p1, const glm::vec3& p2,
                                           const glm::vec3& p3, const glm::vec3& normalTarget)
{
	const glm::vec3 v1 = p3 - p1;
	const glm::vec3 v2 = p3 - p2;
	
	glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
	
	//Flips the normal if it doesn't face the normal target (which is the center of the frustum)
	//This is to make sure that all normals face into the frustum
	if (glm::dot(normal, normalTarget - p1) < 0.0f)
		normal = -normal;
	
	return glm::vec4(normal, -glm::dot(p1, normal));
}

void unprojectFrustumCorners(const glm::mat4& inverseViewProj, glm::vec3* cornersOut) {
	cornersOut[0] = { -1,  1, 0 };
	cornersOut[1] = {  1,  1, 0 };
	cornersOut[2] = {  1, -1, 0 };
	cornersOut[3] = { -1, -1, 0 };
	cornersOut[4] = { -1,  1, 1 };
	cornersOut[5] = {  1,  1, 1 };
	cornersOut[6] = {  1, -1, 1 };
	cornersOut[7] = { -1, -1, 1 };
	
	for (size_t i = 0; i < 8; i++) {
		glm::vec4 cornerV4 = inverseViewProj * glm::vec4(cornersOut[i], 1);
		cornersOut[i] = glm::vec3(cornerV4.x, cornerV4.y, cornerV4.z) / cornerV4.w;
	}
}

std::array<glm::vec4, 6> createFrustumPlanes(const glm::mat4& inverseViewProj) {
	glm::vec3 corners[8];
	unprojectFrustumCorners(inverseViewProj, corners);
	glm::vec3 frustumCenter;
	for (glm::vec3& corner : corners) {
		frustumCenter += corner;
	}
	
	frustumCenter /= 8;
	
	std::array<glm::vec4, 6> planes;
	planes[0] = createFrustumPlane(corners[0], corners[4], corners[7], frustumCenter); //Left
	planes[1] = createFrustumPlane(corners[2], corners[6], corners[1], frustumCenter); //Right
	planes[2] = createFrustumPlane(corners[5], corners[4], corners[0], frustumCenter); //Up
	planes[3] = createFrustumPlane(corners[7], corners[2], corners[3], frustumCenter); //Down
	planes[4] = createFrustumPlane(corners[3], corners[1], corners[0], frustumCenter); //Near
	planes[5] = createFrustumPlane(corners[4], corners[5], corners[7], frustumCenter); //Far
	
	return planes;
}
