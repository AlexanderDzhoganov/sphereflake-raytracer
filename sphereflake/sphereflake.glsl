#version 440

layout(binding=0) uniform sampler2D positions;
layout(binding=1) uniform sampler2D normals;

uniform vec3 cameraPosition;
uniform float time;

const float spec = 1.0;
const float diffuse = 0.1;
const float ambient = 0.15;
const float shininess = 32.0;
const vec3 color = vec3(1.0);

void main()
{
	vec3 position = texelFetch(positions, ivec2(gl_FragCoord.xy - vec2(0.5)), 0).xyz;
	
	if(length(position) == 0.0)
	{
		gl_FragColor = vec4(0, 0, 0, 1);
		return;
	}
	
	vec3 normal = texelFetch(normals, ivec2(gl_FragCoord.xy - vec2(0.5)), 0).xyz;

	vec3 lightPosition = vec3(cos(time * 0.25) * 64.0, 4.0, sin(time * 0.25) * 64.0);
	vec3 lightDir = normalize(position - lightPosition);

	vec3 reflected = reflect(-lightDir, normal);

	vec3 viewDir = normalize(position - cameraPosition);
	float phong = ambient + diffuse * max(0.0, dot(lightDir, normal)) + spec * pow(max(0.0, dot(reflected, viewDir)), shininess);

	gl_FragColor = vec4(phong * color, 1.0);
}