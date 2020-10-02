layout(location=0) in vec3 position_in;
layout(location=1) in vec3 normal_in;
layout(location=2) in vec3 tangent_in;
layout(location=3) in vec2 texcoord_in;

out vec2 texcoord_v;
out vec3 normal_v;
out vec3 tangent_v;

layout(location=0) uniform mat4 worldMatrix;

#include rendersettings.glh

void main() {
	vec3 worldPos = (worldMatrix * vec4(position_in, 1)).xyz;
	texcoord_v = texcoord_in;
	normal_v = (worldMatrix * vec4(normal_in, 0)).xyz;
	tangent_v = (worldMatrix * vec4(tangent_in, 0)).xyz;
	gl_Position = rs.vpMatrix * vec4(worldPos, 1);
}
