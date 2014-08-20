#ifndef __UTIL_H
#define __UTIL_H

namespace SphereflakeRaytracer
{

	vec3 SphericalToWorldCoodinates(float longitude, float latitude)
	{
		auto sint = sinf(longitude);
		return vec3(cosf(latitude) * sint, sinf(latitude) * sint, cosf(longitude));
	}

	mat4 CreateRotationMatrix(const vec3& rot)
	{
		return glm::rotate(radians(rot.x), vec3(1, 0, 0)) *
			glm::rotate(radians(rot.y), vec3(0, 1, 0)) *
			glm::rotate(radians(rot.z), vec3(0, 0, 1));
	}

}

#endif
