#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D noiseTexture;

uniform vec3 cameraPosition;

uniform mat4 viewProjection;
uniform mat4 view;
uniform mat3 invTranspView3x3;

const float noiseScale = 0.1;
const int KERNEL_SIZE = 32;
uniform vec3 rotationKernel[KERNEL_SIZE];

const float kernelRadius = 32.0;

const vec2 vec[4] = { vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1) };

const float random_size = 1.0;
const float g_sample_rad = 1.0;
const float g_intensity = 1.0;
const float g_scale = 1.0;
const float g_bias = 1.0;

vec3 getPosition(vec2 uv)
{
	return texelFetch(positions, ivec2(uv), 0).xyz;
}

vec3 getNormal(vec2 uv)
{
	return normalize(texture(normals, uv).xyz * 2.0f - 1.0f);
}

vec2 getRandom(vec2 uv)
{
	return normalize(texture(noiseTexture, gl_FragCoord.xy / random_size).xy * 2.0f - 1.0f);
}

float doAmbientOcclusion(vec2 tcoord, vec2 uv, vec3 p, vec3 cnorm)
{
	vec3 diff = getPosition(tcoord + uv) - p;
	const vec3 v = normalize(diff);
	const float d = length(diff) * g_scale;
	return max(0.0,dot(cnorm,v)-g_bias)*(1.0/(1.0+d))*g_intensity;
}

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;

	vec3 position = texelFetch(positions, ivec2(fragCoord.xy - vec2(0.5)), 0).xyz;
	if(length(position) == 0.0)
	{
		gl_FragColor = vec4(0, 0, 0, 1);
		return;
	}

	vec3 normal = texelFetch(normals, ivec2(gl_FragCoord.xy - vec2(0.5)), 0).xyz;
	vec3 noise = texture(noiseTexture, gl_FragCoord.xy / vec2(1280, 720)).xyz;
	vec2 rand = getRandom(gl_FragCoord.xy);

	float ao = 0.0;

	for (int i = 0; i < 4; i++)
	{
		vec2 coord1 = reflect(vec[i],rand) * kernelRadius;
		vec2 coord2 = vec2(coord1.x * 0.707 - coord1.y * 0.707, coord1.x * 0.707 + coord1.y * 0.707);
  
		ao += doAmbientOcclusion(gl_FragCoord.xy, coord1*0.25, position, normal);
		ao += doAmbientOcclusion(gl_FragCoord.xy, coord2*0.5, position, normal);
		ao += doAmbientOcclusion(gl_FragCoord.xy, coord1*0.75, position, normal);
		ao += doAmbientOcclusion(gl_FragCoord.xy, coord2, position, normal);
	}

	ao /= 4;
	
	gl_FragColor = vec4(vec3(ao), 1);
}