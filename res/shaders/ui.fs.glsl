layout(binding=0) uniform sampler2D tex;

in vec2 texCoord_v;
in vec4 color_v;

out vec4 color_out;

void main() {
	color_out = texture(tex, texCoord_v) * color_v;
}
