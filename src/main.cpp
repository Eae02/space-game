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
#include "graphics/collision_debug.hpp"
#include "game.hpp"
#include "menu.hpp"

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
	
	std::array<glm::vec4, 6> frustumPlanes;
	bool frustumPlanesFrozen = false;
	bool drawAsteroidsWireframe = false;
	
	uint64_t prevFrameEnd = 0, prevFrameAfterSwap = 0;
	
	Game game;
	bool inGame = false;
	
	float gameScreenFade = 1;
	
	ui::Button gameOverPlayAgain = { "Play Again" };
	ui::Button gameOverMainMenu = { "Main Menu" };
	
	bool shouldClose = false;
	while (!shouldClose) {
		const uint64_t thisFrameBegin = SDL_GetPerformanceCounter();
		dt = std::min((thisFrameBegin - lastFrameBegin) / (float)perfCounterFrequency, 0.1f);
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
#ifdef DEBUG
				if (event.key.keysym.scancode == SDL_SCANCODE_F6)
					game.invincibleOverride = !game.invincibleOverride;
				if (event.key.keysym.scancode == SDL_SCANCODE_F7)
					drawAsteroidsWireframe = !drawAsteroidsWireframe;
				if (event.key.keysym.scancode == SDL_SCANCODE_F8)
					frustumPlanesFrozen = !frustumPlanesFrozen;
				if (event.key.keysym.scancode == SDL_SCANCODE_F9)
					game.remTime = 60;
				if (event.key.keysym.scancode == SDL_SCANCODE_F10)
					collisionDebug::enabled = !collisionDebug::enabled;
#endif
				if (event.key.keysym.scancode == SDL_SCANCODE_C)
					game.ship.stopped = true;
				if (event.key.keysym.scancode == SDL_SCANCODE_X)
					shouldHideTargetInfo = !shouldHideTargetInfo;
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					shouldClose = true;
			}
			if (event.type == SDL_MOUSEMOTION) {
				curInput.mouseDX += event.motion.xrel;
				curInput.mouseDY += event.motion.yrel;
			}
		}
		curInput.leftMouse = (SDL_GetMouseState(&curInput.mouseX, &curInput.mouseY) & SDL_BUTTON_LMASK) != 0;
		
		if (inGame) {
			game.runFrame(curInput, prevInput);
		}
		
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
		
		RenderSettings renderSettings;
		if (inGame) {
			game.initRenderSettings(drawableWidth, drawableHeight, renderSettings);
		} else {
			menu::initRenderSettings(drawableWidth, drawableHeight, renderSettings);
		}
		
		ShadowMapMatrices shadowMapMatrices = calculateShadowMapMatrices(renderSettings.vpMatrixInverse, SUN_DIR);
		std::copy_n(shadowMapMatrices.matrices, NUM_SHADOW_CASCADES, renderSettings.shadowMatrices);
		renderer::updateRenderSettings(renderSettings);
		
		if (!frustumPlanesFrozen) {
			frustumPlanes = createFrustumPlanes(renderSettings.vpMatrixInverse);
			prepareAsteroids(frustumPlanes.data(), shadowMapMatrices.frustumPlanes);
		}
		
		renderShadows([&] (uint32_t cascade) {
			drawAsteroidsShadow(cascade, shadowMapMatrices.matrices[cascade]);
		});
		
		renderer::beginMainPass();
		
		glBindTextureUnit(2, shadowMap);
		
		if (inGame) {
			game.ship.draw();
		}
		drawAsteroids(drawAsteroidsWireframe);
		renderer::drawSkybox();
		
		if (inGame) {
			beginDrawTargets();
			for (Target& target : game.targets) {
				drawTarget(target, game.targetsAlpha);
			}
			endDrawTargets();
		}
		
		drawParticles(renderSettings.cameraPos);
		
		if (inGame) {
			if (game.isGameOver) {
				gameScreenFade = std::max(gameScreenFade - dt * 2, 0.4f);
			} else {
				gameScreenFade = std::min(gameScreenFade + dt * 2, 1.0f);
			}
		} else {
			gameScreenFade = std::max(gameScreenFade - dt * 2, 0.8f);
		}
		
		glm::vec3 vignetteColor(0);
		glm::vec3 overlayColor(0.5f);
		if (inGame && !game.isGameOver) {
			vignetteColor = game.vignetteColor * game.vignetteColorFade;
		}
		renderer::endMainPass(vignetteColor, glm::vec3(gameScreenFade));
		
		collisionDebug::draw();
		
		ui::begin(drawableWidth, drawableHeight);
		
		if (inGame) {
			for (Target& target : game.targets) {
				drawTargetUI(target, renderSettings.vpMatrix, glm::vec2(drawableWidth, drawableHeight), game.ship, game.remTime);
			}
			
			if (game.isGameOver) {
				Rect gameOverSrcRect = { glm::vec2(0, 150), glm::vec2(256, 45) };
				glm::vec2 centerScreen = glm::vec2(drawableWidth / 2.0f, drawableHeight / 2.0f);
				ui::drawSprite(centerScreen - gameOverSrcRect.size / 2.0f, gameOverSrcRect);
				
				ColoredStringBuilder scoreStringBuilder = game.buildScoreString();
				std::string_view scoreLabel = "Score:";
				
				float scoreLabelWidth = ui::textWidth(scoreLabel) + 10;
				float totScoreWidth = scoreLabelWidth + ui::textWidth(scoreStringBuilder.text);
				float scoreTextY = centerScreen.y - 60;
				ui::drawText(scoreLabel, glm::vec2(centerScreen.x - totScoreWidth / 2, scoreTextY), glm::vec4(1, 1, 1, 0.6f));
				ui::drawText(scoreStringBuilder, glm::vec2(centerScreen.x - totScoreWidth / 2 + scoreLabelWidth, scoreTextY));
				
				gameOverPlayAgain.pos = centerScreen - glm::vec2(0, 110);
				gameOverMainMenu.pos = centerScreen - glm::vec2(0, 150);
				
				if (gameOverPlayAgain(curInput)) {
					game.newGame();
				}
				
				if (gameOverMainMenu(curInput)) {
					inGame = false;
				}
			}
		} else {
			menu::updateAndDraw(drawableWidth, drawableHeight, curInput, inGame, shouldClose);
			if (inGame) {
				game.newGame();
			}
		}
		
