#include "model.hpp"
#include "../utils.hpp"

#include <tiny_obj_loader.h>
#include <iostream>

static_assert(sizeof(Vertex) == sizeof(float) * 7);

void Model::initializeVao() {
	glCreateVertexArrays(1, &vao);
	
	for (GLuint i = 0; i < 4; i++) {
		glEnableVertexArrayAttrib(vao, i);
		glVertexArrayAttribBinding(vao, i, 0);
	}
	
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
	glVertexArrayAttribFormat(vao, 1, 3, GL_BYTE, GL_TRUE, offsetof(Vertex, normal));
	glVertexArrayAttribFormat(vao, 2, 3, GL_BYTE, GL_TRUE, offsetof(Vertex, tangent));
	glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
}

void Model::initialize(std::span<Vertex> vertices, std::span<uint32_t> indices) {
	glCreateBuffers(1, &vertexBuffer);
	glNamedBufferStorage(vertexBuffer, vertices.size_bytes(), vertices.data(), 0);
	
	glCreateBuffers(1, &indexBuffer);
	glNamedBufferStorage(indexBuffer, indices.size_bytes(), indices.data(), 0);
}

void Model::destroy() {
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &indexBuffer);
}

void generateTangents(std::span<Vertex> vertices, std::span<const glm::vec3> normals, std::span<const uint32_t> indices) {
	glm::vec3* tangents1 = (glm::vec3*)std::calloc(1, vertices.size() * sizeof(glm::vec3));
	glm::vec3* tangents2 = (glm::vec3*)std::calloc(1, vertices.size() * sizeof(glm::vec3));
	for (size_t i = 0; i < indices.size(); i += 3) {
		const glm::vec3 dp0 = vertices[indices[i + 1]].pos - vertices[indices[i]].pos;
		const glm::vec3 dp1 = vertices[indices[i + 2]].pos - vertices[indices[i]].pos;
		const glm::vec2 dtc0 = vertices[indices[i + 1]].texcoord - vertices[indices[i]].texcoord;
		const glm::vec2 dtc1 = vertices[indices[i + 2]].texcoord - vertices[indices[i]].texcoord;
		
		const float div = dtc0.x * dtc1.y - dtc1.x * dtc0.y;
		if (std::abs(div) < 1E-6f)
			continue;
		
		const float r = 1.0f / div;
		
		glm::vec3 d1((dtc1.y * dp0.x - dtc0.y * dp1.x) * r, (dtc1.y * dp0.y - dtc0.y * dp1.y) * r, (dtc1.y * dp0.z - dtc0.y * dp1.z) * r);
		glm::vec3 d2((dtc0.x * dp1.x - dtc1.x * dp0.x) * r, (dtc0.x * dp1.y - dtc1.x * dp0.y) * r, (dtc0.x * dp1.z - dtc1.x * dp0.z) * r);
		
		for (size_t j = i; j < i + 3; j++) {
			tangents1[indices[j]] += d1;
			tangents2[indices[j]] += d2;
		}
	}
	
	for (size_t v = 0; v < vertices.size(); v++) {
		if (glm::length2(tangents1[v]) > 1E-6f) {
			tangents1[v] -= normals[v] * glm::dot(normals[v], tangents1[v]);
			tangents1[v] = glm::normalize(tangents1[v]);
			if (glm::dot(glm::cross(normals[v], tangents1[v]), tangents2[v]) < 0.0f) {
				tangents1[v] = -tangents1[v];
			}
		}
		vertices[v].tangent = packVec3(tangents1[v]);
	}
	
	std::free(tangents1);
	std::free(tangents2);
}

void Model::loadObj(const std::string& path) {
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string errorString;
	if (!tinyobj::LoadObj(shapes, materials, errorString, path.c_str(), nullptr, tinyobj::triangulation)) {
		std::cerr << "error loading obj: " << errorString << std::endl;
		std::abort();
	}
	
	assert(shapes.size() < MAX_MESHES);
	
	std::vector<Vertex> vertices;
	std::vector<glm::vec3> normals;
	std::vector<uint32_t> indices;
	
	for (const tinyobj::shape_t& shape : shapes) {
		Mesh& mesh = meshes[numMeshes++];
		mesh.name = std::move(shape.name);
		mesh.firstIndex = indices.size();
		mesh.firstVertex = vertices.size();
		mesh.numIndices = shape.mesh.indices.size();
		
		size_t underscorePos = mesh.name.rfind('_');
		if (underscorePos != std::string::npos) {
			mesh.name = mesh.name.substr(underscorePos + 1);
		}
		
		indices.insert(indices.end(), shape.mesh.indices.begin(), shape.mesh.indices.end());
		
		normals.clear();
		
		assert(shape.mesh.positions.size() % 3 == 0);
		assert(shape.mesh.positions.size() == shape.mesh.normals.size());
		assert(shape.mesh.positions.size() / 3 == shape.mesh.texcoords.size() / 2);
		
		for (size_t i = 0; i * 3 < shape.mesh.positions.size(); i++) {
			const glm::vec3 normal = glm::normalize(glm::vec3(
				shape.mesh.normals[i * 3 + 0],
				shape.mesh.normals[i * 3 + 1],
				shape.mesh.normals[i * 3 + 2]
			));
			
			Vertex& vertex = vertices.emplace_back();
			vertex.pos.x = shape.mesh.positions[i * 3 + 0];
			vertex.pos.y = shape.mesh.positions[i * 3 + 1];
			vertex.pos.z = shape.mesh.positions[i * 3 + 2];
			vertex.normal = packVec3(normal);
			vertex.texcoord.x = shape.mesh.texcoords[i * 2 + 0];
			vertex.texcoord.y = shape.mesh.texcoords[i * 2 + 1];
			
			normals.push_back(normal);
		}
		
		generateTangents(
			std::span<Vertex>(&vertices[mesh.firstVertex], vertices.size() - mesh.firstVertex),
			normals, shape.mesh.indices);
	}
	
#ifdef DEBUG
	std::cout << path << " mesh names: ";
	for (uint32_t i = 0; i < numMeshes; i++) {
		if (i) std::cout << ", ";
		std::cout << "'" << meshes[i].name << "'";
	}
	std::cout << std::endl;
#endif
	
	initialize(vertices, indices);
}

uint32_t Model::findMesh(std::string_view name) const {
	for (uint32_t i = 0; i < numMeshes; i++) {
		if (meshes[i].name == name) {
			return i;
		}
	}
	std::abort();
}

void Model::bind() const {
	glBindVertexBuffer(0, vertexBuffer, 0, sizeof(Vertex));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
}

void Model::drawMesh(uint32_t meshIndex) const {
	glDrawElementsBaseVertex(
		GL_TRIANGLES,
		meshes[meshIndex].numIndices,
		GL_UNSIGNED_INT,
		(void*)(meshes[meshIndex].firstIndex * sizeof(uint32_t)),
		meshes[meshIndex].firstVertex
	);
}

void Model::drawAllMeshes() const {
	for (uint32_t i = 0; i < numMeshes; i++) {
		drawMesh(i);
	}
}
