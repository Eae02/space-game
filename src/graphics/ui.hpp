#pragma once

#include <span>

#include "texture.hpp"

struct Rect {
	glm::vec2 min;
	glm::vec2 size;
};

struct InputState;

struct ColoredStringBuilder {
	std::string text;
	std::vector<glm::vec4> colors;
	
	void push(std::string_view newText, const glm::vec4& color);
};

namespace ui {
	constexpr float FONT_SIZE = 20;
	
	void initialize();
	void begin(uint32_t fbWidth, uint32_t fbHeight);
	void end();
	
	void drawSprite(const glm::vec2& pos, const Rect& srcRect, const glm::vec4& color = glm::vec4(1));
	
	void drawText(std::string_view string, const glm::vec2& pos, const glm::vec4& color = glm::vec4(1));
	
	void drawText(std::string_view string, const glm::vec2& pos, std::span<const glm::vec4> colors);
	
	void drawText(const ColoredStringBuilder& builder, const glm::vec2& pos);
	
	int textWidth(std::string_view text);
	
	inline void drawTextCentered(std::string_view string, const glm::vec2& pos, const glm::vec4& color = glm::vec4(1)) {
		drawText(string, pos - glm::vec2(textWidth(string) / 2, FONT_SIZE / 2), color);
	}
	
	constexpr float BUTTON_WIDTH = 180;
	constexpr float BUTTON_HEIGHT = 30;
	
	struct Button {
		std::string_view text;
		glm::vec2 pos;
		
		float hoverAnim = 0;
		
		bool operator()(const InputState& is);
	};
}
