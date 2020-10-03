#include "renderer.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "../utils.hpp"

namespace renderer {
	uint32_t frameCycleIndex = 0;
	int uboAlignment = 0;
	
	static Texture mainPassColorAttachment;
	static Texture mainPassDepthAttachment;
	
	static uint32_t fbWidth = 0;
	static uint32_t fbHeight = 0;
	
	static bool framebuffersInitialized = false;
	static GLuint mainPassFbo;
	
	static GLuint renderSettingsUbo;
	static uint64_t renderSettingsOffset;
	static char* renderSettingsUboMemory;
	
	static Shader baseLightShader;
	static Shader bloomShader;
	static Shader postShader;
	
	static GLuint skyboxTexture;
	
	void initialize() {
		mainPassColorAttachment.format = GL_RGBA16F;
		mainPassDepthAttachment.format = GL_DEPTH_COMPONENT16;
		
		postShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
		postShader.attachStage(GL_FRAGMENT_SHADER, "post.fs.glsl");
		postShader.link("Post");
		
		bloomShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
		bloomShader.attachStage(GL_FRAGMENT_SHADER, "bloomblur.fs.glsl");
		bloomShader.link("Bloom");
		
		glCreateBuffers(1, &renderSettingsUbo);
		renderSettingsOffset = roundToNextMul(sizeof(RenderSettings), uboAlignment);
		const int64_t bufferLen = renderSettingsOffset * frameCycleLen;
		glNamedBufferStorage(renderSettingsUbo, bufferLen, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		renderSettingsUboMemory = (char*)glMapNamedBufferRange(renderSettingsUbo, 0, bufferLen,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		
		skyboxTexture = loadTextureCube("./res/textures/skybox/", 4096);
	}
	
	void updateFramebuffers(uint32_t width, uint32_t height) {
		if (fbWidth == width && fbHeight == height)
			return;
		
		if (framebuffersInitialized) {
			glDeleteFramebuffers(1, &mainPassFbo);
		}
		
		fbWidth = width;
		fbHeight = height;
		framebuffersInitialized = true;
		
		mainPassColorAttachment.width = width;
		mainPassColorAttachment.height = height;
		mainPassColorAttachment.initialize();
		
		mainPassDepthAttachment.width = width;
		mainPassDepthAttachment.height = height;
		mainPassDepthAttachment.initialize();
		
		glCreateFramebuffers(1, &mainPassFbo);
		glNamedFramebufferTexture(mainPassFbo, GL_COLOR_ATTACHMENT0, mainPassColorAttachment.texture, 0);
		glNamedFramebufferTexture(mainPassFbo, GL_DEPTH_ATTACHMENT, mainPassDepthAttachment.texture, 0);
		glNamedFramebufferDrawBuffer(mainPassFbo, GL_COLOR_ATTACHMENT0);
	}
	
	void updateRenderSettings(const RenderSettings& renderSettings) {
		const int64_t bufferOffset = frameCycleIndex * renderSettingsOffset;
		memcpy(renderSettingsUboMemory + bufferOffset, &renderSettings, sizeof(RenderSettings));
		glFlushMappedNamedBufferRange(renderSettingsUbo, bufferOffset, sizeof(RenderSettings));
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, renderSettingsUbo, bufferOffset, sizeof(RenderSettings));
	}
	
	void beginMainPass() {
		glBindFramebuffer(GL_FRAMEBUFFER, mainPassFbo);
		
		glDepthMask(1);
		glEnable(GL_DEPTH_TEST);
		
		const float clearColor[] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, clearColor);
		glClearBufferfv(GL_COLOR, 1, clearColor);
		
		const float clearDepth = 1;
		glClearBufferfv(GL_DEPTH, 0, &clearDepth);
	}
	
	void endMainPass() {
		glDisable(GL_DEPTH_TEST);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		postShader.use();
		mainPassColorAttachment.bind(0);
		mainPassDepthAttachment.bind(1);
		glBindTextureUnit(2, skyboxTexture);
		
		glEnable(GL_FRAMEBUFFER_SRGB);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
}
