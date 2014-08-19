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

void main()
{
	if(position.x > 0.99 && position.x < 1.01)
	{
		gl_FragColor = vec4(1, 0, 0, 1);
		return;
	} else if(position.y > 0.99 && position.y < 1.01)
	{
		gl_FragColor = vec4(0, 1, 0, 1);
		return;
	} else if(position.z > 0.99 && position.z < 1.01)
	{
		gl_FragColor = vec4(0, 0, 1, 1);
		return;
	}

	gl_FragColor = vec4(1, 1, 1, 1);
}