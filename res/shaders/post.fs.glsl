in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D texIn;
layout(binding=1) uniform sampler2D depthSampler;

const float exposure = 1;

#include rendersettings.glh

const uint grSampleCount = 64;
const float grLightDecay = pow(0.00004, 1.0 / float(grSampleCount));
const float grSunRadius = 15;
const float grBrightness = 5;

float calcGodRays() {
	vec4 ssPosPps = rs.vpMatrix * vec4(-rs.sunDir, 0);
	if (ssPosPps.z < 0)
		return 0;
	vec2 sunScreenPosition = (ssPosPps.xy / ssPosPps.w) * 0.5 + 0.5;
	
	const float viewSize = 700;
	
	vec2 texSize = textureSize(depthSampler, 0);
	vec2 coordMul = vec2(viewSize, viewSize * (texSize.y / texSize.x));;
	
	float distToLight = distance(screenCoord_v * coordMul, sunScreenPosition * coordMul);
	float lightBeginSample = max(1.0 - (grSunRadius / max(distToLight, 0.0001)), 0.0) * grSampleCount;
	
	float illuminationDecay = pow(grLightDecay, floor(lightBeginSample) + 1);
	
	float light = 0.0;
	for (uint i = uint(ceil(lightBeginSample)); i < grSampleCount; i++) {
		vec2 sampleCoord = mix(screenCoord_v, sunScreenPosition, float(i) / float(grSampleCount));
		float depth = texture(depthSampler, sampleCoord).r;
		if (depth > 1.0 - 1E-6) {
			light += illuminationDecay;
		}
		illuminationDecay *= grLightDecay;
	}
	
	return light * grBrightness;
}

void main() {
	vec3 color = texture(texIn, screenCoord_v).rgb + calcGodRays() * rs.sunColor;
	
	color_out = vec4(vec3(1.0) - exp(-exposure * color), 1.0);
	
	float vignette = pow(length(screenCoord_v - 0.5) / length(vec2(0.55)), 2.0);
	color_out *= 1.0 - vignette;
}
