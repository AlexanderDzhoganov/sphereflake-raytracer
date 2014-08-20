#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D noiseTexture;

uniform vec3 cameraPosition;

uniform mat4 viewProjection;
uniform mat4 view;
uniform mat3 invTranspView3x3;

const float noiseScale = 0.1;
const float kernelRadius = 32.0;

const int KERNEL_SIZE = 32;
uniform vec3 rotationKernel[KERNEL_SIZE];

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;

	vec3 position = texelFetch(positions, ivec2(fragCoord.xy - vec2(0.5)), 0).xyz;
	if(length(position) == 0.0)
	{
		gl_FragColor = vec4(0, 0, 0, 1);
		return;
	}

	gl_FragColor = vec4(position, 1.0);
	return;

	vec3 normal = texelFetch(normals, ivec2(gl_FragCoord.xy - vec2(0.5)), 0).xyz;
	vec3 noise = texture(noiseTexture, gl_FragCoord.xy / vec2(1280, 720)).xyz;
	
}