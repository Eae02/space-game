#include "collision_debug.hpp"
#include "shader.hpp"
#include "../utils.hpp"

bool collisionDebug::enabled = false;

struct CollisionVertex {
	glm::vec3 pos;
	uint32_t color;
	
	CollisionVertex(const glm::vec3& _pos, const glm::vec4& _color)
		: pos(_pos), color(glm::packUnorm4x8(glm::convertSRGBToLinear(_color))) { }
};

static std::vector<CollisionVertex> pointVertices;
static std::vector<CollisionVertex> lineVertices;

void collisionDebug::addPoint(const glm::vec3& point, const glm::vec4& color) {
	if (enabled) {
		pointVertices.emplace_back(point, color);
	}
}

void collisionDebug::addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color) {
	if (enabled) {
		lineVertices.emplace_back(start, color);
		lineVertices.emplace_back(end, color);
	}
}

static bool initialized = false;
static Shader collisionDebugShader;
static GLuint collisionVerticesBuffer;
static size_t collisionVerticesCapacity;
static GLuint vao;

void collisionDebug::draw() {
	if (enabled) {
		if (!initialized) {
			collisionDebugShader.attachStage(GL_VERTEX_SHADER, "collision_debug.vs.glsl");
			collisionDebugShader.attachStage(GL_FRAGMENT_SHADER, "collision_debug.fs.glsl");
			collisionDebugShader.link("CollisionDebug");
			
			glCreateVertexArrays(1, &vao);
			
			for (GLuint i = 0; i < 2; i++) {
				glEnableVertexArrayAttrib(vao, i);
				glVertexArrayAttribBinding(vao, i, 0);
			}
			glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(CollisionVertex, pos));
			glVertexArrayAttribFormat(vao, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(CollisionVertex, color));
			
			glPointSize(4);
			glLineWidth(2);
			
			initialized = true;
		}
		
		if (!pointVertices.empty() || !lineVertices.empty()) {
			size_t reqCollisionVertices = roundToNextMul(pointVertices.size() + lineVertices.size(), 1024);
			if (collisionVerticesCapacity < reqCollisionVertices) {
				if (collisionVerticesCapacity > 0) {
					glDeleteBuffers(1, &collisionVerticesBuffer);
				}
				glCreateBuffers(1, &collisionVerticesBuffer);
				glNamedBufferStorage(collisionVerticesBuffer, reqCollisionVertices * sizeof(CollisionVertex),
									nullptr, GL_DYNAMIC_STORAGE_BIT);
				collisionVerticesCapacity = reqCollisionVertices;
			}
			
			glNamedBufferSubData(collisionVerticesBuffer,
				0,
				pointVertices.size() * sizeof(CollisionVertex),
				pointVertices.data()
			);
			glNamedBufferSubData(collisionVerticesBuffer,
				pointVertices.size() * sizeof(CollisionVertex),
				lineVertices.size() * sizeof(CollisionVertex),
				lineVertices.data()
			);
			
			collisionDebugShader.use();
			glBindVertexArray(vao);
			glBindVertexBuffer(0, collisionVerticesBuffer, 0, sizeof(CollisionVertex));
			
			glDrawArrays(GL_POINTS, 0, pointVertices.size());
			glDrawArrays(GL_LINES, pointVertices.size(), lineVertices.size());
		}
	}
	pointVertices.clear();
	lineVertices.clear();
}
