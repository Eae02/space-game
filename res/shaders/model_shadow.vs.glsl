layout(location=0) in vec3 position_in;

layout(location=0) uniform mat4 worldMatrix;

#include rendersettings.glh

void main() {
	vec3 worldPos = (worldMatrix * vec4(position_in, 1)).xyz;
	gl_Position = rs.shadowMatrix * vec4(worldPos, 1);
}
