in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D texIn;

layout(location=0) uniform float minBrightness;

void main() {
	color_out =
		textureOffset(texIn, screenCoord_v, ivec2(0, 0)) +
		textureOffset(texIn, screenCoord_v, ivec2(0, 1)) +
		textureOffset(texIn, screenCoord_v, ivec2(1, 0)) +
		textureOffset(texIn, screenCoord_v, ivec2(1, 1));
	color_out = max(color_out * 0.25 - minBrightness, vec4(0));
}
