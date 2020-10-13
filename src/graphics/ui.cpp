#include "ui.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "../utils.hpp"

#include <fstream>
#include <bitset>
#include <GL/glew.h>

namespace ui {
	constexpr uint32_t MAX_SPRITES_PER_FRAME = 1024;
	
	static Shader shader;
	
	static GLuint spriteVertexBuffer;
	static GLuint spriteVertexArray;
	
	struct SpriteInstance {
		glm::vec4 srcRect;
		glm::vec4 dstRect;
		uint32_t color;
	};
	
	static GLuint spriteInstanceBuffer;
	static SpriteInstance* spriteInstanceMemory;
	
	static Texture texture;
	
	struct FontChar {
		int textureX;
		int textureY;
		int width;
		int height;
		int xOffset;
		int yOffset;
		int xAdvance;
	};
	static std::array<FontChar, 128> fontChars;
	static constexpr uint32_t DEFAULT_CHAR = '*';
	
	static inline void loadFontFile(const std::string& path) {
		std::ifstream stream(path);
		if (!stream) {
			std::cerr << "error opening font file '" << path << "'" << std::endl;
			std::abort();
		}
		
		std::bitset<fontChars.size()> seen;
		
		std::string line;
		while (std::getline(stream, line)) {
			if (!line.starts_with("char "))
				continue;
			auto getParam = [&] (std::string_view nameWithEq) -> int {
				size_t pos = line.find(nameWithEq);
				assert(pos != std::string::npos);
				return atoi(&line[pos + nameWithEq.size()]);
			};
			const size_t idx = (size_t)getParam("id=");
			fontChars.at(idx).textureX = getParam("x=");
			fontChars.at(idx).textureY = getParam("y=");
			fontChars.at(idx).width    = getParam("width=");
			fontChars.at(idx).height   = getParam("height=");
			fontChars.at(idx).xOffset  = getParam("xoffset=");
			fontChars.at(idx).yOffset  = getParam("yoffset=");
			fontChars.at(idx).xAdvance = getParam("xadvance=");
			seen.set(idx);
		}
		
		for (size_t i = 0; i < fontChars.size(); i++) {
			if (!seen[i]) {
				fontChars[i] = fontChars[DEFAULT_CHAR];
			}
		}
	}
	
	void initialize() {
		shader.attachStage(GL_VERTEX_SHADER, "ui.vs.glsl");
		shader.attachStage(GL_FRAGMENT_SHADER, "ui.fs.glsl");
		shader.link("ui");
		
		const uint8_t spriteVertices[] = { 0, 0, 0, 1, 1, 0, 1, 1 };
		glCreateBuffers(1, &spriteVertexBuffer);
		glNamedBufferStorage(spriteVertexBuffer, sizeof(spriteVertices), spriteVertices, 0);
		
		constexpr uint32_t INSTANCE_BUFFER_BYTES = MAX_SPRITES_PER_FRAME * sizeof(SpriteInstance) * renderer::frameCycleLen;
		
		glCreateBuffers(1, &spriteInstanceBuffer);
		glNamedBufferStorage(
			spriteInstanceBuffer,
			INSTANCE_BUFFER_BYTES,
			nullptr,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT
		);
		spriteInstanceMemory = (SpriteInstance*)glMapNamedBufferRange(
			spriteInstanceBuffer,
			0,
			INSTANCE_BUFFER_BYTES,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT
		);
		
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
		glVertexArrayVertexBuffer(spriteVertexArray, 1, spriteInstanceBuffer, 0, sizeof(SpriteInstance));
		
		texture.load(exeDirPath + "res/textures/ui.png", true, true);
		glProgramUniform2f(shader.program, 0, 1.0f / texture.width, 1.0f / texture.height);
		
		loadFontFile(exeDirPath + "res/font.fnt");
	}
	
	static uint32_t firstSprite;
	static uint32_t numSprites;
	
	static glm::vec2 pixelSize;
	
	void begin(uint32_t fbWidth, uint32_t fbHeight) {
		firstSprite = renderer::frameCycleIndex * MAX_SPRITES_PER_FRAME;
		pixelSize = glm::vec2(1.0f / fbWidth, 1.0f / fbHeight);
		numSprites = 0;
	}
	
	void end() {
		glFlushMappedNamedBufferRange(
			spriteInstanceBuffer,
			firstSprite * sizeof(SpriteInstance),
			numSprites * sizeof(SpriteInstance)
		);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		shader.use();
		texture.bind(0);
		glBindVertexArray(spriteVertexArray);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, numSprites, firstSprite);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
	
	void drawSprite(const glm::vec2& pos, const Rect& srcRect, const glm::vec4& color) {
		assert(numSprites < MAX_SPRITES_PER_FRAME);
		
		SpriteInstance& instanceRef = spriteInstanceMemory[firstSprite + numSprites];
		instanceRef.dstRect = { pos * pixelSize, (pos + srcRect.size) * pixelSize };
		instanceRef.srcRect.x = srcRect.min.x;
		instanceRef.srcRect.y = srcRect.min.y + srcRect.size.y;
		instanceRef.srcRect.z = srcRect.min.x + srcRect.size.x;
		instanceRef.srcRect.w = srcRect.min.y;
		instanceRef.color = packVectorU(color);
		
		numSprites++;
	}
	
	static constexpr int CHAR_PADDING = 2;
	
	void drawText(std::string_view string, const glm::vec2& pos, const glm::vec4& color) {
		float xOffset = 0;
		for (char c : string) {
			size_t idx = (size_t)c;
			if (idx >= fontChars.size())
				idx = DEFAULT_CHAR;
			
			float drawX = pos.x + (xOffset + fontChars[idx].xOffset);
			float drawY = pos.y + FONT_SIZE - fontChars[idx].yOffset - fontChars[idx].height;
			
			Rect srcRect;
			srcRect.min = { fontChars[idx].textureX, fontChars[idx].textureY };
			srcRect.size = { fontChars[idx].width, fontChars[idx].height };
			
			drawSprite({ drawX, drawY }, srcRect, color);
			
			xOffset += fontChars[idx].xAdvance - CHAR_PADDING * 2;
		}
	}
	
	int textWidth(std::string_view text) {
		int w = 0;
		for (char c : text) {
			w += fontChars.at((size_t)c).xAdvance - CHAR_PADDING * 2;
		}
		return w;
	}
}
