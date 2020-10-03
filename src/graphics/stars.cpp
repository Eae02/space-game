#include "stars.hpp"
#include "shader.hpp"

#include <random>

static Shader starShader;

static GLuint starVao;
static GLuint starQuadVertexBuffer;
static GLuint starsVertexBuffer;

static float starQuadVertices[] = {
	-1, -1,
	1, -1,
	-1, 1,
	1, 1
};

struct Star {
	glm::vec3 dir;
	float distance;
	glm::vec3 color;
};

static constexpr size_t NUM_STARS = 2000;

static void generateStars(Star* stars) {
	std::mt19937 rand;
	std::uniform_real_distribution<float> thetaDist(0, M_PI * 2);
	std::uniform_real_distribution<float> pitchDist(-1.0f, 1.0f);
	
	std::uniform_real_distribution<float> intensityDist(0.1f, 2.0f);
	std::uniform_real_distribution<float> distanceDist(300000, 1000000);
	
	glm::vec3 colorB = glm::convertSRGBToLinear(glm::vec3(0.92f, 0.98f, 1.0f));
	glm::vec3 colorR = glm::convertSRGBToLinear(glm::vec3(1.0f, 0.97f, 0.97f));
	std::uniform_real_distribution<float> colorFadeDist(0.0f, 1.0f);
	const float colorPow = 3.0f;
	
	for (size_t i = 0; i < NUM_STARS; i++) {
		const float theta = thetaDist(rand);
		const float cosPitch = pitchDist(rand);
		const float sinPitch = std::sin(std::acos(cosPitch));
		
		stars[i].dir = glm::vec3(std::cos(theta) * sinPitch, std::sin(theta) * sinPitch, cosPitch);
		stars[i].color = glm::pow(glm::mix(colorR, colorB, colorFadeDist(rand)), glm::vec3(colorPow)) * intensityDist(rand);
		stars[i].distance = distanceDist(rand);
	}
}

void stars::initialize() {
	starShader.attachStage(GL_VERTEX_SHADER, "star.vs.glsl");
	starShader.attachStage(GL_FRAGMENT_SHADER, "star.fs.glsl");
	starShader.link("Stars");
	
	glCreateBuffers(1, &starQuadVertexBuffer);
	glNamedBufferStorage(starQuadVertexBuffer, sizeof(starQuadVertices), starQuadVertices, 0);
	
	Star stars[NUM_STARS];
	generateStars(stars);
	
	glCreateBuffers(1, &starsVertexBuffer);
	glNamedBufferStorage(starsVertexBuffer, sizeof(stars), stars, 0);
	
	glCreateVertexArrays(1, &starVao);
	
	// position attribute
	glEnableVertexArrayAttrib(starVao, 0);
	glVertexArrayAttribBinding(starVao, 0, 0);
	glVertexArrayAttribFormat(starVao, 0, 2, GL_FLOAT, false, 0);
	
	// direction and distance attribute
	glEnableVertexArrayAttrib(starVao, 1);
	glVertexArrayAttribBinding(starVao, 1, 1);
	glVertexArrayAttribFormat(starVao, 1, 4, GL_FLOAT, false, offsetof(Star, dir));
	
	// color attribute
	glEnableVertexArrayAttrib(starVao, 2);
	glVertexArrayAttribBinding(starVao, 2, 1);
	glVertexArrayAttribFormat(starVao, 2, 3, GL_FLOAT, false, offsetof(Star, color));
	
	glVertexArrayVertexBuffer(starVao, 0, starQuadVertexBuffer, 0, sizeof(float) * 2);
	glVertexArrayBindingDivisor(starVao, 0, 0);
	glVertexArrayVertexBuffer(starVao, 1, starsVertexBuffer, 0, sizeof(Star));
	glVertexArrayBindingDivisor(starVao, 1, 1);
}

void stars::draw(int resX, int resY) {
	glEnable(GL_BLEND);
	glDepthMask(0);
	
	starShader.use();
	glUniform1f(0, (float)resX / (float)resY);
	glBindVertexArray(starVao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, NUM_STARS);
	
	glDisable(GL_BLEND);
	glDepthMask(1);
}
