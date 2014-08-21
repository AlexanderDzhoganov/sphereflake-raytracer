#version 440 core

layout(binding=2) uniform sampler2D source;

void main()
{
	vec2 uv = gl_FragCoord.xy / textureSize(source, 0);
	gl_FragColor = texture(source, uv);
}