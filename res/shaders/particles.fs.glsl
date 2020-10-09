in vec2 position_v;

layout(location=0) out vec4 color_out;

const vec3 COLOR = pow(vec3(0.242281139, 0.617206633, 0.830769956), vec3(0.2)) * 0.6;

void main() {
	color_out = vec4(COLOR, 0.8 * max(1 - length(position_v), 0));
}
