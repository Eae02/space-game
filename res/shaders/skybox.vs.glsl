const vec2 positions[] = vec2[] (
	vec2(-1, 1), vec2(-1, -1), vec2(1, -1),
	vec2(-1, 1), vec2(1, -1), vec2(1, 1)
);

out vec3 eyeVector_v;

#include rendersettings.glh

void main() {
	gl_Position = vec4(positions[gl_VertexID], 0.99999, 1);
	vec4 farFrustumCoord = rs.vpMatrixInv * vec4(positions[gl_VertexID], 1, 1);
	vec4 nearFrustumCoord = rs.vpMatrixInv * vec4(positions[gl_VertexID], -1, 1);
	eyeVector_v = farFrustumCoord.xyz / farFrustumCoord.w - nearFrustumCoord.xyz / nearFrustumCoord.w;
}