#ifdef DEBUG
		uint64_t elapsedTicks = SDL_GetPerformanceCounter() - prevFrameEnd;
		auto floatToStr = [&] (float f) {
			std::ostringstream stream;
			stream << std::setprecision(2) << std::fixed << f;
			return stream.str();
		};
		const glm::vec3 truePos = game.ship.pos + glm::vec3(game.ship.boxIndex) * ASTEROID_BOX_SIZE;
		std::string debugLines[] = {
			"vel: " + floatToStr(game.ship.forwardVel),
			"tpos: " + floatToStr(truePos.x) + ", " + floatToStr(truePos.y) + ", " + floatToStr(truePos.z),
			"bpos: " + floatToStr(game.ship.pos.x) + ", " + floatToStr(game.ship.pos.y) + ", " + floatToStr(game.ship.pos.z),
			"box: " + std::to_string(game.ship.boxIndex.x) + ", " + std::to_string(game.ship.boxIndex.y) + ", " + std::to_string(game.ship.boxIndex.z),
			"atot: " + std::to_string(numAsteroids),
			(game.ship.intersected ? "int: true" : "int: false"),
			"fps: " + floatToStr(1.0f / dt),
			"frame: " + floatToStr(1000 * (float)elapsedTicks / (float)perfCounterFrequency) + "ms",
			"sync: " + floatToStr(1000 * (float)fenceWaitTime / (float)perfCounterFrequency) + "ms",
			"swap: " + floatToStr(1000 * (float)(prevFrameAfterSwap - prevFrameEnd) / (float)perfCounterFrequency) + "ms"
		};
		for (size_t i = 0; i < std::size(debugLines); i++) {
			ui::drawText(debugLines[i], glm::vec2(10, drawableHeight - 30 - 25 * i), glm::vec4(1, 1, 1, 0.5f));
		}
#endif
		
		if (inGame) {
			if (game.invincibleTime > 0 || game.invincibleOverride) {
				std::string_view invulnString = "invulnerable";
				glm::vec4 invulnColor(0.5f, 0.5f, 1.0f, std::min(game.invincibleTime * 5, 1.0f));
				if (game.invincibleOverride)
					invulnColor.a = 1;
				ui::drawTextCentered(invulnString, glm::vec2(drawableWidth / 2.0f, 70), invulnColor);
			}
			
			std::string speedString = std::to_string((int)std::round(game.ship.forwardVel)) + "m/s";
			ui::drawText(speedString, glm::vec2(drawableWidth / 2.0f - ui::textWidth(speedString) / 2, 5), glm::vec4(1));
			
			std::string timeString = std::to_string((int)std::round(game.remTime)) + "s";
			ui::drawText(timeString, glm::vec2(drawableWidth / 2.0f + 60, 5), glm::vec4(1));
			
			ColoredStringBuilder scoreStringBuilder = game.buildScoreString();
			ui::drawText(scoreStringBuilder, glm::vec2(drawableWidth / 2.0f - 60 - ui::textWidth(scoreStringBuilder.text), 5));
			
			ui::drawText("speed", glm::vec2(drawableWidth / 2.0f - ui::textWidth("speed") / 2, 30), glm::vec4(1, 1, 1, 0.5f));
			ui::drawText("time", glm::vec2(drawableWidth / 2.0f + 60, 30), glm::vec4(1, 1, 1, 0.5f));
			ui::drawText("score", glm::vec2(drawableWidth / 2.0f - 60 - ui::textWidth("score"), 30), glm::vec4(1, 1, 1, 0.5f));
		}
		
		ui::end();
		
		fences[renderer::frameCycleIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		renderer::frameCycleIndex = (renderer::frameCycleIndex + 1) % renderer::frameCycleLen;
		
		prevFrameEnd = SDL_GetPerformanceCounter();
		
		SDL_GL_SwapWindow(window);
		
		prevFrameAfterSwap = SDL_GetPerformanceCounter();
	}
	
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
