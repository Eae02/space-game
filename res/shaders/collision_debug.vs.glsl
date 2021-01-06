layout(location=0) in vec3 position_in;
layout(location=1) in vec4 color_in;

#include rendersettings.glh

out vec4 color_v;

void main() {
	color_v = color_in;
	gl_Position = rs.vpMatrix * vec4(position_in, 1);
}
