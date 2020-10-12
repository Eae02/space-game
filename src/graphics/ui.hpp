#pragma once

#include <span>

#include "texture.hpp"

struct Rect {
	glm::vec2 min;
	glm::vec2 size;
};

namespace ui {
	constexpr float FONT_SIZE = 20;
	
	void initialize();
	void begin(uint32_t fbWidth, uint32_t fbHeight);
	void end();
	
	void drawSprite(const glm::vec2& pos, const Rect& srcRect, const glm::vec4& color = glm::vec4(1));
	
	void drawText(std::string_view string, const glm::vec2& pos, const glm::vec4& color = glm::vec4(1));
	
	int textWidth(std::string_view text);
}