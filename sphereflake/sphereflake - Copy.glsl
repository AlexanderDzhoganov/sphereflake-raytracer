uniform vec3 cameraPosition;
uniform vec3 topLeft;
uniform vec3 topRight;
uniform vec3 bottomLeft;

float raySphereFindT(vec3 rayOrigin, vec3 rayDirection, vec3 sphereOrigin, float sphereRadius)
{
	vec3 sphereTOrigin = sphereOrigin - rayOrigin;
	return dot(rayDirection, sphereTOrigin);
}

float raySphereIntersect(vec3 rayOrigin, vec3 rayDirection, vec3 sphereOrigin, float sphereRadius, float t)
{
	vec3 sphereTOrigin = sphereOrigin - rayOrigin;
	float t = dot(rayDirection, sphereTOrigin);
    vec3 closestPoint = rayDirection * t;
   	float dist = length(closestPoint - sphereTOrigin);
    return dist - sphereRadius;
}

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

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(1280, 720);
    vec3 rayOrigin = cameraPosition;
	vec3 targetDirection = topLeft + (topRight - topLeft) * uv.x + (bottomLeft - topLeft) * uv.y;
    vec3 rayDirection = normalize(targetDirection - rayOrigin);
    

    
    gl_FragColor = vec4(1, 0, 1, 1);
}