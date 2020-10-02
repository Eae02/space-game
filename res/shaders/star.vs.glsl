layout(location=0) in vec2 position_in;
layout(location=1) in vec4 dirAndDist_in;
layout(location=2) in vec3 color_in;

out vec3 color_v;
out vec4 dirAndDist_v;
out vec3 eyeVector_v;

#include rendersettings.glh

layout(location=0) uniform float aspectRatio;

void main() {
	color_v = color_in;
	dirAndDist_v = dirAndDist_in;
	
	vec3 dir = dirAndDist_in.xyz;
	vec4 ndcPos4 = rs.vpMatrix * vec4(dir, 0);
	vec2 pos2 = ndcPos4.xy / ndcPos4.w + position_in * vec2(1, aspectRatio) * 0.01;
	gl_Position = vec4(pos2, 0.9999, 1);
	
	vec4 nearFrustumVertexWS = rs.vpMatrixInv * vec4(pos2, 0, 1);
	vec4 farFrustumVertexWS = rs.vpMatrixInv * vec4(pos2, 1, 1);
	nearFrustumVertexWS.xyz /= nearFrustumVertexWS.w;
	farFrustumVertexWS.xyz /= farFrustumVertexWS.w;
	eyeVector_v = normalize(farFrustumVertexWS.xyz - nearFrustumVertexWS.xyz);
}
