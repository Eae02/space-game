#include "texture.hpp"
#include <SDL_image.h>

static const std::array<GLenum, 5> formatsByBPP = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };

void Texture::load(const std::string& path, bool srgb, bool generateMipmaps) {
	SDL_Surface* surface = IMG_Load(path.c_str());
	if (surface == nullptr) {
		std::cerr << IMG_GetError() << std::endl;
		std::abort();
	}
	
	width = surface->w;
	height = surface->h;
	format = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	mipLevels = generateMipmaps ? ((uint32_t)log2(std::max(width, height)) + 1) : 1;
	
	initialize();
	setData(formatsByBPP.at(surface->format->BytesPerPixel), GL_UNSIGNED_BYTE, surface->pixels);
	SDL_FreeSurface(surface);
	
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
		"right.png",
		"left.png",
		"top.png",
		"bot.png",
		"front.png",
		"back.png"
	};
	
	for (int i = 0; i < 6; i++) {
		std::string path = dirPath + layerNames[i];
		SDL_Surface* surface = IMG_Load(path.c_str());
		if (surface == nullptr) {
			std::cerr << IMG_GetError() << std::endl;
			std::abort();
		}
		assert(surface->w == resolution && surface->h == resolution);
		
		glTextureSubImage3D(texture, 0, 0, 0, i, resolution, resolution, 1,
			formatsByBPP.at(surface->format->BytesPerPixel), GL_UNSIGNED_BYTE, surface->pixels);
		SDL_FreeSurface(surface);
	}
	
	glGenerateTextureMipmap(texture);
	
	return texture;
}
