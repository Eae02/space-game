layout(location=0) in vec2 position_in;
layout(location=1) in vec4 srcRect_in;
layout(location=2) in vec4 dstRect_in;
layout(location=3) in vec4 color_in;

out vec2 texCoord_v;
out vec4 color_v;

layout(location=0) uniform vec2 texelSize;

void main() {
	gl_Position = vec4(mix(dstRect_in.xy, dstRect_in.zw, position_in) * 2 - 1, 0, 1);
	texCoord_v = mix(srcRect_in.xy, srcRect_in.zw, position_in) * texelSize;
	color_v = color_in;
}
