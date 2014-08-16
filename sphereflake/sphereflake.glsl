float raySphereIntersect(vec3 rayOrigin, vec3 rayDirection, vec3 sphereOrigin, float sphereRadius)
{
	vec3 sphereTOrigin = sphereOrigin - rayOrigin;
    float t = dot(rayDirection, sphereTOrigin);
    vec3 closestPoint = rayDirection * t;
   	float dist = length(closestPoint - sphereTOrigin);
    return dist - sphereRadius;
}

vec3 rayOrigin;
vec3 rayDirection;

mat4 parentSphereTransformations[128];

mat4 createTranslation(vec3 translation)
{
	mat4 result = mat4(1.0);
    result[3] = vec4(translation, 1.0);
    return result;
}

mat4 createRotation(vec3 rot)
{
    float sinx = sin(rot.x);
    float siny = sin(rot.y);
    float sinz = sin(rot.z);
    
    float cosx = cos(rot.x);
    float cosy = cos(rot.y);
    float cosz = cos(rot.z);
    
	mat4 result = mat4(1.0);
    result[0] = vec4(cosy * cosz, cosx * sinz + sinx * siny * cosz, sinx * sinz - cosx * siny * cosz, 0.0);
    result[1] = vec4(-cosy * sinz, cosx * cosz - sinx * siny * sinz, sinx * cosz + cosx * siny * sinz, 0.0);
    result[2] = vec4(siny, -sinx * cosy, cosx * cosy, 0.0);
    result[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return result;
}

mat4 createScale(vec3 scale)
{
    mat4 result = mat4(1.0);
    result[0][0] = scale.x;
    result[1][1] = scale.y;
    result[2][2] = scale.z;
    return result;
}

void main(void)
{
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
    
    vec3 rayOrigin = vec3(uv.x, 0.0, uv.y);
    vec3 rayDirection = vec3(0, -1.0, 0.0);
    
    vec3 sphereOrigin = vec3(0.5, -2.0, 0.5);
    float sphereRadius = 0.25;
    
    vec4 color = vec4(0.0);
    
    const int depth = 20;
    
    for(int i = 1; i <= depth; i++)
    {
		for(float q = 0.0; q < 6.0; q+= 1.0)
        {
            mat4 parentTransform = mat4(1.0);
            if(i > 1)
            {
				parentTransform = parentSphereTransformations[(i - 1) * int(q)];
            }
            else
            {
				parentTransform = mat4(1.0);
            }
            
			mat4 transform = mat4(1.0);
            transform *= createScale(vec3(1.0 / 3.0));
            transform *= createRotation(vec3(0.0, 0.0, -1.57079633));
            transform *= createTranslation(vec3(0.0, 0.0, 0.0));
            parentSphereTransformations[int(q) * i] = parentTransform * transform;
        }
    }
    
    for(int i = 0; i <= depth; i++)
    {
		if(i == 0)
        {
            if(raySphereIntersect(rayOrigin, rayDirection, sphereOrigin, sphereRadius) < 0.0)
            {
                color = vec4(0.0, 1.0, 0.0, 1.0);
            }	
        }
        else
        {
			float radiusMultiplier = 0.25 * (1.0 / 3.0);
            
            for(float q = 0.0; q < 6.0; q += 1.0)
            {
                float cosx = cos(1.04719755 * q);
                float sinx = sin(1.04719755 * q);
                
                vec3 displacement = vec3(cosx, 0.0, sinx) * (sphereRadius + radiusMultiplier);
				vec3 origin = sphereOrigin + displacement;
                
                if(raySphereIntersect(rayOrigin, rayDirection, origin, radiusMultiplier) < 0.0)
                {
                    color = vec4(0.0, 0.0, 1.0, 1.0);
                }
            }
        }
    }
    
    gl_FragColor = color;
}