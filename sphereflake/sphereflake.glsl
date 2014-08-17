#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;

vec3 lightDir = vec3(1, 1, 0);

void main()
{
	vec3 position = texelFetch(positions, ivec2(gl_FragCoord.xy), 0).xyz;
	vec3 normal = texelFetch(normals, ivec2(gl_FragCoord.xy), 0).xyz;

	float d = max(0.0, dot(normal, normalize(lightDir)));

	gl_FragColor = vec4(vec3(1.0) * d, 1.0);
}