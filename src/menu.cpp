#include "target.hpp"

std::string instructions[] = {
	"In this game you fly a spaceship through an asteroid field in order to reach checkpoints.",
	"The game ends if you collide with an asteroid or if more than a minute passes between two checkpoints.",
	"There are 3 different types of checkpoints: blue, yellow and red. Yellow and red checkpoints are placed further away but give more points.",
	"The number of points you get when reaching a checkpoint is:",
	">For blue: " + std::to_string(BLUE_BASE_PTS) + "pts + " + std::to_string(BLUE_TIME_PTS) + "pts per second remaining",
	">For yellow: " + std::to_string(YELLOW_BASE_PTS) + "pts + " + std::to_string(YELLOW_TIME_PTS) + "pts per second remaining",
	">For red: " + std::to_string(RED_BASE_PTS) + "pts + " + std::to_string(RED_TIME_PTS) + "pts per second remaining",
	"-",
	"#Controls",
	">W/S Pitch up / down (can be reversed in settings.txt)",
	">A/D Bank left / right",
	">Q/E Roll left / right",
	">Space Accelerate",
	">Ctrl Decelerate",
	">X Toggle minimal ui (useful when flying fast)",
	">C Stop ship",
};
