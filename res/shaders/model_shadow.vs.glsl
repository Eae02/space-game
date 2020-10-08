layout(location=0) in vec3 position_in;

layout(location=0) uniform mat4 worldAndShadowMatrix;

void main() {
	gl_Position = worldAndShadowMatrix * vec4(position_in, 1);
}
