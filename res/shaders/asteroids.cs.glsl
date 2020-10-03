layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

struct AsteroidSettings {
	vec3 pos;
	float scale;
	vec3 rotationAxis;
	float initialRotation;
	float rotationSpeed;
	uint firstVertex;
	uint enabled;
};

layout(binding=0, std430) readonly AsteroidSettingsBuf {
	AsteroidSettings asteroidSettings[];
};

layout(binding=1, std430) writeonly buffer AsteroidTransformsBuf {
	mat4 transformsOut[];
};

layout(binding=2, std430) writeonly buffer AsteroidDrawArgsBuf {
	uint drawArgsOut[];
};

layout(binding=2, std430) writeonly buffer AsteroidDrawArgsBuf {
	uint drawArgsOutShadow[];
};

const uint NUM_LOD_LEVELS = 4;

uniform vec3 wrappingOffset;
uniform vec3 wrappingModulo;
uniform vec3 globalOffset;
uniform float distancePerLod;

uniform uint lodVertexOffsets[NUM_LOD_LEVELS];
uniform uint lodFirstIndex[NUM_LOD_LEVELS];
uniform uint lodNumIndices[NUM_LOD_LEVELS];
uniform vec4 frustumPlanes[6];
uniform vec4 frustumPlanesShadow[4];

#include rendersettings.glh

void main() {
	uint asteroidIdx = gl_GlobalInvocationID.x;
	vec3 pos = mod(asteroidSettings[asteroidIdx].pos + wrappingOffset, wrappingModulo) + globalOffset;
	
	uint drawArgsIdx = asteroidIdx * 5;
	
	//Frustum culling
	drawArgsOut[drawArgsIdx + 1] = asteroidSettings[asteroidIdx].enabled;
	for (int i = 0; i < 6; i++) {
		if (dot(vec4(pos, 1), frustumPlanes[i]) < -asteroidSettings[asteroidIdx].scale) {
			drawArgsOut[drawArgsIdx + 1] = 0;
		}
	}
	
	//Frustum culling for shadow mapping
	drawArgsOutShadow[drawArgsIdx + 1] = asteroidSettings[asteroidIdx].enabled;
	for (int i = 0; i < 4; i++) {
		if (dot(vec4(pos, 1), frustumPlanesShadow[i]) < -asteroidSettings[asteroidIdx].scale) {
			drawArgsOutShadow[drawArgsIdx + 1] = 0;
		}
	}
	
	//Lod
	float distToEdge = max(distance(rs.cameraPos, pos) - asteroidSettings[asteroidIdx].scale, 0);
	int lodLevel = NUM_LOD_LEVELS - 1 - clamp(int(distToEdge / distancePerLod), 0, NUM_LOD_LEVELS - 1);
	
	//Writes draw arguments
	drawArgsOutShadow[drawArgsIdx + 0] = drawArgsOut[drawArgsIdx + 0] = lodNumIndices[lodLevel];
	drawArgsOutShadow[drawArgsIdx + 2] = drawArgsOut[drawArgsIdx + 2] = lodFirstIndex[lodLevel];
	drawArgsOutShadow[drawArgsIdx + 3] = drawArgsOut[drawArgsIdx + 3] = asteroidSettings[asteroidIdx].firstVertex + lodVertexOffsets[lodLevel];
	drawArgsOutShadow[drawArgsIdx + 4] = drawArgsOut[drawArgsIdx + 4] = asteroidIdx;
	
	//Transform
	float rotation = asteroidSettings[asteroidIdx].initialRotation + asteroidSettings[asteroidIdx].rotationSpeed * rs.gameTime;
	vec3 rotationAxis = asteroidSettings[asteroidIdx].rotationAxis;
	transformsOut[asteroidIdx] = mat4();
}
