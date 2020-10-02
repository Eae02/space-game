layout(location=0) out vec4 color_out;

layout(location=1) uniform vec3 emissiveColor;

void main() {
	color_out = vec4(emissiveColor, 0);
}
