#include <SDL.h>

#include "input.hpp"
#include "resources.hpp"
#include "ship.hpp"
#include "settings.hpp"
#include "utils.hpp"
#include "target.hpp"
#include "graphics/ui.hpp"
#include "graphics/sphere.hpp"
#include "graphics/particles.hpp"
#include "graphics/shadows.hpp"
#include "graphics/asteroids.hpp"
#include "graphics/model.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer.hpp"

#include <iomanip>

int main() {
	initExeDirPath();
	settings::parse();
	
	if (SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << SDL_GetError() << std::endl;
		return 1;
	}
	
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
	
	if (settings::mouseInput) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	
	std::cout << "using opengl device " << glGetString(GL_RENDERER) << std::endl;
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glDepthFunc(GL_LEQUAL);
	glBlendEquation(GL_FUNC_ADD);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &renderer::uboAlignment);
	
	for (int i = 0; i < 3; i++) {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &maxComputeWorkGroupSize[i]);
	}
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeWorkGroupInvocations);
	
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
	ui::initialize();
	Model::initializeVao();
	res::load();
	
	renderer::initialize();
	Ship::initShaders();
	generateSphereMeshes();
	initializeTargetShader();
	initializeParticles();
	initializeAsteroids();
	
	uint64_t lastFrameBegin = SDL_GetPerformanceCounter();
	const uint64_t perfCounterFrequency = SDL_GetPerformanceFrequency();
	
	GLsync fences[renderer::frameCycleLen] = { };
	
	InputState curInput, prevInput;
	
	Ship ship;
	
	glm::mat4 prevViewProj;
	std::array<glm::vec4, 6> frustumPlanes;
	bool frustumPlanesFrozen = false;
	bool drawAsteroidsWireframe = false;
	
	glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, -1, -0.8f));
	
	constexpr float LOW_FOV = 75.0f;
	constexpr float HIGH_FOV = 110.0f;
	
	Target nextTarget;
	Target optionalTarget;
	
	nextTarget.color = glm::convertSRGBToLinear(glm::vec3(0.5f, 0.5f, 1.0f));
	nextTarget.pos = ship.pos + glm::vec3(0);
	
	optionalTarget.color = glm::convertSRGBToLinear(glm::vec3(1.0f, 0.5f, 0.5f));
	optionalTarget.pos =ship.pos + glm::vec3(0, 0, 10000);
	
	uint64_t prevFrameEnd = 0, prevFrameAfterSwap = 0;
	
	bool shouldClose = false;
	while (!shouldClose) {
		const uint64_t thisFrameBegin = SDL_GetPerformanceCounter();
		const float currentTime = (float)thisFrameBegin / (float)perfCounterFrequency;
		const float dt = (thisFrameBegin - lastFrameBegin) / (float)perfCounterFrequency;
		lastFrameBegin = thisFrameBegin;
		prevInput = curInput;
		curInput.mouseDX = 0;
		curInput.mouseDY = 0;
		
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
				if (event.key.keysym.scancode == SDL_SCANCODE_C)
					ship.stopped = !ship.stopped;
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					shouldClose = true;
			}
			if (event.type == SDL_MOUSEMOTION) {
				curInput.mouseDX += event.motion.xrel;
				curInput.mouseDY += event.motion.yrel;
			}
		}
		SDL_GetMouseState(&curInput.mouseX, &curInput.mouseY);
		
		ship.update(dt, curInput, prevInput);
		glm::vec3 boxPositionOffset = glm::vec3(ship.boxIndex) * asteroidBoxSize;
		
		uint64_t fenceWaitTime = 0;
		if (fences[renderer::frameCycleIndex] != nullptr) {
			uint64_t beforeWaitFence = SDL_GetPerformanceCounter();
			glClientWaitSync(fences[renderer::frameCycleIndex], GL_SYNC_FLUSH_COMMANDS_BIT, UINT64_MAX);
			glDeleteSync(fences[renderer::frameCycleIndex]);
			fenceWaitTime = SDL_GetPerformanceCounter() - beforeWaitFence;
		}
		
		int drawableWidth, drawableHeight;
		SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
		renderer::updateFramebuffers(drawableWidth, drawableHeight);
		
		float fov = glm::radians(glm::mix(LOW_FOV, HIGH_FOV, ship.speed01));
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
			
			prepareAsteroids(renderSettings.cameraPos, frustumPlanes.data(), shadowMapMatrices.frustumPlanes);
		}
		
		renderShadows([&] (uint32_t cascade) {
			drawAsteroidsShadow(cascade, shadowMapMatrices.matrices[cascade]);
		});
		
		renderer::beginMainPass();
		
		glBindTextureUnit(2, shadowMap);
		ship.draw();
		drawAsteroids(drawAsteroidsWireframe);
		renderer::drawSkybox();
		
		nextTarget.truePos = nextTarget.pos - boxPositionOffset;
		optionalTarget.truePos = optionalTarget.pos - boxPositionOffset;
		
		beginDrawTargets(currentTime);
		drawTarget(nextTarget);
		drawTarget(optionalTarget);
		endDrawTargets();
		
		drawParticles(currentTime, renderSettings.cameraPos);
		
		renderer::endMainPass(prevViewProj, dt);
		
		ui::begin(drawableWidth, drawableHeight);
		
#ifdef DEBUG
		uint64_t elapsedTicks = SDL_GetPerformanceCounter() - prevFrameEnd;
		
		auto floatToStr = [&] (float f) {
			std::ostringstream stream;
			stream << std::setprecision(2) << std::fixed << f;
			return stream.str();
		};
		
		glm::vec3 truePos = ship.pos + boxPositionOffset;
		std::string debugLines[] = {
			"vel: " + floatToStr(ship.forwardVel),
			"tpos: " + floatToStr(truePos.x) + ", " + floatToStr(truePos.y) + ", " + floatToStr(truePos.z),
			"bpos: " + floatToStr(ship.pos.x) + ", " + floatToStr(ship.pos.y) + ", " + floatToStr(ship.pos.z),
			"box: " + std::to_string(ship.boxIndex.x) + ", " + std::to_string(ship.boxIndex.y) + ", " + std::to_string(ship.boxIndex.z),
			"atot: " + std::to_string(numAsteroids),
			"fps: " + floatToStr(1.0f / dt),
			"frame: " + floatToStr(1000 * (float)elapsedTicks / (float)perfCounterFrequency) + "ms",
			"sync: " + floatToStr(1000 * (float)fenceWaitTime / (float)perfCounterFrequency) + "ms",
			"swap: " + floatToStr(1000 * (float)(prevFrameAfterSwap - prevFrameEnd) / (float)perfCounterFrequency) + "ms"
		};
		for (size_t i = 0; i < std::size(debugLines); i++) {
			ui::drawText(debugLines[i], glm::vec2(10, drawableHeight - 30 - 25 * i), glm::vec4(1, 1, 1, 0.5f));
		}
#endif
		
		ui::end();
		
		fences[renderer::frameCycleIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		renderer::frameCycleIndex = (renderer::frameCycleIndex + 1) % renderer::frameCycleLen;
		
		prevFrameEnd = SDL_GetPerformanceCounter();
		
		SDL_GL_SwapWindow(window);
		
		prevFrameAfterSwap = SDL_GetPerformanceCounter();
		
		prevViewProj = renderSettings.vpMatrix;
	}
	
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
