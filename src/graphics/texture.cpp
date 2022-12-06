#include "texture.hpp"
#include <stb/stb_image.h>

void Texture::load(const std::string& path, bool srgb, bool generateMipmaps) {
	uint8_t* data = stbi_load(path.c_str(), (int*)&width, (int*)&height, nullptr, 4);
	if (data == nullptr) {
		std::cerr << "image failed to load: '" << path << "': " << stbi_failure_reason() << std::endl;
		std::abort();
	}
	
	format = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	mipLevels = generateMipmaps ? ((uint32_t)log2(std::max(width, height)) + 1) : 1;
	
	initialize();
	setData(GL_RGBA, GL_UNSIGNED_BYTE, data);
	free(data);
	
	if (generateMipmaps) {
		glGenerateTextureMipmap(texture);
	}
}

void Texture::initialize() {
	destroy();
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureStorage2D(texture, mipLevels, format, width, height);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_BASE_LEVEL, 0);
	initialized = true;
}

void Texture::setParamsForFramebuffer() {
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture::setData(GLenum internalFormat, GLenum type, const void* data) {
	glTextureSubImage2D(texture, 0, 0, 0, width, height, internalFormat, type, data);
}

void Texture::destroy() {
	if (initialized) {
		glDeleteTextures(1, &texture);
		initialized = false;
	}
}

void Texture::bind(int unit) const {
	glBindTextureUnit(unit, texture);
}

GLuint loadTextureCube(const std::string& dirPath, int resolution) {
	GLuint texture;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
	glTextureStorage2D(texture, (int)log2(resolution) + 1, GL_SRGB8, resolution, resolution);
	
	static std::string layerNames[] = {
		"1.jpg",
		"3.jpg",
		"5.jpg",
		"6.jpg",
		"2.jpg",
		"4.jpg"
	};
	
	for (int i = 0; i < 6; i++) {
		std::string path = dirPath + layerNames[i];
		
		int layerWidth, layerHeight;
		uint8_t* data = stbi_load(path.c_str(), (int*)&layerWidth, (int*)&layerHeight, nullptr, 4);
		if (data == nullptr) {
			std::cerr << stbi_failure_reason() << std::endl;
			std::abort();
		}
		assert(layerWidth == resolution && layerHeight == resolution);
		
		glTextureSubImage3D(texture, 0, 0, 0, i, resolution, resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
		free(data);
	}
	
	glGenerateTextureMipmap(texture);
	
	return texture;
}
