layout(location=0) in vec3 position_in;
layout(location=1) in vec4 normal_in;
layout(location=2) in vec4 tangent_in;
layout(location=3) in vec2 texcoord_in;

out vec3 worldPos_v;
out vec2 texcoord_v;
out vec3 normal_v;
out vec3 tangent_v;

layout(location=0) uniform mat4 worldMatrix;

#include rendersettings.glh

void main() {
	worldPos_v = (worldMatrix * vec4(position_in, 1)).xyz;
	texcoord_v = texcoord_in;
	normal_v = (worldMatrix * normal_in).xyz;
	tangent_v = (worldMatrix * tangent_in).xyz;
	gl_Position = rs.vpMatrix * vec4(worldPos_v, 1);
}
