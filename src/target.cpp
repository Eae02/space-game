#include "target.hpp"
#include "utils.hpp"
#include "graphics/sphere.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer.hpp"

constexpr uint32_t TARGET_SPHERE_LOD_LEVEL = 3;
static_assert(TARGET_SPHERE_LOD_LEVEL < NUM_SPHERE_LODS);

static Shader targetShader;
static Shader targetMergeShader;

static GLuint targetVertexBuffer;
static GLuint targetIndexBuffer;
static GLuint targetVertexArray;

static Texture targetNoiseTexture;

void initializeTargetShader() {
	targetShader.attachStage(GL_VERTEX_SHADER, "target.vs.glsl");
	targetShader.attachStage(GL_FRAGMENT_SHADER, "target.fs.glsl");
	targetShader.link("target");
	
	targetMergeShader.attachStage(GL_VERTEX_SHADER, "fullscreen.vs.glsl");
	targetMergeShader.attachStage(GL_FRAGMENT_SHADER, "targetmerge.fs.glsl");
	targetMergeShader.link("targetMerge");
	
	glCreateBuffers(1, &targetVertexBuffer);
	glNamedBufferStorage(
		targetVertexBuffer,
		sphereVertices[TARGET_SPHERE_LOD_LEVEL].size() * sizeof(glm::vec3),
		sphereVertices[TARGET_SPHERE_LOD_LEVEL].data(),
		0
	);
	
	glCreateBuffers(1, &targetIndexBuffer);
	glNamedBufferStorage(
		targetIndexBuffer,
		sphereTriangles[TARGET_SPHERE_LOD_LEVEL].size() * sizeof(glm::uvec3),
		sphereTriangles[TARGET_SPHERE_LOD_LEVEL].data(),
		0
	);
	
	glCreateVertexArrays(1, &targetVertexArray);
	glEnableVertexArrayAttrib(targetVertexArray, 0);
	glVertexArrayAttribBinding(targetVertexArray, 0, 0);
	glVertexArrayAttribFormat(targetVertexArray, 0, 3, GL_FLOAT, false, 0);
	glVertexArrayVertexBuffer(targetVertexArray, 0, targetVertexBuffer, 0, sizeof(glm::vec3));
	glVertexArrayElementBuffer(targetVertexArray, targetIndexBuffer);
	
	glProgramUniform1f(targetShader.program, 2, TARGET_RADIUS);
	
	targetNoiseTexture.load(exeDirPath + "res/textures/targetNoise.png", false, true);
}

glm::vec4 rotations[3] = {
	glm::vec4(1, 0, 0, 0.1f),
	glm::vec4(0, 1, 0, 0.2f),
	glm::vec4(0, 0, 1, -0.1f)
};

void beginDrawTargets(float gameTime) {
	glBindFramebuffer(GL_FRAMEBUFFER, renderer::targetsPassFbo);
	const float clearColor[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, 0, clearColor);
	const float clearDepth[] = { 1 };
	glClearBufferfv(GL_DEPTH, 0, clearDepth);
	
	glm::mat3 rotationMatrices[3];
	for (int i = 0; i < 3; i++) {
		rotationMatrices[i] = glm::mat3(glm::rotate(glm::mat4(), gameTime * rotations[i].w, glm::vec3(rotations[i])));
	}
	
	targetShader.use();
	glBindVertexArray(targetVertexArray);
	renderer::mainPassColorAttachment.bind(0);
	renderer::mainPassDepthAttachment.bind(1);
	targetNoiseTexture.bind(2);
	glUniformMatrix3fv(3, 3, false, (const float*)rotationMatrices);
	
	glDisable(GL_CULL_FACE);
}

void endDrawTargets() {
	glBindFramebuffer(GL_FRAMEBUFFER, renderer::mainPassFbo);
	
	renderer::targetsPassColorAttachment.bind(0);
	renderer::targetsPassDepthAttachment.bind(1);
	
	targetMergeShader.use();
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glDisable(GL_CULL_FACE);
}

void drawTarget(const Target& target) {
	glUniform3fv(0, 1, (const float*)&target.truePos);
	glUniform3fv(1, 1, (const float*)&target.color);
	glDrawElements(GL_TRIANGLES, sphereTriangles[TARGET_SPHERE_LOD_LEVEL].size() * 3, GL_UNSIGNED_INT, nullptr);
}
