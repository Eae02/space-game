#include rendersettings.glh

in vec3 worldPos_v;
in vec2 texcoord_v;
in vec3 normal_v;
in vec3 tangent_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D diffuseMapAndAO;
layout(binding=1) uniform sampler2D normalMapAndSpecular;
layout(binding=2) uniform sampler2DArrayShadow shadowMap;

#include lighting.glh

layout(location=1) uniform vec3 specSettings;

void main() {
	vec3 normal = normalize(normal_v);
	vec3 tangent = normalize(tangent_v - normal * dot(tangent_v, normal));
	mat3 tbnMatrix = mat3(tangent, cross(tangent, normal), normal);
	
	vec4 nmValue = texture(normalMapAndSpecular, texcoord_v);
	vec3 nmNormal = nmValue.xyz * (255.0 / 128.0) - 1.0;
	normal = normalize(tbnMatrix * nmNormal);
	float specIntensity = mix(specSettings.x, specSettings.y, nmValue.a);
	
	vec4 diffuseAndAO = texture(diffuseMapAndAO, texcoord_v);
	
	color_out = vec4(calculateLighting(worldPos_v, normal, diffuseAndAO, specIntensity, specSettings.z), 1);
}
