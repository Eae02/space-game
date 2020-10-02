#include deferred.glh
#include rendersettings.glh

in vec2 screenCoord_v;

out vec4 color_out;

layout(binding=0) uniform sampler2D gbufferColor1;
layout(binding=1) uniform sampler2D gbufferColor2;
layout(binding=2) uniform sampler2D gbufferDepth;
layout(binding=3) uniform samplerCube skybox;

const float C1 = 0.429043;
const float C2 = 0.511664;
const float C3 = 0.743125;
const float C4 = 0.886227;
const float C5 = 0.247708;

const vec3 L00  = vec3( 0.7870665,  0.9379944,  0.9799986);
const vec3 L1m1 = vec3( 0.4376419,  0.5579443,  0.7024107);
const vec3 L10  = vec3(-0.1020717, -0.1824865, -0.2749662);
const vec3 L11  = vec3( 0.4543814,  0.3750162,  0.1968642);
const vec3 L2m2 = vec3( 0.1841687,  0.1396696,  0.0491580);
const vec3 L2m1 = vec3(-0.1417495, -0.2186370, -0.3132702);
const vec3 L20  = vec3(-0.3890121, -0.4033574, -0.3639718);
const vec3 L21  = vec3( 0.0872238,  0.0744587,  0.0353051);
const vec3 L22  = vec3( 0.6662600,  0.6706794,  0.5246173);

void main() {
	float hDepth = texture(gbufferDepth, screenCoord_v).r;
	vec3 worldPos = worldPosFromDepth(hDepth, screenCoord_v, rs.vpMatrixInv);
	if (hDepth == 1) {
		color_out = texture(skybox, worldPos);
		return;
	}
	
	MaterialData matData = readMaterialData(texture(gbufferColor1, screenCoord_v), texture(gbufferColor2, screenCoord_v));
	
	vec3 irradiance = C1 * L22 * (matData.normal.x * matData.normal.x - matData.normal.y * matData.normal.y) +
	                  C3 * L20 * matData.normal.z * matData.normal.z +
	                  C4 * L00 -
	                  C5 * L20 +
	                  2.0 * C1 * L2m2 * matData.normal.x * matData.normal.y +
	                  2.0 * C1 * L21  * matData.normal.x * matData.normal.z +
	                  2.0 * C1 * L2m1 * matData.normal.y * matData.normal.z +
	                  2.0 * C2 * L11  * matData.normal.x +
	                  2.0 * C2 * L1m1 * matData.normal.y +
	                  2.0 * C2 * L10  * matData.normal.z;
	
	color_out = vec4(irradiance * 0.1 * matData.ambientOcclusion * matData.diffuse, 0);
	
	float diffuseFactor = max(dot(matData.normal, -rs.sunDir), 0.0);
	vec3 dirToEye = normalize(rs.cameraPos - worldPos);
	vec3 hDir = normalize(dirToEye - rs.sunDir);
	float specFactor = pow(max(dot(matData.normal, hDir), 0.0), matData.specExponent);
	color_out.rgb += rs.sunColor * diffuseFactor * (matData.diffuse + matData.specIntensity * specFactor);
}
