#version 420 core

// depth and normal- aware 1D gaussian blur

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;
layout(binding=2) uniform sampler2D source;

uniform float offset[3] = float[] (0.0, 1.3846153846, 3.2307692308);
uniform float weight[3] = float[] (0.2270270270, 0.3162162162, 0.0702702703);

uniform vec2 framebufferSize;

uniform float normalThreshold;
uniform float depthThreshold;

uniform vec2 blurDirection;

out vec4 outColor;

void main(void)
{
	vec4 color = vec4(0.0);
	vec2 pixelSize = 1.0 / framebufferSize;
	vec2 pixelSizeGBuffer = 1.0 / textureSize(positions, 0);

	vec2 uv = gl_FragCoord.xy * pixelSize;
	vec2 uvGBuffer = gl_FragCoord.xy * pixelSizeGBuffer;

	float leftOverWeight = 0.0;

	vec3 position = texture(positions, uv).xyz;
	vec3 normal = texture(normals, uv).xyz;

	for (int i = 1; i < 3; i++)
	{
		vec2 sampleOffset = blurDirection * vec2(offset[i]) * pixelSize;
		vec2 sampleOffsetGBuffer = blurDirection * vec2(offset[i]) * pixelSizeGBuffer;

		vec3 sampleAPosition = texture(positions, uvGBuffer + sampleOffsetGBuffer).xyz;
		vec3 sampleANormal = texture(normals, uvGBuffer + sampleOffsetGBuffer).xyz;
		
		vec3 sampleBPosition = texture(positions, uvGBuffer - sampleOffsetGBuffer).xyz;
		vec3 sampleBNormal = texture(normals, uvGBuffer - sampleOffsetGBuffer).xyz;

		if(dot(normal, sampleANormal) >= normalThreshold && abs(sampleAPosition.z - position.z) >= depthThreshold)	 
		{
			color += texture(source, uv + sampleOffset) * weight[i];
		}
		else
		{
			leftOverWeight += weight[i];
		}

		if(dot(normal, sampleBNormal) >= normalThreshold && abs(sampleBPosition.z - position.z) >= depthThreshold)
		{
			color += texture(source, uv - sampleOffset) * weight[i];	
		}
		else
		{
			leftOverWeight += weight[i];
		}
	}

	color += texture(source, uv) * (weight[0] + leftOverWeight);
	outColor = color;
}
