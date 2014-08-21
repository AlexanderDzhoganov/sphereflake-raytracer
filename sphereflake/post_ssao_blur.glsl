#version 440 core

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D SSAO;

uniform vec3 cameraPosition;
uniform vec2 framebufferSize;

void main()
{
	vec2 uv = gl_FragCoord.xy / framebufferSize;
	vec2 ps = vec2(1.0) / framebufferSize;

	vec3 color = texture(SSAO, uv).xyz;

	const vec2 vec[8] = { vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1), vec2(1, 1), vec2(-1, -1), vec2(1, -1), vec2(-1, 1) };

	const int iterations = 8;
	for(int x = 0; x < iterations; x++)
	{
		color += texture(SSAO, uv + vec[x] * ps).xyz;
	}

	color /= float(iterations + 1);

	gl_FragColor = vec4(color, 1.0);
}