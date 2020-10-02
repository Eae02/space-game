in vec3 color_v;
in vec4 dirAndDist_v;
in vec3 eyeVector_v;

layout(location=0) out vec4 color_out;

void main() {
	float alpha = abs(dot(normalize(dirAndDist_v.xyz), normalize(eyeVector_v)));
	float intensity = pow(alpha, dirAndDist_v.w * 2);
	color_out = vec4(color_v * intensity, 0);
}
