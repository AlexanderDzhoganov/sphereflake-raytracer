#ifndef __SPHEREFLAKERAYTRACER_UTIL_H
#define __SPHEREFLAKERAYTRACER_UTIL_H

namespace SphereflakeRaytracer
{

	inline vec3 SphericalToWorldCoodinates(float longitude, float latitude)
	{
		auto sint = sinf(longitude);
		return vec3(cosf(latitude) * sint, sinf(latitude) * sint, cosf(longitude));
	}

	inline mat4 CreateRotationMatrix(const vec3& rot)
	{
		return glm::rotate(radians(rot.x), vec3(1, 0, 0)) *
			glm::rotate(radians(rot.y), vec3(0, 1, 0)) *
			glm::rotate(radians(rot.z), vec3(0, 0, 1));
	}

}

// make_unique for Visual Studio 2012's broken variadics implementation as per Herb Sutter's blog
#if _MSC_VER <= 1700

namespace std
{

#define _MAKE_UNIQUE(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4)	\
\
template<class T COMMA LIST(_CLASS_TYPE)>	\
inline std::unique_ptr<T> make_unique(LIST(_TYPE_REFREF_ARG))	\
{	\
return std::unique_ptr<T>(new T(LIST(_FORWARD_ARG)));	\
}

_VARIADIC_EXPAND_0X(_MAKE_UNIQUE, , , , )
#undef _MAKE_UNIQUE

}

#endif

#endif
