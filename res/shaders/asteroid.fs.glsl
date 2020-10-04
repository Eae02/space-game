#include rendersettings.glh
#include lighting.glh

in vec3 worldPos_v;
in vec3 texPos_v;
in vec3 normal_v;
flat in int drawIndex_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D diffuseMapAndAO;
layout(binding=1) uniform sampler2D normalMap;

const float specIntensity = 0.2;
const float specExponent = 20;

layout(binding=0, std430) readonly buffer AsteroidTransformsBuf {
	mat4 asteroidTransforms[];
};

const vec3 SKY_COLOR = vec3(0.242281139, 0.617206633, 0.830769956);
const float FOG_DENSITY = 0.0003;
const float FOG_START = 200;

void main() {
	vec3 normal = normalize(normal_v);
	
	vec3 blend = abs(normal);
	blend /= blend.x + blend.y + blend.z;
	
	vec4 diffuseAndAO = 
		texture(diffuseMapAndAO, texPos_v.zy) * blend.x +
		texture(diffuseMapAndAO, texPos_v.xz) * blend.y +
		texture(diffuseMapAndAO, texPos_v.xy) * blend.z;
	
	vec3 tnormalX = vec3(texture(normalMap, texPos_v.zy).rg * 2 - 1 + normal.zy, normal.x);
	vec3 tnormalY = vec3(texture(normalMap, texPos_v.xz).rg * 2 - 1 + normal.xz, normal.y);
	vec3 tnormalZ = vec3(texture(normalMap, texPos_v.xy).rg * 2 - 1 + normal.xy, normal.z);
	vec3 worldNormal = normalize((asteroidTransforms[drawIndex_v] * vec4(tnormalX.zyx * blend.x + tnormalY.xzy * blend.y + tnormalZ.xyz * blend.z, 0)).xyz);
	
	color_out = vec4(calculateLighting(worldPos_v, worldNormal, diffuseAndAO, specIntensity, specExponent), 1);
	
	color_out.rgb = mix(SKY_COLOR, color_out.rgb, exp(-max(distance(worldPos_v, rs.cameraPos) - FOG_START, 0.0) * FOG_DENSITY));
}
