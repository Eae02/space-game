layout(location=0) in vec4 position_in;

const vec2 vertices[] = vec2[] (
	vec2(-1, -1), vec2(-1, 1), vec2(1, -1), vec2(-1, 1), vec2(1, 1), vec2(1, -1)
);

out vec2 position_v;

#include rendersettings.glh

layout(location=0) uniform vec3 wrappingOffset;
layout(location=1) uniform vec3 globalOffset;
layout(location=2) uniform float wrapModulo;

void main() {
	vec3 particlePos = mod(position_in.xyz + wrappingOffset, vec3(wrapModulo)) + globalOffset;
	
	position_v = vertices[gl_VertexID];
	vec3 toEye = normalize(rs.cameraPos - particlePos);
	vec3 left = normalize(cross(toEye, vec3(0, 1, 0)));
	vec3 up = cross(left, toEye);
	vec3 worldPos = particlePos + left * vertices[gl_VertexID].x * position_in.w + up * vertices[gl_VertexID].y * position_in.w;
	gl_Position = rs.vpMatrix * vec4(worldPos, 1);
}
