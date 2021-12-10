#pragma once

#include "opengl.hpp"

struct Texture {
	uint32_t mipLevels = 1;
	uint32_t width = 0;
	uint32_t height = 0;
	GLenum format = 0;
	
	GLuint texture;
	bool initialized = false;
	
	void load(const std::string& path, bool srgb, bool generateMipmaps);
	
	void initialize();
	void setParamsForFramebuffer();
	
	void setData(GLenum internalFormat, GLenum type, const void* data);
	
	void destroy();
	
	void bind(int unit) const;
};

GLuint loadTextureCube(const std::string& dirPath, int resolution);
