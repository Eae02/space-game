in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D texIn;
layout(binding=1) uniform sampler2D depthSampler;

const float exposure = 1;

#define NO_CALC_SHADOW
#include rendersettings.glh
#include lighting.glh

const uint grSampleCount = 50;
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
		if (depth > 0.99995) {
			light += illuminationDecay;
		}
		illuminationDecay *= grLightDecay;
	}
	
	return light * grBrightness;
}

vec3 worldPosFromDepth(float depthH) {
	vec4 h = vec4(screenCoord_v * 2 - 1, depthH * 2 - 1, 1);
	vec4 d = rs.vpMatrixInv * h;
	return d.xyz / d.w;
}

const float FOG_START = 200;
#include fog.glh

layout(location=0) uniform vec3 vignetteColor;
layout(location=1) uniform vec3 colorScale;

void main() {
	float depthH = texture(depthSampler, screenCoord_v).r;
	vec3 worldPos = worldPosFromDepth(depthH);
	
	vec4 color4 = texture(texIn, screenCoord_v);
	vec3 color = color4.rgb;
	bool isShip = color4.a == 1;
	
	color += calcGodRays() * rs.sunColor;
	
	color = fog(color, distance(worldPos, rs.cameraPos) - FOG_START);
	
	color_out = vec4(vec3(1.0) - exp(-exposure * color), 1.0);
	
	float vignette = pow(length(screenCoord_v - 0.5) / length(vec2(0.55)), 2.0);
	color_out.rgb = mix(color_out.rgb, vignetteColor, vignette) * colorScale;
}
