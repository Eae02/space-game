#include rendersettings.glh
#include lighting.glh

in vec3 worldPos_v;
in vec3 normal_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D diffuseMapAndAO;
layout(binding=1) uniform sampler2D normalMap;

const float specIntensity = 2;
const float specExponent = 2;
const float textureScale = 0.25;

void main() {
	vec3 normal = normalize(normal_v);
	
	vec3 blend = abs(normal);
	blend /= blend.x + blend.y + blend.z;
	
	vec4 diffuseAndAO = 
		texture(diffuseMapAndAO, textureScale * worldPos_v.zy) * blend.x +
		texture(diffuseMapAndAO, textureScale * worldPos_v.xz) * blend.y +
		texture(diffuseMapAndAO, textureScale * worldPos_v.xy) * blend.z;
	
	vec3 tnormalX = vec3(texture(normalMap, textureScale * worldPos_v.zy).rg * 2 - 1 + normal.zy, normal.x);
	vec3 tnormalY = vec3(texture(normalMap, textureScale * worldPos_v.xz).rg * 2 - 1 + normal.xz, normal.y);
	vec3 tnormalZ = vec3(texture(normalMap, textureScale * worldPos_v.xy).rg * 2 - 1 + normal.xy, normal.z);
	vec3 worldNormal = normalize(
		tnormalX.zyx * blend.x +
		tnormalY.xzy * blend.y +
		tnormalZ.xyz * blend.z
    );
	
	color_out = vec4(calculateLighting(worldPos_v, worldNormal, diffuseAndAO, specIntensity, specExponent), 1);
}
