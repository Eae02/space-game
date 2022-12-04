#extension GL_ARB_shader_draw_parameters : require

layout(location=0) in vec3 position_in;
layout(location=1) in vec3 lowerLodPos_in;

#include asteroid_vs.glh

layout(location=0) uniform mat4 shadowMatrix;

void main() {
	vec3 scaledPos;
	gl_Position = shadowMatrix * vec4(transformToWorld(position_in, lowerLodPos_in, SHADOW_LOD_BIAS, scaledPos), 1);
}
