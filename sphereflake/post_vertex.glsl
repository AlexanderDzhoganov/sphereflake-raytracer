#version 330 core

// attribute-less fullscreen quad vertex shader

const vec2 quad[4] = vec2[]
(
  vec2(-1.0,  1.0), vec2(-1.0, -1.0),
  vec2( 1.0,  1.0), vec2( 1.0, -1.0)
);
 
void main()
{
	gl_Position = vec4(quad[gl_VertexID], 0.0, 1.0);
}