#pragma once

#include <span>

struct Vertex {
	glm::vec3 pos;
	uint32_t normal;
	uint32_t tangent;
	glm::vec2 texcoord;
};

struct Mesh {
	std::string name;
	uint32_t firstVertex;
	uint32_t firstIndex;
	uint32_t numIndices;
};

struct Model {
	static inline GLuint vao;
	
	static void initializeVao();
	
	static constexpr size_t MAX_MESHES = 8;
	
	GLuint vertexBuffer;
	GLuint indexBuffer;
	uint32_t numMeshes = 0;
	Mesh meshes[MAX_MESHES];
	
	void initialize(std::span<Vertex> vertices, std::span<uint32_t> indices);
	void loadObj(const std::string& path);
	
	void destroy();
	
	uint32_t findMesh(std::string_view name) const;
	
	void bind() const;
	
	void drawMesh(uint32_t meshIndex) const;
	void drawAllMeshes() const;
};

void generateTangents(std::span<Vertex> vertices, std::span<const glm::vec3> normals, std::span<const uint32_t> indices);
