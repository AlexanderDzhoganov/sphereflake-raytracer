#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D noiseTexture;

uniform vec3 cameraPosition;
uniform vec2 framebufferSize;

const float g_sample_rad = 4.0;
const float g_intensity = 1.0;
const float g_scale = 3.0;
const float g_bias = 0.5;

float doAmbientOcclusion(vec2 uv, vec3 position, vec3 normal)
{
	vec3 samplePosition = texelFetch(positions, ivec2(gl_FragCoord.xy + uv), 0).xyz;
	vec3 diff = samplePosition - position;
	vec3 v = normalize(diff);
	float d = length(diff) * g_scale;
	return max(0.0, dot(normal, v) - g_bias) * (1.0 / (1.0 + d)) * g_intensity;
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
	float rad = g_sample_rad / abs(position.z);

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