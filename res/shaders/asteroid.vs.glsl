#extension GL_ARB_shader_draw_parameters : require

layout(location=0) in vec3 position_in;
layout(location=1) in vec3 lowerLodPos_in;
layout(location=2) in vec3 normal_in;

#include rendersettings.glh
#include asteroid_vs.glh

out vec3 normal_v;
out vec3 worldPos_v;
out vec3 texPos_v;
flat out int drawIndex_v;

const float textureScale = 0.05;

void main() {
	vec3 scaledPos;
	worldPos_v = transformToWorld(position_in, lowerLodPos_in, NORMAL_LOD_BIAS, scaledPos);
	texPos_v = scaledPos * textureScale;
	normal_v = normal_in;
	drawIndex_v = gl_DrawIDARB;
	gl_Position = rs.vpMatrix * vec4(worldPos_v, 1);
}
