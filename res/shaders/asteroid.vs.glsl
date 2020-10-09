#extension GL_ARB_shader_draw_parameters : require

layout(location=0) in vec3 position_in;
layout(location=1) in vec3 normal_in;

#include rendersettings.glh

out vec3 normal_v;
out vec3 worldPos_v;
out vec3 texPos_v;
flat out int drawIndex_v;

const float textureScale = 0.05;

layout(binding=0, std430) readonly buffer AsteroidTransformsBuf {
	mat4 asteroidTransforms[];
};

void main() {
	drawIndex_v = gl_DrawIDARB;
	mat4 worldTransform = asteroidTransforms[gl_DrawIDARB];
	texPos_v = position_in * length(worldTransform[0]) * textureScale;
	worldPos_v = (worldTransform * vec4(position_in, 1)).xyz;
	normal_v = normal_in;
	gl_Position = rs.vpMatrix * vec4(worldPos_v, 1);
}
