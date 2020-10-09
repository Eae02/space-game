#include <SDL.h>
#include <SDL_image.h>

#include "input.hpp"
#include "resources.hpp"
#include "ship.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "graphics/shadows.hpp"
#include "graphics/asteroids.hpp"
#include "graphics/model.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer.hpp"
#include "graphics/stars.hpp"

int main() {
	initExeDirPath();
	settings::parse();
	
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
	
	uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	if (settings::fullscreen) {
		windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	
	SDL_Window* window = SDL_CreateWindow("Space Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, windowFlags);
	if (window == nullptr) {
		std::cerr << SDL_GetError() << std::endl;
		return 1;
	}
	
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr) {
		std::cerr << SDL_GetError() << std::endl;
		return 1;
	}
	
	SDL_GL_SetSwapInterval(settings::vsync);
	
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
	
	renderer::initialize();
	stars::initialize();
	Ship::initShaders();
	initializeAsteroids();
	
	uint64_t lastFrameBegin = SDL_GetPerformanceCounter();
	const uint64_t perfCounterFrequency = SDL_GetPerformanceFrequency();
	
	GLsync fences[renderer::frameCycleLen] = { };
	
	InputState curInput, prevInput;
	
	Ship ship;
	ship.pos.y += ASTEROID_BOX_HEIGHT * 0.9f;
	
	std::array<glm::vec4, 6> frustumPlanes;
	bool frustumPlanesFrozen = false;
	bool drawAsteroidsWireframe = false;
	
	glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, -1, -0.8f));
	
	constexpr float TIME_PER_FPS_PRINT = 3;
	float timeAtLastFpsPrint = SDL_GetPerformanceCounter() / perfCounterFrequency;
	uint32_t numFrames = 0;
	
	constexpr float LOW_FOV = 70.0f;
	constexpr float HIGH_FOV = 110.0f;
	constexpr float LOW_FOV_SPEED = 20.0f;
	constexpr float HIGH_FOV_SPEED = 200.0f;
	
	bool shouldClose = false;
	while (!shouldClose) {
		const uint64_t thisFrameBegin = SDL_GetPerformanceCounter();
		const float currentTime = (float)thisFrameBegin / (float)perfCounterFrequency;
		const float dt = (thisFrameBegin - lastFrameBegin) / (float)perfCounterFrequency;
		lastFrameBegin = thisFrameBegin;
		prevInput = curInput;
		
		if (currentTime - timeAtLastFpsPrint > TIME_PER_FPS_PRINT) {
			std::cout << "avg fps: " << (numFrames / (currentTime - timeAtLastFpsPrint)) << std::endl;
			timeAtLastFpsPrint = currentTime;
			numFrames = 0;
		}
		
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
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					shouldClose = true;
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
		
		float fovSpeed01 = glm::clamp((ship.forwardVel - LOW_FOV_SPEED) / (HIGH_FOV_SPEED - LOW_FOV_SPEED), 0.0f, 1.0f);
		float fov = glm::radians(glm::mix(LOW_FOV, HIGH_FOV, fovSpeed01));
		glm::mat4 projMatrix = glm::perspectiveFov(fov, (float)drawableWidth, (float)drawableHeight, Z_NEAR, Z_FAR);
		glm::mat4 inverseProjMatrix = glm::inverse(projMatrix);
		glm::mat4 vpMatrixInv = ship.viewMatrixInv * inverseProjMatrix;
		ShadowMapMatrices shadowMapMatrices = calculateShadowMapMatrices(vpMatrixInv, sunDir);
		
		RenderSettings renderSettings;
		renderSettings.vpMatrix = projMatrix * ship.viewMatrix;
		renderSettings.vpMatrixInverse = vpMatrixInv;
		renderSettings.cameraPos = glm::vec3(ship.viewMatrixInv[3]);
		renderSettings.gameTime = currentTime;
		renderSettings.sunColor = { 1, 0.9f, 0.95f };
		renderSettings.sunDir = sunDir;
		std::copy_n(shadowMapMatrices.matrices, NUM_SHADOW_CASCADES, renderSettings.shadowMatrices);
		renderer::updateRenderSettings(renderSettings);
		
		if (!frustumPlanesFrozen) {
			frustumPlanes = createFrustumPlanes(vpMatrixInv);
		}
		
		prepareAsteroids(renderSettings.cameraPos, frustumPlanes.data(), shadowMapMatrices.frustumPlanes);
		
		renderShadows([&] (uint32_t cascade) {
			ship.drawShadow(shadowMapMatrices.matrices[cascade]);
			drawAsteroidsShadow(cascade, shadowMapMatrices.matrices[cascade]);
		});
		
		renderer::beginMainPass();
		
		glBindTextureUnit(2, shadowMap);
		ship.draw();
		drawAsteroids(drawAsteroidsWireframe);
		
		stars::draw(drawableWidth, drawableHeight);
		
		renderer::endMainPass();
		
		fences[renderer::frameCycleIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		renderer::frameCycleIndex = (renderer::frameCycleIndex + 1) % renderer::frameCycleLen;
		
		SDL_GL_SwapWindow(window);
		
		numFrames++;
	}
	
	res::destroy();
	
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
