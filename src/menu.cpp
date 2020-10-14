#include "menu.hpp"
#include "target.hpp"
#include "utils.hpp"
#include "graphics/asteroids.hpp"
#include "graphics/renderer.hpp"
#include "graphics/ui.hpp"

static std::string instructions[] = {
	"In this game you fly a spaceship through an",
	"asteroid field in order to reach checkpoints.",
	"",
	"The game ends if you collide with an asteroid",
	"or if more than a minute passes between two checkpoints.",
	"",
	"There are 3 different types of checkpoints: blue, yellow and red.",
	"Yellow and red checkpoints are placed further away,",
	"and so are more difficult to reach in time, but give more points.",
	"",
	"The number of points you get when reaching a checkpoint is:",
	">Blue: " + std::to_string(BLUE_BASE_PTS) + " points + " + std::to_string(BLUE_TIME_PTS) + " per second remaining",
	">Yellow: " + std::to_string(YELLOW_BASE_PTS) + " points + " + std::to_string(YELLOW_TIME_PTS) + " per second remaining",
	">Red: " + std::to_string(RED_BASE_PTS) + " points + " + std::to_string(RED_TIME_PTS) + " per second remaining",
	"",
	"Controls:",
	">W/S - Pitch down / up",
	">A/D - Bank left / right",
	">Q/E - Roll left / right",
	">Space - Accelerate",
	">Alt - Decelerate",
	">X - Toggle minimal ui (useful when flying fast)",
	">C - Stop ship",
};

static const glm::vec3 cameraFlyDir = glm::normalize(glm::vec3(0.5f, 1, -0.2f));
static constexpr float CAMERA_FLY_SPEED = 60;
static constexpr float CAMERA_ROLL_SPEED = 0.1f;

static ui::Button playButton = { "Play" };
static ui::Button instructionsButton = { "Instructions" };
static ui::Button quitButton = { "Quit" };

static ui::Button instBackButton = { "Back" };

static bool inInstructionsMenu = false;

void menu::updateAndDraw(uint32_t drawableWidth, uint32_t drawableHeight, const InputState& is, bool& startGame, bool& quit) {
	glm::vec2 centerScreen(drawableWidth / 2.0f, drawableHeight / 2.0f);
	
	if (inInstructionsMenu) {
		float y = centerScreen.y + 250;
		for (const std::string& inst : instructions) {
			if (inst.empty()) {
				y -= ui::FONT_SIZE / 2;
				continue;
			}
			
			float drawX = centerScreen.x - 300;
			std::string textToDraw = inst;
			if (textToDraw[0] == '>') {
				drawX += 40;
				textToDraw = textToDraw.substr(1);
			}
			ui::drawText(textToDraw, glm::vec2(drawX, y), glm::vec4(1));
			
			y -= ui::FONT_SIZE + 5;
		}
		
		instBackButton.pos = glm::vec2(centerScreen.x, y - 40);
		if (instBackButton(is)) {
			inInstructionsMenu = false;
		}
	} else {
		playButton.pos = glm::vec2(centerScreen.x, centerScreen.y + 50);
		instructionsButton.pos = glm::vec2(centerScreen.x, centerScreen.y);
		quitButton.pos = glm::vec2(centerScreen.x, centerScreen.y - 50);
		
		if (playButton(is))
			startGame = true;
		if (instructionsButton(is))
			inInstructionsMenu = true;
		if (quitButton(is))
			quit = true;
	}
}

void menu::initRenderSettings(uint32_t drawableWidth, uint32_t drawableHeight, RenderSettings& renderSettings) {
	gameTime += dt;
	
	float roll = gameTime * CAMERA_ROLL_SPEED;
	glm::vec3 up(std::sin(roll), 0, std::cos(roll));
	
	glm::vec3 cameraPos = cameraFlyDir * gameTime * CAMERA_FLY_SPEED;
	glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFlyDir, up);
	glm::mat4 viewMatrixInv = glm::inverse(viewMatrix);
	
	constexpr float FOV = M_PI / 2;
	glm::mat4 projMatrix = glm::perspectiveFov(FOV, (float)drawableWidth, (float)drawableHeight, Z_NEAR, Z_FAR);
	glm::mat4 inverseProjMatrix = glm::inverse(projMatrix);
	
	renderSettings.vpMatrix = projMatrix * viewMatrix;
	renderSettings.vpMatrixInverse = viewMatrixInv * inverseProjMatrix;
	renderSettings.cameraPos = cameraPos;
	renderSettings.gameTime = gameTime;
	renderSettings.sunColor = SUN_COLOR;
	renderSettings.sunDir = SUN_DIR;
	renderSettings.plColor = glm::vec3(0);
	renderSettings.plPosition = glm::vec3(0);
	
	updateAsteroidWrapping(cameraPos);
}
