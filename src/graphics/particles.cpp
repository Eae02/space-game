#include "particles.hpp"
#include "shader.hpp"
#include "renderer.hpp"
#include "../utils.hpp"
#include "../settings.hpp"

#include <random>

constexpr float PARTICLE_BOX_SIZE = 300;
constexpr size_t NUM_PARTICLES = 6000;

static GLuint particlesVao;
static GLuint particlesBuffer;

static Shader particlesShader;

void initializeParticles() {
	std::mt19937 rng(15);
	std::uniform_real_distribution<float> posDist(0, PARTICLE_BOX_SIZE);
	std::uniform_real_distribution<float> sizeDist(0.1f, 0.2f);
	glm::vec4 particleData[NUM_PARTICLES];
	for (size_t i = 0; i < NUM_PARTICLES; i++) {
		particleData[i] = glm::vec4(posDist(rng), posDist(rng), posDist(rng), sizeDist(rng));
	}
	
	glCreateBuffers(1, &particlesBuffer);
	glNamedBufferStorage(particlesBuffer, sizeof(particleData), particleData, 0);
	
	particlesShader.attachStage(GL_VERTEX_SHADER, "particles.vs.glsl");
	particlesShader.attachStage(GL_FRAGMENT_SHADER, "particles.fs.glsl");
	particlesShader.link("Particles");
	glProgramUniform1f(particlesShader.program, 2, PARTICLE_BOX_SIZE);
	
	glCreateVertexArrays(1, &particlesVao);
	glEnableVertexArrayAttrib(particlesVao, 0);
	glVertexArrayAttribFormat(particlesVao, 0, 4, GL_FLOAT, false, 0);
	glVertexArrayBindingDivisor(particlesVao, 0, 1);
	glVertexArrayVertexBuffer(particlesVao, 0, particlesBuffer, 0, sizeof(glm::vec4));
}

void drawParticles(float time, const glm::vec3& cameraPos) {
	glm::vec3 boxOffset = glm::floor(cameraPos / PARTICLE_BOX_SIZE) * PARTICLE_BOX_SIZE;
	glm::vec3 wrappingOffset = PARTICLE_BOX_SIZE * 1.5f - (cameraPos - boxOffset);
	glm::vec3 globalOffset = cameraPos - PARTICLE_BOX_SIZE * 0.5f;
	
	particlesShader.use();
	glUniform3fv(0, 1, (const float*)&wrappingOffset);
	glUniform3fv(1, 1, (const float*)&globalOffset);
	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);
	glBindVertexArray(particlesVao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, NUM_PARTICLES);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glDepthMask(1);
}
