#include <SDL.h>
#include <SDL_image.h>

#include "input.hpp"
#include "resources.hpp"
#include "ship.hpp"
#include "utils.hpp"
#include "graphics/shadows.hpp"
#include "graphics/asteroids.hpp"
#include "graphics/model.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer.hpp"
#include "graphics/stars.hpp"

int main() {
	if (SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << SDL_GetError() << std::endl;
		return 1;
	}
	
	IMG_Init(IMG_INIT_PNG);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG | SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
	
	SDL_Window* window = SDL_CreateWindow("Space Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == nullptr) {
		std::cerr << SDL_GetError() << std::endl;
		return 1;
	}
	
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr) {
		std::cerr << SDL_GetError() << std::endl;
		return 1;
	}
	
	GLenum glewInitStatus = glewInit();
	if (glewInitStatus != GLEW_OK) {
		std::cerr << glewGetErrorString(glewInitStatus) << std::endl;
		return 1;
	}
	
	std::cout << "using opengl device " << glGetString(GL_RENDERER) << std::endl;
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glDepthFunc(GL_LEQUAL);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &renderer::uboAlignment);
	
#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback([] (GLenum, GLenum, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*) {
		if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
			std::string_view messageView(message);
			if (messageView.back() == '\n') {
				messageView = messageView.substr(0, messageView.size() - 1);
			}
			std::cout << " [gl] " << messageView << std::endl;
		}
	}, nullptr);
