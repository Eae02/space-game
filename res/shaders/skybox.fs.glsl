in vec3 eyeVector_v;

layout(location=0) out vec4 color_out;

layout(binding=0) uniform samplerCube skybox;

void main() {
	color_out = texture(skybox, eyeVector_v) * 10;
}
