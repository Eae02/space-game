#include "renderer.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "../utils.hpp"
#include "../settings.hpp"

namespace renderer {
	uint32_t frameCycleIndex = 0;
	int uboAlignment = 0;
	
	static Texture mainPassColorAttachment;
	static Texture mainPassDepthAttachment;
	
	static constexpr uint32_t BLOOM_STEPS = 4;
	static Texture bloomDownscaleAttachments[BLOOM_STEPS];
	static Texture bloomBlurAttachments[BLOOM_STEPS];
	
	static uint32_t fbWidth = 0;
	static uint32_t fbHeight = 0;
	
	static bool framebuffersInitialized = false;
	static GLuint mainPassFbo;
	static GLuint bloomDownscaleFbos[BLOOM_STEPS];
	static GLuint bloomBlurFbos[BLOOM_STEPS];
	
	static GLuint renderSettingsUbo;
	static uint64_t renderSettingsOffset;
	static char* renderSettingsUboMemory;
	
	static Shader baseLightShader;
	static Shader bloomDownscaleShader;
	static Shader bloomBlurShader;
	static Shader postShader;
	
	void initialize() {
		mainPassColorAttachment.format = GL_RGBA16F;
		mainPassDepthAttachment.format = GL_DEPTH_COMPONENT32F;
		for (uint32_t i = 0; i < BLOOM_STEPS; i++) {
			bloomDownscaleAttachments[i].format = GL_RGBA16F;
			bloomBlurAttachments[i].format = GL_RGBA16F;
		}
		
		std::string extraFSCode;
		if (settings::volumetricLighting) {
			extraFSCode += "#define ENABLE_VOL_LIGHT\n";
		}
		if (settings::motionBlur) {
			extraFSCode += "#define ENABLE_MOTION_BLUR\n";
		}
		
		postShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
		postShader.attachStage(GL_FRAGMENT_SHADER, "post.fs.glsl", extraFSCode);
		postShader.link("Post");
		
		bloomDownscaleShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
		bloomDownscaleShader.attachStage(GL_FRAGMENT_SHADER, "bloom_downscale.fs.glsl");
		bloomDownscaleShader.link("BloomDownscale");
		
		bloomBlurShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
		bloomBlurShader.attachStage(GL_FRAGMENT_SHADER, "bloom_blur.fs.glsl");
		bloomBlurShader.link("BloomBlur");
		
		glCreateBuffers(1, &renderSettingsUbo);
		renderSettingsOffset = roundToNextMul(sizeof(RenderSettings), uboAlignment);
		const int64_t bufferLen = renderSettingsOffset * frameCycleLen;
		glNamedBufferStorage(renderSettingsUbo, bufferLen, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		renderSettingsUboMemory = (char*)glMapNamedBufferRange(renderSettingsUbo, 0, bufferLen,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	}
	
	void updateFramebuffers(uint32_t width, uint32_t height) {
		if (fbWidth == width && fbHeight == height)
			return;
		
		if (framebuffersInitialized) {
			glDeleteFramebuffers(1, &mainPassFbo);
			if (settings::bloom) {
				glDeleteFramebuffers(BLOOM_STEPS, bloomDownscaleFbos);
				glDeleteFramebuffers(BLOOM_STEPS, bloomBlurFbos);
			}
		}
		
		fbWidth = width;
		fbHeight = height;
		framebuffersInitialized = true;
		
		mainPassColorAttachment.width = width;
		mainPassColorAttachment.height = height;
		mainPassColorAttachment.initialize();
		mainPassColorAttachment.setParamsForFramebuffer();
		
		mainPassDepthAttachment.width = width;
		mainPassDepthAttachment.height = height;
		mainPassDepthAttachment.initialize();
		mainPassDepthAttachment.setParamsForFramebuffer();
		
		if (settings::bloom) {
			glCreateFramebuffers(BLOOM_STEPS, bloomDownscaleFbos);
			glCreateFramebuffers(BLOOM_STEPS, bloomBlurFbos);
			for (uint32_t i = 0; i < BLOOM_STEPS; i++) {
				uint32_t widthDS = width >> (i + 1);
				uint32_t heightDS = height >> (i + 1);
				
				bloomDownscaleAttachments[i].width = widthDS;
				bloomDownscaleAttachments[i].height = heightDS;
				bloomDownscaleAttachments[i].initialize();
				bloomDownscaleAttachments[i].setParamsForFramebuffer();
				
				bloomBlurAttachments[i].width = widthDS;
				bloomBlurAttachments[i].height = heightDS;
				bloomBlurAttachments[i].initialize();
				glTextureParameteri(bloomBlurAttachments[i].texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTextureParameteri(bloomBlurAttachments[i].texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				
				glNamedFramebufferTexture(bloomDownscaleFbos[i], GL_COLOR_ATTACHMENT0, bloomDownscaleAttachments[i].texture, 0);
				glNamedFramebufferTexture(bloomBlurFbos[i], GL_COLOR_ATTACHMENT0, bloomBlurAttachments[i].texture, 0);
			}
		}
		
		glCreateFramebuffers(1, &mainPassFbo);
		glNamedFramebufferTexture(mainPassFbo, GL_COLOR_ATTACHMENT0, mainPassColorAttachment.texture, 0);
		glNamedFramebufferTexture(mainPassFbo, GL_DEPTH_ATTACHMENT, mainPassDepthAttachment.texture, 0);
	}
	
	void updateRenderSettings(const RenderSettings& renderSettings) {
		const int64_t bufferOffset = frameCycleIndex * renderSettingsOffset;
		memcpy(renderSettingsUboMemory + bufferOffset, &renderSettings, sizeof(RenderSettings));
		glFlushMappedNamedBufferRange(renderSettingsUbo, bufferOffset, sizeof(RenderSettings));
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, renderSettingsUbo, bufferOffset, sizeof(RenderSettings));
	}
	
	void beginMainPass() {
		glBindFramebuffer(GL_FRAMEBUFFER, mainPassFbo);
		glViewport(0, 0, fbWidth, fbHeight);
		
		glDepthMask(1);
		glEnable(GL_DEPTH_TEST);
		
		const float clearColor[] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, clearColor);
		glClearBufferfv(GL_COLOR, 1, clearColor);
		
		const float clearDepth = 1;
		glClearBufferfv(GL_DEPTH, 0, &clearDepth);
	}
	
	constexpr float BLOOM_MIN_BRIGHTNESS = 1.0f;
	constexpr float BLOOM_BLUR_RAD = 1.0f;
	constexpr float BLOOM_BRIGHTNESS = 0.7f;
	
	constexpr float MOTION_BLUR_INTENSITY = 0.02f;
	
	inline void invalidateAttachment(GLenum attachment) {
		glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &attachment);
	}
	
	void endMainPass(const glm::mat4& prevViewProj, float dt) {
		glDisable(GL_DEPTH_TEST);
		
		if (settings::bloom) {
			//bloom downscale
			bloomDownscaleShader.use();
			for (uint32_t i = 0; i < BLOOM_STEPS; i++) {
				glBindFramebuffer(GL_FRAMEBUFFER, bloomDownscaleFbos[i]);
				glViewport(0, 0, fbWidth >> (i + 1), fbHeight >> (i + 1));
				invalidateAttachment(GL_COLOR_ATTACHMENT0);
				if (i) {
					glUniform1f(0, 0);
					bloomDownscaleAttachments[i - 1].bind(0);
				} else {
					glUniform1f(0, BLOOM_MIN_BRIGHTNESS);
					mainPassColorAttachment.bind(0);
				}
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			
			//Bloom first blur
			bloomBlurShader.use();
			for (uint32_t i = 0; i < BLOOM_STEPS; i++) {
				uint32_t curWidth = fbWidth >> (i + 1);
				uint32_t curHeight = fbHeight >> (i + 1);
				glBindFramebuffer(GL_FRAMEBUFFER, bloomBlurFbos[i]);
				glViewport(0, 0, curWidth, curHeight);
				invalidateAttachment(GL_COLOR_ATTACHMENT0);
				bloomDownscaleAttachments[i].bind(0);
				glUniform2f(0, BLOOM_BLUR_RAD / curWidth, 0);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			
			//Bloom second blur
			glBindFramebuffer(GL_FRAMEBUFFER, mainPassFbo);
			glViewport(0, 0, fbWidth, fbHeight);
			glEnable(GL_BLEND);
			glUniform1f(1, BLOOM_BRIGHTNESS);
			for (uint32_t i = 0; i < BLOOM_STEPS; i++) {
				bloomBlurAttachments[i].bind(0);
				glUniform2f(0, 0, BLOOM_BLUR_RAD / (fbHeight >> (i + 1)));
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			glDisable(GL_BLEND);
		}
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		invalidateAttachment(GL_COLOR);
		
		postShader.use();
		mainPassColorAttachment.bind(0);
		mainPassDepthAttachment.bind(1);
		glBindTextureUnit(2, shadowMap);
		
		if (settings::motionBlur) {
			glUniformMatrix4fv(0, 1, false, (const float*)&prevViewProj);
			glUniform1f(1, MOTION_BLUR_INTENSITY / dt);
		}
		
		glEnable(GL_FRAMEBUFFER_SRGB);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisable(GL_FRAMEBUFFER_SRGB);
	}
}
