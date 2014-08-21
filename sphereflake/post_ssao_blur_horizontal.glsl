#version 440 core

layout(binding=2) uniform sampler2D source;

uniform float offset[3] = float[] (0.0, 1.3846153846, 3.2307692308);
uniform float weight[3] = float[] (0.2270270270, 0.3162162162, 0.0702702703);

void main(void)
{
	vec4 color = vec4(1.0);
	vec2 pixelSize = 1.0 / textureSize(source, 0);
	vec2 uv = gl_FragCoord.xy * pixelSize;

	color = texture(source, uv) * weight[0];

	for (int i = 1; i < 3; i++)
	{
		color += texture(source, uv + vec2(offset[i], 0.0) * pixelSize) * weight[i];
		color += texture(source, uv - vec2(offset[i], 0.0) * pixelSize) * weight[i];
	}

	gl_FragColor = color;
}
