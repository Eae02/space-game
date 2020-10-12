#include "ui.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "../utils.hpp"

#include <fstream>
#include <GL/glew.h>

static Shader uiShader;

static GLuint spriteVertexBuffer;
static GLuint spriteVertexArray;

static float pixelSizeX;
static float pixelSizeY;

struct SpriteInstance {
	glm::vec4 srcRect;
	glm::vec4 dstRect;
	uint32_t color;
};

void initializeUI() {
	uiShader.attachStage(GL_VERTEX_SHADER, "ui.vs.glsl");
	uiShader.attachStage(GL_FRAGMENT_SHADER, "ui.fs.glsl");
	uiShader.link("ui");
	
	const uint8_t spriteVertices[] = { 0, 0, 0, 1, 1, 0, 1, 1 };
	glCreateBuffers(1, &spriteVertexBuffer);
	glNamedBufferStorage(spriteVertexBuffer, sizeof(spriteVertices), spriteVertices, 0);
	
	glCreateVertexArrays(1, &spriteVertexArray);
	for (int i = 0; i < 4; i++) {
		glEnableVertexArrayAttrib(spriteVertexArray, i);
		glVertexArrayAttribBinding(spriteVertexArray, i, i ? 1 : 0);
	}
	glVertexArrayAttribFormat(spriteVertexArray, 0, 2, GL_UNSIGNED_BYTE, false, 0);
	glVertexArrayAttribFormat(spriteVertexArray, 1, 4, GL_FLOAT, false, offsetof(SpriteInstance, srcRect));
	glVertexArrayAttribFormat(spriteVertexArray, 2, 4, GL_FLOAT, false, offsetof(SpriteInstance, dstRect));
	glVertexArrayAttribFormat(spriteVertexArray, 3, 4, GL_UNSIGNED_BYTE, true, offsetof(SpriteInstance, color));
	glVertexArrayBindingDivisor(spriteVertexArray, 1, 1);
	glVertexArrayVertexBuffer(spriteVertexArray, 0, spriteVertexBuffer, 0, 2);
}

void beginDrawUI(uint32_t fbWidth, uint32_t fbHeight) {
	uiShader.use();
	glBindVertexArray(spriteVertexArray);
	pixelSizeX = 2.0f / fbWidth;
	pixelSizeY = 2.0f / fbHeight;
}

void SpriteBuffer::initialize(uint32_t _maxSprites, const struct Texture& _texture) {
	glCreateBuffers(1, &buffer);
	glNamedBufferStorage(buffer, _maxSprites * sizeof(SpriteInstance), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
	bufferMemory = (SpriteInstance*)glMapNamedBufferRange(buffer, 0, _maxSprites * sizeof(SpriteInstance),
		GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	maxSprites = _maxSprites;
	numSprites = 0;
	texture = &_texture;
}

void SpriteBuffer::beginBuild() {
	numSprites = 0;
}

void SpriteBuffer::addSprite(const glm::vec2& pos, const Rect* srcRect, const glm::vec4& color) {
	Rect dstRect;
	dstRect.min = pos;
	dstRect.max = pos + (srcRect ? srcRect->size() : glm::vec2(texture->width, texture->height));
	addSprite(dstRect, srcRect, color);
}

void SpriteBuffer::addSprite(const Rect& dstRect, const Rect* srcRect, const glm::vec4& color) {
	if (numSprites >= maxSprites) {
		return;
	}
	
	glm::vec2 texelSize(1.0f / texture->width, 1.0f / texture->height);
	
	bufferMemory[numSprites].dstRect = { dstRect.min * texelSize, dstRect.max * texelSize };
	
	if (srcRect) {
		bufferMemory[numSprites].srcRect = { srcRect->min, srcRect->max };
	} else {
		bufferMemory[numSprites].dstRect = { glm::vec2(0), glm::vec2(1) };
	}
	
	bufferMemory[numSprites].color = packVectorU(color);
	
	numSprites++;
}

void SpriteBuffer::endBuild() {
	glFlushMappedNamedBufferRange(buffer, 0, sizeof(SpriteInstance) * numSprites);
}

void SpriteBuffer::draw() const {
	if (numSprites != 0) {
		texture->bind(0);
		glBindVertexBuffer(1, buffer, 0, sizeof(SpriteInstance));
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, numSprites);
	}
}

void Font::loadFnt(const std::string& fntPath, const std::string& imgPath) {
	std::ifstream stream(fntPath);
	if (stream) {
		std::cerr << "error opening font file '" << fntPath << "'" << std::endl;
		std::abort();
	}
	
	std::string line;
	while (std::getline(stream, line)) {
		if (!line.starts_with("char"))
			continue;
		
		auto getParam = [&] (std::string_view nameWithEq) -> int {
			size_t pos = line.find(nameWithEq);
			assert(pos != std::string::npos);
			return atoi(&line[pos + nameWithEq.size()]);
		};
		
		const int id = getParam("id=");
		chars[id].textureX = getParam("x=");
		chars[id].textureY = getParam("y=");
		chars[id].width = getParam("width=");
		chars[id].height = getParam("height=");
		chars[id].xOffset = getParam("xoffset=");
		chars[id].yOffset = getParam("yoffset=");
		chars[id].xAdvance = getParam("xadvance=");
	}
}

int Font::stringWidth(std::string_view string) const {
	int w = 0;
	for (char c : string) {
		w += chars[(size_t)c].xAdvance;
	}
	return w;
}

void Font::drawString(std::string_view string, const glm::vec2& pos, const glm::vec4& color, SpriteBuffer& buffer) const {
	float xOffset = pos.x;
	for (char c : string) {
		size_t idx = (size_t)c;
		
		Rect dstRect;
		dstRect.min.x = xOffset + chars[idx].xOffset;
		dstRect.min.y = pos.y - (0 - chars[idx].yOffset + chars[idx].height);
		dstRect.max = dstRect.min + glm::vec2(chars[idx].width, chars[idx].height);
		
		Rect srcRect;
		srcRect.min = { chars[idx].textureX, chars[idx].textureY };
		srcRect.max = srcRect.min + glm::vec2(chars[idx].width, chars[idx].height);
		
		buffer.addSprite(dstRect, &srcRect, color);
		
		xOffset += chars[idx].xAdvance;
	}
}
