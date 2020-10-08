in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D texIn;

layout(location=0) uniform vec2 blurVector;
layout(location=1) uniform float brightnessScale;

const float kernel[] = float[] (0.382928, 0.241732, 0.060598, 0.005977, 0.000229);

void main() {
	color_out = texture(texIn, screenCoord_v) * kernel[0];
	
	for (int i = 1; i < kernel.length(); i++) {
		color_out += texture(texIn, screenCoord_v + blurVector * i) * kernel[i];
		color_out += texture(texIn, screenCoord_v - blurVector * i) * kernel[i];
	}
	
	color_out *= brightnessScale;
}
