#include rendersettings.glh

in vec3 worldPos_v;
in vec3 texPos_v;
in vec3 normal_v;
flat in int drawIndex_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D diffuseMapAndAO;
layout(binding=1) uniform sampler2D normalMap;
layout(binding=2) uniform sampler2DArrayShadow shadowMap;

#include lighting.glh

const float specIntensity = 0.2;
const float specExponent = 20;

layout(binding=0, std430) readonly buffer AsteroidTransformsBuf {
	mat4 asteroidTransforms[];
};

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
	
	color_out = vec4(calculateLighting(worldPos_v, worldNormal, diffuseAndAO, specIntensity, specExponent), 0);
}
