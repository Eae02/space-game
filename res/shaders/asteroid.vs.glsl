layout(location=0) in vec3 position_in;
layout(location=1) in vec3 normal_in;
//layout(location=2) in mat4 worldTransform_in;

#include rendersettings.glh

out vec3 normal_v;
out vec3 worldPos_v;

layout(location=0) uniform mat4 worldTransform_in;

void main() {
	worldPos_v = (worldTransform_in * vec4(position_in, 1)).xyz;
	normal_v = (worldTransform_in * vec4(normal_in, 0)).xyz;
	gl_Position = rs.vpMatrix * vec4(worldPos_v, 1);
}
