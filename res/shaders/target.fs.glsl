layout(location=1) uniform vec3 edgeColor;

layout(location=0) out vec4 color_out;

in vec3 worldPos_v;
in vec3 normal_v;
in vec3 normal1_v;
in vec3 normal2_v;
in vec3 normal3_v;

layout(binding=0) uniform sampler2D mpColor;
layout(binding=1) uniform sampler2D mpDepth;
layout(binding=2) uniform sampler2D targetNoise;

#include rendersettings.glh
#include fog.glh

vec2 screenCoord;

vec3 worldPosFromDepth(float depthH) {
	vec4 h = vec4(screenCoord * 2 - 1, depthH * 2 - 1, 1);
	vec4 d = rs.vpMatrixInv * h;
	return d.xyz / d.w;
}

vec3 sampleNoise(vec3 normal) {
	vec3 noiseBlend = abs(normal);
	noiseBlend /= (noiseBlend.x + noiseBlend.y + noiseBlend.z);
	
	return
		texture(targetNoise, normal.yz * 0.7).rgb * noiseBlend.x + 
		texture(targetNoise, normal.xz * 0.7).rgb * noiseBlend.y +
		texture(targetNoise, normal.xy * 0.7).rgb * noiseBlend.z;
}

void main() {
	vec3 ns = (sampleNoise(normal1_v) + sampleNoise(normal2_v) + sampleNoise(normal3_v)) / 3;
	vec3 normal = normalize(normal_v);
	
	vec3 toEye = normalize(rs.cameraPos - worldPos_v);
	float d = abs(dot(toEye, normal));
	vec2 displace = (ns.yz * 2 - 1) * 25;
	
	screenCoord = (vec2(gl_FragCoord.xy) + displace) / vec2(textureSize(mpColor, 0));
	
	vec3 mpWorldPos = worldPosFromDepth(texture(mpDepth, screenCoord).r);
	vec3 mpColor = texture(mpColor, screenCoord).rgb;
	
	
	float intensity = pow(clamp(1 - d * 0.8 + ns.x * 0.4, 0, 1), 5);
	
	vec3 color = edgeColor * 3 * (0.25 + intensity);
	float alpha = mix(0.1, 1.0, clamp(intensity, 0, 1));
	
	color_out = vec4(mix(fog(mpColor, distance(worldPos_v, mpWorldPos)), color, alpha), 1);
}
