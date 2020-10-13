#include "target.hpp"
#include "ship.hpp"
#include "utils.hpp"
#include "graphics/ui.hpp"
#include "graphics/sphere.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer.hpp"

#include <sstream>
#include <iomanip>

constexpr uint32_t TARGET_SPHERE_LOD_LEVEL = 3;
static_assert(TARGET_SPHERE_LOD_LEVEL < NUM_SPHERE_LODS);

static Shader targetShader;
static Shader targetMergeShader;

static GLuint targetVertexBuffer;
static GLuint targetIndexBuffer;
static GLuint targetVertexArray;

static Texture targetNoiseTexture;

void initializeTargetShader() {
	targetShader.attachStage(GL_VERTEX_SHADER, "target.vs.glsl");
	targetShader.attachStage(GL_FRAGMENT_SHADER, "target.fs.glsl");
	targetShader.link("target");
	
	targetMergeShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
	targetMergeShader.attachStage(GL_FRAGMENT_SHADER, "targetmerge.fs.glsl");
	targetMergeShader.link("targetMerge");
	
	glCreateBuffers(1, &targetVertexBuffer);
	glNamedBufferStorage(
		targetVertexBuffer,
		sphereVertices[TARGET_SPHERE_LOD_LEVEL].size() * sizeof(glm::vec3),
		sphereVertices[TARGET_SPHERE_LOD_LEVEL].data(),
		0
	);
	
	glCreateBuffers(1, &targetIndexBuffer);
	glNamedBufferStorage(
		targetIndexBuffer,
		sphereTriangles[TARGET_SPHERE_LOD_LEVEL].size() * sizeof(glm::uvec3),
		sphereTriangles[TARGET_SPHERE_LOD_LEVEL].data(),
		0
	);
	
	glCreateVertexArrays(1, &targetVertexArray);
	glEnableVertexArrayAttrib(targetVertexArray, 0);
	glVertexArrayAttribBinding(targetVertexArray, 0, 0);
	glVertexArrayAttribFormat(targetVertexArray, 0, 3, GL_FLOAT, false, 0);
	glVertexArrayVertexBuffer(targetVertexArray, 0, targetVertexBuffer, 0, sizeof(glm::vec3));
	glVertexArrayElementBuffer(targetVertexArray, targetIndexBuffer);
	
	glProgramUniform1f(targetShader.program, 2, TARGET_RADIUS);
	
	targetNoiseTexture.load(exeDirPath + "res/textures/targetNoise.png", false, true);
}

glm::vec4 rotations[3] = {
	glm::vec4(1, 0, 0, 0.1f),
	glm::vec4(0, 1, 0, 0.2f),
	glm::vec4(0, 0, 1, -0.1f)
};

bool shouldHideTargetInfo;
float targetInfoOpacity = 1;

void beginDrawTargets(float gameTime, float dt) {
	if (shouldHideTargetInfo) {
		targetInfoOpacity = std::max(targetInfoOpacity - dt * 8, 0.0f);
	} else {
		targetInfoOpacity = std::min(targetInfoOpacity + dt * 8, 1.0f);
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, renderer::targetsPassFbo);
	const float clearColor[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, 0, clearColor);
	const float clearDepth[] = { 1 };
	glClearBufferfv(GL_DEPTH, 0, clearDepth);
	
	glm::mat3 rotationMatrices[3];
	for (int i = 0; i < 3; i++) {
		rotationMatrices[i] = glm::mat3(glm::rotate(glm::mat4(), gameTime * rotations[i].w, glm::vec3(rotations[i])));
	}
	
	targetShader.use();
	glBindVertexArray(targetVertexArray);
	renderer::mainPassColorAttachment.bind(0);
	renderer::mainPassDepthAttachment.bind(1);
	targetNoiseTexture.bind(2);
	glUniformMatrix3fv(3, 3, false, (const float*)rotationMatrices);
	
	glDisable(GL_CULL_FACE);
}

void endDrawTargets() {
	glBindFramebuffer(GL_FRAMEBUFFER, renderer::mainPassFbo);
	
	renderer::targetsPassColorAttachment.bind(0);
	renderer::targetsPassDepthAttachment.bind(1);
	
	targetMergeShader.use();
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glDisable(GL_CULL_FACE);
}

void drawTarget(const Target& target) {
	glUniform3fv(0, 1, (const float*)&target.truePos);
	glUniform3fv(1, 1, (const float*)&target.color);
	glDrawElements(GL_TRIANGLES, sphereTriangles[TARGET_SPHERE_LOD_LEVEL].size() * 3, GL_UNSIGNED_INT, nullptr);
}