#endif
	
	initializeShadowMapping();
	Model::initializeVao();
	res::load();
	
	Shader modelShader;
	modelShader.attachStage(GL_VERTEX_SHADER, "model.vs.glsl");
	modelShader.attachStage(GL_FRAGMENT_SHADER, "model.fs.glsl");
	modelShader.link("model");
	
	Shader modelShaderShadow;
	modelShaderShadow.attachStage(GL_VERTEX_SHADER, "model_shadow.vs.glsl");
	modelShaderShadow.link("model_shadow");
	
	Shader emissiveShader;
	emissiveShader.attachStage(GL_VERTEX_SHADER, "model.vs.glsl");
	emissiveShader.attachStage(GL_FRAGMENT_SHADER, "emissive.fs.glsl");
	emissiveShader.link("emissive");
	
	const glm::vec3 emissiveColor = 5.0f * glm::convertSRGBToLinear(glm::vec3(153, 196, 233) / 255.0f);
	glProgramUniform3fv(emissiveShader.program, 1, 1, (const float*)&emissiveColor);
	
	renderer::initialize();
	
	stars::initialize();
	
	initializeAsteroids();
	
	uint64_t lastFrameBegin = SDL_GetPerformanceCounter();
	const uint64_t perfCounterFrequency = SDL_GetPerformanceFrequency();
	
	GLsync fences[renderer::frameCycleLen] = { };
	
	InputState curInput, prevInput;
	
	Ship ship;
	
	glm::mat4 projMatrix, inverseProjMatrix;
	int prevDrawableWidth = -1, prevDrawableHeight = -1;
	
	std::array<glm::vec4, 6> frustumPlanes;
	bool frustumPlanesFrozen = false;
	bool drawAsteroidsWireframe = false;
	
	bool shouldClose = false;
	while (!shouldClose) {
		const uint64_t thisFrameBegin = SDL_GetPerformanceCounter();
		const float currentTime = (float)thisFrameBegin / (float)perfCounterFrequency;
		const float dt = (thisFrameBegin - lastFrameBegin) / (float)perfCounterFrequency;
		lastFrameBegin = thisFrameBegin;
		prevInput = curInput;
		
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				shouldClose = true;
			if (event.type == SDL_KEYDOWN)
				curInput.keyStateChanged(event.key.keysym.scancode, true);
			if (event.type == SDL_KEYUP) {
				curInput.keyStateChanged(event.key.keysym.scancode, false);
				if (event.key.keysym.scancode == SDL_SCANCODE_F8)
					frustumPlanesFrozen = !frustumPlanesFrozen;
				if (event.key.keysym.scancode == SDL_SCANCODE_F7)
					drawAsteroidsWireframe = !drawAsteroidsWireframe;
			}
		}
		
		ship.update(dt, curInput, prevInput);
		
		if (fences[renderer::frameCycleIndex] != nullptr) {
			glClientWaitSync(fences[renderer::frameCycleIndex], GL_SYNC_FLUSH_COMMANDS_BIT, UINT64_MAX);
			glDeleteSync(fences[renderer::frameCycleIndex]);
		}
		
		int drawableWidth, drawableHeight;
		SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
		renderer::updateFramebuffers(drawableWidth, drawableHeight);
		
		if (prevDrawableWidth != drawableWidth || prevDrawableHeight != drawableHeight) {
			prevDrawableWidth = drawableWidth;
			prevDrawableHeight = drawableHeight;
			projMatrix = glm::perspectiveFov(glm::radians(75.0f), (float)drawableWidth, (float)drawableHeight, Z_NEAR, Z_FAR);
			inverseProjMatrix = glm::inverse(projMatrix);
		}
		
		RenderSettings renderSettings;
		renderSettings.vpMatrix = projMatrix * ship.viewMatrix;
		renderSettings.vpMatrixInverse = ship.viewMatrixInv * inverseProjMatrix;
		renderSettings.cameraPos = glm::vec3(ship.viewMatrixInv[3]);
		renderSettings.gameTime = currentTime;
		renderSettings.sunColor = { 1, 0.9f, 0.95f };
		renderSettings.sunDir = glm::normalize(glm::vec3(1, -1, 1));
		renderSettings.shadowMatrix = calculateShadowMapMatrix(renderSettings.vpMatrixInverse, renderSettings.sunDir);
		renderer::updateRenderSettings(renderSettings);
		
		if (!frustumPlanesFrozen) {
			frustumPlanes = createFrustumPlanes(renderSettings.vpMatrixInverse);
		}
		
		std::array<glm::vec4, 6> frustumPlanesShadow = createFrustumPlanes(glm::inverse(renderSettings.shadowMatrix));
		prepareAsteroids(renderSettings.cameraPos, frustumPlanes.data(), frustumPlanesShadow.data());
		
		beginRenderShadows();
		
		glCullFace(GL_FRONT);
		
		//glBindVertexArray(Model::vao);
		//modelShaderShadow.use();
		//glUniformMatrix4fv(0, 1, false, (const float*)&ship.worldMatrix);
		//res::shipModel.bind();
		//res::shipModel.drawMesh(res::shipModel.findMesh("Aluminum"), 1);
		
		drawAsteroidsShadow();
		
		glCullFace(GL_BACK);
		
		renderer::beginMainPass();
		
		glBindTextureUnit(2, shadowMap);
		
		glBindVertexArray(Model::vao);
		modelShader.use();
		res::shipAlbedo.bind(0);
		res::shipNormals.bind(1);
		glUniformMatrix4fv(0, 1, false, (const float*)&ship.worldMatrix);
		glUniform3f(1, 3, 20, 100); //specular intensity (low), specular intensity (high), specular exponent
		res::shipModel.bind();
		res::shipModel.drawMesh(res::shipModel.findMesh("Aluminum"), 1);
		
		emissiveShader.use();
		glUniformMatrix4fv(0, 1, false, (const float*)&ship.worldMatrix);
		//res::shipModel.bind();
		res::shipModel.drawMesh(res::shipModel.findMesh("BlueL"), 1);
		res::shipModel.drawMesh(res::shipModel.findMesh("BlueS"), 1);
		
		drawAsteroids(drawAsteroidsWireframe);
		
		stars::draw(drawableWidth, drawableHeight);
		
		renderer::endMainPass();
		
		fences[renderer::frameCycleIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		renderer::frameCycleIndex = (renderer::frameCycleIndex + 1) % renderer::frameCycleLen;
		
		SDL_GL_SwapWindow(window);
	}
	
	res::destroy();
	
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
