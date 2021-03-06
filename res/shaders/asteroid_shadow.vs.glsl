#extension GL_ARB_shader_draw_parameters : require

layout(location=0) in vec3 position_in;

layout(binding=0, std430) readonly buffer AsteroidTransformsBuf {
	mat4 asteroidTransforms[];
};

layout(location=0) uniform mat4 shadowMatrix;

void main() {
	vec3 worldPos = (asteroidTransforms[gl_DrawIDARB] * vec4(position_in, 1)).xyz;
	gl_Position = shadowMatrix * vec4(worldPos, 1);
}
