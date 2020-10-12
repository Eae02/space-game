in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D inColor;
layout(binding=1) uniform sampler2D inDepth;

void main() {
	color_out = texture(inColor, screenCoord_v);
	gl_FragDepth = texture(inDepth, screenCoord_v).r;
}
