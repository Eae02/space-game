#pragma once

#include <span>

#include "texture.hpp"

struct Rect {
	glm::vec2 min;
	glm::vec2 max;
	
	glm::vec2 size() const { return max - min; }
};

struct Sprite {
	Rect srcRect;
	Rect dstRect;
	glm::vec4 color { 1.0f };
};

struct SpriteBuffer {
	GLuint buffer;
	struct SpriteInstance* bufferMemory;
	const Texture* texture;
	uint32_t maxSprites;
	uint32_t numSprites;
	
	void initialize(uint32_t _maxSprites, const struct Texture& _texture);
	
	void beginBuild();
	void addSprite(const glm::vec2& pos, const Rect* srcRect = nullptr, const glm::vec4& color = glm::vec4(1));
	void addSprite(const Rect& dstRect, const Rect* srcRect = nullptr, const glm::vec4& color = glm::vec4(1));
	void endBuild();
	
	void draw() const;
};

struct FontChar {
	int textureX;
	int textureY;
	int width;
	int height;
	int xOffset;
	int yOffset;
	int xAdvance;
};

struct Font {
	Texture texture;
	FontChar chars[128];
	
	void loadFnt(const std::string& fntPath, const std::string& imgPath);
	
	int stringWidth(std::string_view string) const;
	
	void drawString(std::string_view string, const glm::vec2& pos, const glm::vec4& color, SpriteBuffer& buffer) const;
};

void initializeUI();

void beginDrawUI();
