in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D texIn;
layout(binding=1) uniform sampler2D depthSampler;
layout(binding=2) uniform samplerCube skybox;
layout(binding=3) uniform sampler2DArrayShadow shadowMap;

const float exposure = 1;

#include rendersettings.glh
#include lighting.glh

const uint grSampleCount = 32;
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

layout(location=0) uniform uint enableVolLight;

vec3 worldPosFromDepth(float depthH) {
	vec4 h = vec4(screenCoord_v * 2 - 1, depthH * 2 - 1, 1);
	vec4 d = rs.vpMatrixInv * h;
	return d.xyz / d.w;
}

const vec3 SKY_COLOR = vec3(0.242281139, 0.617206633, 0.830769956);
const float FOG_DENSITY = 0.0003;
const float FOG_START = 200;

float fog(float dist) {
	return exp(-max(dist - FOG_START, 0.0) * FOG_DENSITY);
}

void main() {
	vec4 farFrustumVertex = rs.vpMatrixInv * vec4(screenCoord_v * 2 - 1, 1, 1);
	vec4 nearFrustumVertex = rs.vpMatrixInv * vec4(screenCoord_v * 2 - 1, 0, 1);
	vec3 eyeVector = farFrustumVertex.xyz / farFrustumVertex.w - nearFrustumVertex.xyz / nearFrustumVertex.w;
	
	vec3 color = texture(texIn, screenCoord_v).rgb + calcGodRays() * rs.sunColor;
	
	float depthH = texture(depthSampler, screenCoord_v).r;
	vec3 worldPos = worldPosFromDepth(depthH);
	
	color = mix(SKY_COLOR, color, fog(distance(worldPos, rs.cameraPos)));
	
	if (enableVolLight == 1) {
		const int VOL_LIGHT_SAMPLES = 64;
		vec3 vlSampleRay = worldPos - rs.cameraPos;
		float vlSampleRayLen = min(300, max(length(vlSampleRay) - 5, 0));
		vec3 vlSampleRayStep = normalize(vlSampleRay) * vlSampleRayLen / VOL_LIGHT_SAMPLES;
		float blockedSamples = 0;
		for (int i = 1; i <= VOL_LIGHT_SAMPLES; i++) {
			vec3 samplePos = rs.cameraPos + vlSampleRayStep * float(i);
			vec4 coords;
			float inScatter = 1;
			if (getShadowMapCoords(samplePos, coords)) {
				blockedSamples += (1 - texture(shadowMap, coords).r);
			}
		}
		color *= exp(-blockedSamples * 0.025);
	}
	
	color_out = vec4(vec3(1.0) - exp(-exposure * color), 1.0);
	
	float vignette = pow(length(screenCoord_v - 0.5) / length(vec2(0.55)), 2.0);
	color_out *= 1.0 - vignette;
}
