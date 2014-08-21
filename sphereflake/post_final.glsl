#version 440 core

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D SSAO;

uniform vec3 cameraPosition;
uniform vec2 framebufferSize;
uniform float ssaoFactor;

void main()
{
	vec2 uv = gl_FragCoord.xy / framebufferSize;

	vec3 position = texture(positions, uv).xyz;
	if(length(position) == 0.0)
	{
		gl_FragColor = vec4(0, 0, 0, 1);
		return;
	}

	vec3 ssao = texture(SSAO, uv).xyz;
	vec3 color = (0.5 + 0.5 * (position + cameraPosition));
	
	gl_FragColor = vec4(color * ssao, 1.0);
}