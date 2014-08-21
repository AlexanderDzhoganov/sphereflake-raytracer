#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D SSAO;

uniform vec3 cameraPosition;
uniform vec2 framebufferSize;

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
	//gl_FragColor = vec4(vec3(ssao), 1.0);
	gl_FragColor = vec4((0.5 + 0.5 * (position + cameraPosition)) * ssao, 1.0);
}