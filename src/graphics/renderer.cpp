#include "renderer.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "../utils.hpp"

namespace renderer {
	uint32_t frameCycleIndex = 0;
	int uboAlignment = 0;
	
	static Texture gbufferColorAttachment1;
	static Texture gbufferColorAttachment2;
	static Texture gbufferDepthAttachment;
	static Texture lightPassAttachment;
	
	static uint32_t gbufferWidth = 0;
	static uint32_t gbufferHeight = 0;
	
	static bool framebuffersInitialized = false;
	static GLuint geometryPassFbo;
	static GLuint lightPassFbo;
	static GLuint emissivePassFbo;
	
	static GLuint renderSettingsUbo;
	static uint64_t renderSettingsOffset;
	static char* renderSettingsUboMemory;
	
	static Shader baseLightShader;
	static Shader bloomShader;
	static Shader postShader;
	
	static GLuint skyboxTexture;
	
	void initialize() {
		gbufferColorAttachment1.format = GL_RGBA8;
		gbufferColorAttachment2.format = GL_RGBA8;
		gbufferDepthAttachment.format = GL_DEPTH_COMPONENT16;
		lightPassAttachment.format = GL_RGBA16F;
		
		baseLightShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
		baseLightShader.attachStage(GL_FRAGMENT_SHADER, "baselight.fs.glsl");
		baseLightShader.link("BaseLight");
		
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
		if (gbufferWidth == width && gbufferHeight == height)
			return;
		
		if (framebuffersInitialized) {
			glDeleteFramebuffers(1, &geometryPassFbo);
			glDeleteFramebuffers(1, &lightPassFbo);
			glDeleteFramebuffers(1, &emissivePassFbo);
		}
		
		gbufferWidth = width;
		gbufferHeight = height;
		framebuffersInitialized = true;
		
		gbufferColorAttachment1.width = width;
		gbufferColorAttachment1.height = height;
		gbufferColorAttachment1.initialize();
		
		gbufferColorAttachment2.width = width;
		gbufferColorAttachment2.height = height;
		gbufferColorAttachment2.initialize();
		
		gbufferDepthAttachment.width = width;
		gbufferDepthAttachment.height = height;
		gbufferDepthAttachment.initialize();
		
		lightPassAttachment.width = width;
		lightPassAttachment.height = height;
		lightPassAttachment.initialize();
		
		glCreateFramebuffers(1, &geometryPassFbo);
		glNamedFramebufferTexture(geometryPassFbo, GL_COLOR_ATTACHMENT0, gbufferColorAttachment1.texture, 0);
		glNamedFramebufferTexture(geometryPassFbo, GL_COLOR_ATTACHMENT1, gbufferColorAttachment2.texture, 0);
		glNamedFramebufferTexture(geometryPassFbo, GL_DEPTH_ATTACHMENT, gbufferDepthAttachment.texture, 0);
		static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glNamedFramebufferDrawBuffers(geometryPassFbo, 2, drawBuffers);
		
		glCreateFramebuffers(1, &lightPassFbo);
		glNamedFramebufferTexture(lightPassFbo, GL_COLOR_ATTACHMENT0, lightPassAttachment.texture, 0);
		glNamedFramebufferDrawBuffer(lightPassFbo, GL_COLOR_ATTACHMENT0);
		
		glCreateFramebuffers(1, &emissivePassFbo);
		glNamedFramebufferTexture(emissivePassFbo, GL_COLOR_ATTACHMENT0, lightPassAttachment.texture, 0);
		glNamedFramebufferTexture(emissivePassFbo, GL_DEPTH_ATTACHMENT, gbufferDepthAttachment.texture, 0);
		glNamedFramebufferDrawBuffer(emissivePassFbo, GL_COLOR_ATTACHMENT0);
	}
	
	void updateRenderSettings(const RenderSettings& renderSettings) {
		const int64_t bufferOffset = frameCycleIndex * renderSettingsOffset;
		memcpy(renderSettingsUboMemory + bufferOffset, &renderSettings, sizeof(RenderSettings));
		glFlushMappedNamedBufferRange(renderSettingsUbo, bufferOffset, sizeof(RenderSettings));
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, renderSettingsUbo, bufferOffset, sizeof(RenderSettings));
	}
	
	void beginGeometryPass() {
		glBindFramebuffer(GL_FRAMEBUFFER, geometryPassFbo);
		
		glDepthMask(1);
		glEnable(GL_DEPTH_TEST);
		
		const float clearColor[] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, clearColor);
		glClearBufferfv(GL_COLOR, 1, clearColor);
		
		const float clearDepth = 1;
		glClearBufferfv(GL_DEPTH, 0, &clearDepth);
	}
	
	void beginLightPass() {
		glDisable(GL_DEPTH_TEST);
		
		glBindFramebuffer(GL_FRAMEBUFFER, lightPassFbo);
		
		static const GLenum invalidateAttachment = GL_COLOR_ATTACHMENT0;
		glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &invalidateAttachment);
		
		baseLightShader.use();
		gbufferColorAttachment1.bind(0);
		gbufferColorAttachment2.bind(1);
		gbufferDepthAttachment.bind(2);
		glBindTextureUnit(3, skyboxTexture);
		
		glDrawArrays(GL_TRIANGLES, 0, 3);
		
		glEnable(GL_BLEND);
	}
	
	void renderPointLight() {
		
	}
	
	void beginEmissive() {
		glBindFramebuffer(GL_FRAMEBUFFER, emissivePassFbo);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(0);
	}
	
	void endLightPass() {
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		postShader.use();
		lightPassAttachment.bind(0);
		gbufferDepthAttachment.bind(1);
		
		glEnable(GL_FRAMEBUFFER_SRGB);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
}
