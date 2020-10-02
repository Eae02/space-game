#include deferred.glh

in vec2 texcoord_v;
in vec3 normal_v;
in vec3 tangent_v;

layout(location=0) out vec4 color1_out;
layout(location=1) out vec4 color2_out;

layout(binding=0) uniform sampler2D diffuseMapAndAO;
layout(binding=1) uniform sampler2D normalMapAndSpecular;

layout(location=1) uniform vec3 specSettings;

void main() {
	vec3 smoothNormal = normalize(normal_v);
	vec3 tangent = normalize(tangent_v - smoothNormal * dot(tangent_v, smoothNormal));
	mat3 tbnMatrix = mat3(tangent, cross(tangent, smoothNormal), smoothNormal);
	
	MaterialData matData;
	
	vec4 nmValue = texture(normalMapAndSpecular, texcoord_v);
	vec3 nmNormal = nmValue.xyz * (255.0 / 128.0) - 1.0;
	matData.normal = normalize(tbnMatrix * nmNormal);
	matData.specIntensity = mix(specSettings.x, specSettings.y, nmValue.a);
	matData.specExponent = specSettings.z;
	
	vec4 diffuseAndAO = texture(diffuseMapAndAO, texcoord_v);
	matData.diffuse = diffuseAndAO.rgb;
	matData.ambientOcclusion = diffuseAndAO.a;
	
	color1_out = packMaterialData1(matData);
	color2_out = packMaterialData2(matData);
}
