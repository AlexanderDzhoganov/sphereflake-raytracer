#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D noiseTexture;

uniform vec3 cameraPosition;
uniform vec2 framebufferSize;

const float SSAOSampleRadius = 4.0;
const float SSAOIntensity = 1.0;
const float SSAOScale = 3.0;
const float SSAOBias = 0.5;

float doAmbientOcclusion(vec2 uv, vec3 position, vec3 normal)
{
	vec3 samplePosition = texture(positions, (gl_FragCoord.xy + uv) / framebufferSize.xy).xyz;
	vec3 diff = samplePosition - position;
	vec3 v = normalize(diff);
	float d = length(diff) * SSAOScale;
	return max(0.0, dot(normal, v) - SSAOBias) * (1.0 / (1.0 + d)) * SSAOIntensity;
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
	
	const vec2 vec[4] = { vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1) };

	float ao = 0.0f;
	float rad = SSAOSampleRadius / abs(position.z);

	vec2 rand = normalize(texture(noiseTexture, uv).xy * 2.0f - 1.0f);

	int iterations = 16;
	for (int j = 0; j < iterations; ++j)
	{
		vec2 coord1 = reflect(vec[j], rand) * rad;
		vec2 coord2 = vec2(coord1.x * 0.707 - coord1.y * 0.707, coord1.x * 0.707 + coord1.y * 0.707);

		ao += doAmbientOcclusion(coord1 * 0.25, position, normal);
		ao += doAmbientOcclusion(coord2 * 0.5, position, normal);
		ao += doAmbientOcclusion(coord1 * 0.75, position, normal);
		ao += doAmbientOcclusion(coord2, position, normal);
	}

	ao /= float(iterations) * 4.0;
	ao = 1.0 - ao;

	gl_FragColor = vec4(vec3(ao), 1.0);
}