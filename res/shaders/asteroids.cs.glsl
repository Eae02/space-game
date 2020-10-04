layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

struct AsteroidSettings {
	vec3 pos;
	float radius;
	float initialRotation;
	float rotationSpeed;
	uint rotationAxis;
	uint firstVertex;
};

layout(binding=0, std430) readonly buffer AsteroidSettingsBuf {
	AsteroidSettings asteroidSettings[];
};

layout(binding=1, std430) writeonly buffer AsteroidTransformsBuf {
	mat4 transformsOut[];
};

layout(binding=2, std430) writeonly buffer AsteroidDrawArgsBuf {
	uint drawArgsOut[];
};

layout(binding=3, std430) writeonly buffer AsteroidDrawArgsShadowBuf {
	uint drawArgsOutShadow[];
};

const int NUM_LOD_LEVELS = 5;

//per-frame uniforms
uniform vec3 wrappingOffset;
uniform vec3 globalOffset;
uniform vec4 frustumPlanes[6];
uniform vec4 frustumPlanesShadow[4];

//constant uniforms
uniform uint numAsteroids;
uniform float wrappingModulo;
uniform float distancePerLod;
uniform uint lodVertexOffsets[NUM_LOD_LEVELS];
uniform uint lodFirstIndex[NUM_LOD_LEVELS];
uniform uint lodNumIndices[NUM_LOD_LEVELS];

const float FADE_BEGIN_DIST = 4000;
const float FADE_END_DIST = FADE_BEGIN_DIST * 1.05;

#include rendersettings.glh

void main() {
	uint asteroidIdx = gl_GlobalInvocationID.x;
	if (asteroidIdx > numAsteroids)
		return;
	vec3 pos = mod(asteroidSettings[asteroidIdx].pos + wrappingOffset, vec3(wrappingModulo)) + globalOffset;
	
	uint drawArgsIdx = asteroidIdx * 5;
	
	//Frustum culling
	drawArgsOut[drawArgsIdx + 1] = 1;
	for (int i = 0; i < 6; i++) {
		if (dot(vec4(pos, 1), frustumPlanes[i]) < -asteroidSettings[asteroidIdx].radius) {
			drawArgsOut[drawArgsIdx + 1] = 0;
		}
	}
	
	//Frustum culling for shadow mapping
	drawArgsOutShadow[drawArgsIdx + 1] = 1;
	for (int i = 0; i < 4; i++) {
		if (dot(vec4(pos, 1), frustumPlanesShadow[i]) < -asteroidSettings[asteroidIdx].radius) {
			drawArgsOutShadow[drawArgsIdx + 1] = 0;
		}
	}
	
	//Lod
	float distToEdge = max(distance(rs.cameraPos, pos) - asteroidSettings[asteroidIdx].radius, 0);
	int lodLevel = NUM_LOD_LEVELS - 1 - clamp(int(distToEdge / distancePerLod), 0, NUM_LOD_LEVELS - 1);
	
	if (distToEdge > FADE_END_DIST) {
		drawArgsOut[drawArgsIdx + 1] = 0;
		drawArgsOutShadow[drawArgsIdx + 1] = 0;
	}
	
	//Writes draw arguments
	uint daNumIndices = lodNumIndices[lodLevel];
	uint daFirstIndex = lodFirstIndex[lodLevel];
	uint dafirstVertex = asteroidSettings[asteroidIdx].firstVertex + lodVertexOffsets[lodLevel];
	
	drawArgsOut[drawArgsIdx + 0] = daNumIndices;
	drawArgsOut[drawArgsIdx + 2] = daFirstIndex;
	drawArgsOut[drawArgsIdx + 3] = dafirstVertex;
	drawArgsOut[drawArgsIdx + 4] = 0;
	drawArgsOutShadow[drawArgsIdx + 0] = daNumIndices;
	drawArgsOutShadow[drawArgsIdx + 2] = daFirstIndex;
	drawArgsOutShadow[drawArgsIdx + 3] = dafirstVertex;
	drawArgsOutShadow[drawArgsIdx + 4] = 0;
	
	//Transform
	float rotation = asteroidSettings[asteroidIdx].initialRotation + asteroidSettings[asteroidIdx].rotationSpeed * rs.gameTime;
	float sinr = sin(rotation);
	float cosr = cos(rotation);
	vec3 raxis = normalize(unpackSnorm4x8(asteroidSettings[asteroidIdx].rotationAxis).xyz);
	float scale = (1-clamp((distToEdge - FADE_BEGIN_DIST) / (FADE_END_DIST - FADE_BEGIN_DIST), 0, 1));
	transformsOut[asteroidIdx] = mat4(
		vec4(scale, 0, 0, 0),
		vec4(0, scale, 0, 0),
		vec4(0, 0, scale, 0),
		vec4(pos, 1)
	) * transpose(mat4(
		vec4(cosr + raxis.x * raxis.x * (1 - cosr), raxis.x * raxis.y * (1 - cosr) - raxis.z * sinr, raxis.x * raxis.z * (1 - cosr) + raxis.y * sinr, 0),
		vec4(raxis.y * raxis.x * (1 - cosr) + raxis.z * sinr, cosr + raxis.y * raxis.y * (1 - cosr), raxis.y * raxis.z * (1 - cosr) - raxis.x * sinr, 0),
		vec4(raxis.z * raxis.x * (1 - cosr) - raxis.y * sinr, raxis.z * raxis.y * (1 - cosr) + raxis.x * sinr, cosr + raxis.z * raxis.z * (1 - cosr), 0),
		vec4(0, 0, 0, 1)
	));
}
