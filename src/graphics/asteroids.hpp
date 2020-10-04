#pragma once

void initializeAsteroids();

void prepareAsteroids(const glm::vec3& cameraPos, const glm::vec4 frustumPlanes[6], const glm::vec4 frustumPlanesShadow[4]);

void drawAsteroids();
