#pragma once

namespace collisionDebug {
	extern bool enabled;
	
	void addPoint(const glm::vec3& point, const glm::vec4& color);
	void addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
	
	void draw();
}
