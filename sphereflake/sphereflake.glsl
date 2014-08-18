#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;

vec3 lightPosition = vec3(1, 1, 0);

void main()
{
	vec3 position = texelFetch(positions, ivec2(gl_FragCoord.xy), 0).xyz;
	vec3 normal = texelFetch(normals, ivec2(gl_FragCoord.xy), 0).xyz;

	vec3 lightDir = normalize(position - lightPosition);
	float d = max(0.0, dot(normal, lightDir));

	if(length(position) == 0.0)
	{
		gl_FragColor = vec4(1, 0, 1, 1);
		return;
	}

	gl_FragColor = vec4(position, 1.0);
//	gl_FragColor = vec4(vec3(1.0) * d, 1.0);
}