in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D texIn;
layout(binding=1) uniform sampler2D depthSampler;
layout(binding=2) uniform sampler2DArray shadowMap;

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
		if (depth > 1.0 - 1E-6) {
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

const vec3 SKY_COLOR = vec3(0.242281139, 0.617206633, 0.830769956) * 0.7;
const float FOG_DENSITY = 0.00025;
const float FOG_START = 300;
const float FOG_END = 1500;

#ifdef ENABLE_MOTION_BLUR
layout(location=0) uniform mat4 prevViewProj;
layout(location=1) uniform float motionBlurRadius;
#endif

const float BLUR_KERNEL[] = float[] (0.382928, 0.241732, 0.060598, 0.005977, 0.000229);

float fog(float dist) {
	return exp((clamp(dist, FOG_START, FOG_END) - FOG_START) * -FOG_DENSITY);
}

void main() {
	float depthH = texture(depthSampler, screenCoord_v).r;
	vec3 worldPos = worldPosFromDepth(depthH);
	
	vec4 color4 = texture(texIn, screenCoord_v);
	vec3 color = color4.rgb;
	bool isShip = color4.a == 1;
	
#ifdef ENABLE_MOTION_BLUR
	if (!isShip) {
		vec4 prevScreenPos4 = prevViewProj * vec4(worldPos, 1);
		vec2 prevScreenPos = (prevScreenPos4.xy / prevScreenPos4.w) * 0.5 + 0.5;
		vec2 blurVector = (prevScreenPos - screenCoord_v) * motionBlurRadius;
		color *= BLUR_KERNEL[0];
		float weightSum = BLUR_KERNEL[0];
		for (int i = 1; i < BLUR_KERNEL.length(); i++) {
			vec4 c1 = texture(texIn, screenCoord_v + blurVector * i);
			vec4 c2 = texture(texIn, screenCoord_v - blurVector * i);
			color += c1.rgb * (1 - c1.a) * BLUR_KERNEL[i];
			color += c2.rgb * (1 - c2.a) * BLUR_KERNEL[i];
			weightSum += (2 - c1.a - c2.a) * BLUR_KERNEL[i];
		}
		color /= weightSum;
	}
#endif
	
	color += calcGodRays() * rs.sunColor;
	
	color = mix(SKY_COLOR, color, fog(distance(worldPos, rs.cameraPos)));
	
	color_out = vec4(vec3(1.0) - exp(-exposure * color), 1.0);
	
	float vignette = pow(length(screenCoord_v - 0.5) / length(vec2(0.55)), 2.0);
	color_out *= 1.0 - vignette;
}
