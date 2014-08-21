#version 440 core

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D noiseTexture;

uniform vec3 cameraPosition;
uniform vec2 framebufferSize;

uniform float SSAOSampleRadius;
uniform float SSAOIntensity;
uniform float SSAOScale;
uniform float SSAOBias;

const vec2 kernel[4] = { vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1) };

float occlude(vec2 uv, vec3 position, vec3 normal)
{
	vec3 samplePosition = texture(positions, (gl_FragCoord.xy + uv) / framebufferSize.xy).xyz;
	vec3 diff = samplePosition - position;
	float dist = length(diff);
	return max(0.0, dot(normal, diff / dist) - SSAOBias) * (1.0 / (1.0 + dist * dist * SSAOScale)) * SSAOIntensity;
}

void main()
{
	vec2 uv = gl_FragCoord.xy / framebufferSize;

	vec3 position = texture(positions, uv).xyz;

	if(length(position) == 0.0)
	{
		gl_FragColor = vec4(0, 0, 0, 1);
		return;
	}

	vec3 normal = texture(normals, uv).xyz;

	float ao = 0.0f;
	float rad = SSAOSampleRadius / abs(position.z);

	vec2 rand = normalize(texture(noiseTexture, uv * 0.1).xy * 2.0f - 1.0f);

	int iterations = 4;
	for (int i = 0; i < iterations; i++)
	{
		vec2 coord1 = reflect(kernel[i], rand) * rad;
		vec2 coord2 = vec2(coord1.x * 0.707 - coord1.y * 0.707, coord1.x * 0.707 + coord1.y * 0.707);

		ao += occlude(coord1 * 0.25, position, normal);
		ao += occlude(coord1 * 0.75, position, normal);
		ao += occlude(coord2 * 0.5, position, normal);
		ao += occlude(coord2, position, normal);
	}

	ao /= float(iterations) * 4.0;
	ao = 1.0 - ao;

	gl_FragColor = vec4(vec3(ao), 1.0);
}