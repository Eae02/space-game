const vec2 positions[] = vec2[] (
	vec2(-1,  3),
	vec2(-1, -1),
	vec2( 3, -1)
);

out vec2 screenCoord_v;

void main() {
	gl_Position = vec4(positions[gl_VertexID], 0, 1);
	screenCoord_v = gl_Position.xy * 0.5 + 0.5;
}
