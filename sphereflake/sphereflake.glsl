uniform sampler2D tex;

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(1280, 720);
	gl_FragColor = texture(tex, uv);
}