void drawTargetUI(const Target& target, const glm::mat4& viewProj, const glm::vec2& screenSize, const Ship& ship, int remTime) {
	constexpr float RING_TEX_SIZE = 38;
	constexpr float DOT_TEX_SIZE = 14;
	
	static Rect ringSrcRect = { glm::vec2(0, 256 - RING_TEX_SIZE), glm::vec2(RING_TEX_SIZE) };
	static Rect dotSrcRect = { ringSrcRect.min + glm::vec2(RING_TEX_SIZE, RING_TEX_SIZE / 2 - DOT_TEX_SIZE / 2), glm::vec2(DOT_TEX_SIZE) };
	
	glm::vec4 hPos = viewProj * glm::vec4(target.truePos, 1);
	
	glm::vec2 ndcPos = glm::vec2(hPos) / std::abs(hPos.w);
	glm::vec2 screenPos = (glm::vec2(ndcPos) * 0.5f + 0.5f) * screenSize;
	
	constexpr float MIN_DIST_FROM_EDGE = 20;
	
	float opacity = 0.6f;
	
	float dist = glm::distance(ship.pos, target.truePos);
	
	constexpr float FADE_BEGIN_DIST = 80;
	constexpr float FADE_END_DIST = 100;
	opacity *= glm::clamp((dist - FADE_BEGIN_DIST) / (FADE_END_DIST - FADE_BEGIN_DIST), 0.0f, 1.0f);
	
	glm::vec2 screenPosC = screenPos - screenSize * 0.5f;
	float edgeX = screenSize.x * 0.5f - MIN_DIST_FROM_EDGE;
	float edgeY = screenSize.y * 0.5f - MIN_DIST_FROM_EDGE;
	if (hPos.w < 0 || std::abs(screenPosC.x) > edgeX || std::abs(screenPosC.y) > edgeY) {
		glm::vec2 relPos = screenPosC / glm::vec2(edgeX, edgeY);
		relPos /= std::max(std::abs(relPos.x), std::abs(relPos.y));
		screenPosC = relPos * glm::vec2(edgeX, edgeY);
		screenPos = screenPosC + screenSize * 0.5f;
		opacity = 0.5f;
	} else {
		if (opacity == 0)
			return;
		float textOpacity = opacity * targetInfoOpacity;
		
		glm::vec4 labelColor(1, 1, 1, textOpacity * 0.9f);
		
		constexpr float TOTAL_TEXT_H = 40;
		glm::vec2 textBtmLeft(screenPos.x + RING_TEX_SIZE / 2 + 10, screenPos.y - TOTAL_TEXT_H / 2);
		
		std::ostringstream textStream;
		std::string_view distLabel;
		if (dist >= 1000) {
			textStream << std::setprecision(2) << std::fixed << (dist / 1000);
			distLabel = "km";
		} else {
			textStream << (int)std::round(dist);
			distLabel = "m";
		}
		std::string distText = textStream.str();
		ui::drawText(distText, textBtmLeft + glm::vec2(0, 20), glm::vec4(1, 1, 1, textOpacity));
		ui::drawText(distLabel, textBtmLeft + glm::vec2(ui::textWidth(distText) + 5, 20), labelColor);
		
		std::string_view etaLabel = "eta:";
		ui::drawText(etaLabel, textBtmLeft, labelColor);
		
		std::string etaText;
		int eta = std::round(glm::clamp<float>(dist / ship.forwardVel, 1, 60 * 60));
		if (eta >= 60 * 60) {
			etaText = "--:--";
		} else {
			etaText = std::to_string(eta / 60) + (eta % 60 < 10 ? ":0" : ":") + std::to_string(eta % 60);
		}
		glm::vec4 etaColor(1, 1, 1, textOpacity);
		if (remTime != -1) {
			if (eta > remTime) {
				etaColor = glm::vec4(1, 0.5f, 0.5f, textOpacity);
			} else {
				etaColor = glm::vec4(0.7f, 1, 0.7f, textOpacity);
			}
		}
		ui::drawText(etaText, textBtmLeft + glm::vec2(ui::textWidth(etaLabel) + 5, 0), etaColor);
	}
	
	ui::drawSprite(screenPos - RING_TEX_SIZE / 2, ringSrcRect, glm::vec4(1, 1, 1, opacity * targetInfoOpacity));
	ui::drawSprite(screenPos - DOT_TEX_SIZE / 2, dotSrcRect, glm::vec4(target.color, opacity));
}
