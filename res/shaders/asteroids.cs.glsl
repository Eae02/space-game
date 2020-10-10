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

const int NUM_LOD_LEVELS = 5;

#include rendersettings.glh

//per-frame uniforms
uniform vec3 wrappingOffset;
uniform vec3 globalOffset;
uniform vec4 frustumPlanes[6];
uniform vec4 frustumPlanesShadow[4 * NUM_SHADOW_CASCADES];

//constant uniforms
uniform uint numAsteroids;
uniform float wrappingModulo;
uniform float distancePerLod;
uniform uint lodVertexOffsets[NUM_LOD_LEVELS];
uniform uint lodFirstIndex[NUM_LOD_LEVELS];
uniform uint lodNumIndices[NUM_LOD_LEVELS];

const float SCALE_FADE_BEGIN = 0.9;

int getLodLevel(float dist, float bias) {
	return NUM_LOD_LEVELS - 1 - clamp(int(dist / distancePerLod - bias), 0, NUM_LOD_LEVELS - 1);
}

void main() {
	uint asteroidIdx = gl_GlobalInvocationID.x;
	if (asteroidIdx > numAsteroids)
		return;
	
	vec3 posNoGlobalOffset = mod(asteroidSettings[asteroidIdx].pos + wrappingOffset, vec3(wrappingModulo));
	vec3 pos = posNoGlobalOffset + globalOffset;
	
	vec3 distToWrapEdge3 = abs(posNoGlobalOffset - vec3(wrappingModulo / 2));
	float distToWrapEdge = max(max(distToWrapEdge3.x, distToWrapEdge3.y), distToWrapEdge3.z) / (wrappingModulo / 2);
	float scale = 1 - clamp((distToWrapEdge - SCALE_FADE_BEGIN) / (1 - SCALE_FADE_BEGIN), 0, 1);
	
	uint drawArgsIdx = asteroidIdx * 5;
	uint drawArgsStride = numAsteroids * 5;
	
	//Lod
	float distToEdge = max(distance(rs.cameraPos, pos) - asteroidSettings[asteroidIdx].radius, 0);
	int lodLevel = getLodLevel(distToEdge, 1);
	
	//Frustum culling
	drawArgsOut[drawArgsIdx + 1] = 1;
	for (int i = 0; i < 6; i++) {
		if (dot(vec4(pos, 1), frustumPlanes[i]) < -asteroidSettings[asteroidIdx].radius) {
			drawArgsOut[drawArgsIdx + 1] = 0;
		}
	}
	
	//Frustum culling for shadow mapping
	for (uint cascade = 0; cascade < NUM_SHADOW_CASCADES; cascade++) {
		uint enabledCmdIndex = drawArgsStride * (cascade + 1) + drawArgsIdx + 1;
		drawArgsOut[enabledCmdIndex] = 1;
		for (int i = 0; i < 4; i++) {
			if (dot(vec4(pos, 1), frustumPlanesShadow[cascade * 4 + i]) < -asteroidSettings[asteroidIdx].radius) {
				drawArgsOut[enabledCmdIndex] = 0;
			}
		}
	}
	
	//Writes draw arguments
	uint daNumIndices = lodNumIndices[lodLevel];
	uint daFirstIndex = lodFirstIndex[lodLevel];
	uint dafirstVertex = asteroidSettings[asteroidIdx].firstVertex + lodVertexOffsets[lodLevel];
	drawArgsOut[drawArgsIdx + 0] = daNumIndices;
	drawArgsOut[drawArgsIdx + 2] = daFirstIndex;
	drawArgsOut[drawArgsIdx + 3] = dafirstVertex;
	drawArgsOut[drawArgsIdx + 4] = 0;
	
	uint lodLevelShadow = getLodLevel(distToEdge, -1);
	daNumIndices = lodNumIndices[lodLevelShadow];
	daFirstIndex = lodFirstIndex[lodLevelShadow];
	dafirstVertex = asteroidSettings[asteroidIdx].firstVertex + lodVertexOffsets[lodLevelShadow];
	for (uint i = 1; i <= NUM_SHADOW_CASCADES; i++) {
		drawArgsOut[drawArgsIdx + i * drawArgsStride + 0] = daNumIndices;
		drawArgsOut[drawArgsIdx + i * drawArgsStride + 2] = daFirstIndex;
		drawArgsOut[drawArgsIdx + i * drawArgsStride + 3] = dafirstVertex;
		drawArgsOut[drawArgsIdx + i * drawArgsStride + 4] = 0;
	}
	
	//Transform
	float rotation = asteroidSettings[asteroidIdx].initialRotation + asteroidSettings[asteroidIdx].rotationSpeed * rs.gameTime;
	float sinr = sin(rotation);
	float cosr = cos(rotation);
	vec3 raxis = normalize(unpackSnorm4x8(asteroidSettings[asteroidIdx].rotationAxis).xyz);
	vec3 rx = vec3(cosr + raxis.x * raxis.x * (1 - cosr), raxis.x * raxis.y * (1 - cosr) - raxis.z * sinr, raxis.x * raxis.z * (1 - cosr) + raxis.y * sinr);
	vec3 ry = vec3(raxis.y * raxis.x * (1 - cosr) + raxis.z * sinr, cosr + raxis.y * raxis.y * (1 - cosr), raxis.y * raxis.z * (1 - cosr) - raxis.x * sinr);
	vec3 rz = vec3(raxis.z * raxis.x * (1 - cosr) - raxis.y * sinr, raxis.z * raxis.y * (1 - cosr) + raxis.x * sinr, cosr + raxis.z * raxis.z * (1 - cosr));
	transformsOut[asteroidIdx] = mat4(
		vec4(rx * scale, 0),
		vec4(ry * scale, 0),
		vec4(rz * scale, 0),
		vec4(pos, 1)
	);
}
