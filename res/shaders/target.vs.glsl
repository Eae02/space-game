layout(location=0) in vec3 spherePos_in;

layout(location=0) uniform vec3 position;
layout(location=2) uniform float radius;

out vec3 worldPos_v;
out vec3 normal_v;
out vec3 normal1_v;
out vec3 normal2_v;
out vec3 normal3_v;

layout(location=3) uniform mat3 texRotations[3];

#include rendersettings.glh

void main() {
	normal_v = spherePos_in;
	normal1_v = texRotations[0] * spherePos_in;
	normal2_v = texRotations[1] * spherePos_in;
	normal3_v = texRotations[2] * spherePos_in;
	worldPos_v = position + spherePos_in * radius;
	gl_Position = rs.vpMatrix * vec4(worldPos_v, 1);
}
