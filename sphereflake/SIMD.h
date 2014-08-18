#ifndef __SIMD_H
#define __SIMD_H

struct Vec3Packet
{

	union
	{
		__m128 x; float x0; float x1; float x2; float x3;
	};
	
	union
	{
		__m128 y; float y0; float y1; float y2; float y3;
	};

	union
	{
		__m128 z; float z0; float z1; float z2; float z3;
	};

	void Set(const glm::vec3& v)
	{
		x = _mm_set_ps(v.x, v.x, v.x, v.x);
		y = _mm_set_ps(v.y, v.y, v.y, v.y);
		z = _mm_set_ps(v.z, v.z, v.z, v.z);
	}

	void Set(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
	{
		x = _mm_set_ps(a.x, b.x, c.x, d.x);
		y = _mm_set_ps(a.y, b.y, c.y, d.y);
		z = _mm_set_ps(a.z, b.z, c.z, d.z);
	}

};

__forceinline Vec3Packet operator+(const Vec3Packet& a, const Vec3Packet& b)
{
	Vec3Packet result;
	result.x = _mm_add_ps(a.x, b.x);
	result.y = _mm_add_ps(a.y, b.y);
	result.z = _mm_add_ps(a.z, b.z);
	return result;
}

__forceinline Vec3Packet operator-(const Vec3Packet& a, const Vec3Packet& b)
{
	Vec3Packet result;
	result.x = _mm_sub_ps(a.x, b.x);
	result.y = _mm_sub_ps(a.y, b.y);
	result.z = _mm_sub_ps(a.z, b.z);
	return result;
}

__forceinline Vec3Packet operator*(const Vec3Packet& a, __m128 scalar)
{
	Vec3Packet result;
	result.x = _mm_mul_ps(a.x, scalar);
	result.y = _mm_mul_ps(a.y, scalar);
	result.z = _mm_mul_ps(a.z, scalar);
	return result;
}

__forceinline Vec3Packet operator/(const Vec3Packet& a, __m128 scalar)
{
	Vec3Packet result;
	result.x = _mm_div_ps(a.x, scalar);
	result.y = _mm_div_ps(a.y, scalar);
	result.z = _mm_div_ps(a.z, scalar);
	return result;
}

__forceinline __m128 Dot(const Vec3Packet& a, const Vec3Packet& b)
{
	auto x2 = _mm_mul_ps(a.x, b.x);
	auto y2 = _mm_mul_ps(a.y, b.y);
	auto z2 = _mm_mul_ps(a.z, b.z);
	return _mm_add_ps(_mm_add_ps(x2, y2), z2);
}

__forceinline __m128 Length2(const Vec3Packet& a)
{
	auto x2 = _mm_mul_ps(a.x, a.x);
	auto y2 = _mm_mul_ps(a.y, a.y);
	auto z2 = _mm_mul_ps(a.z, a.z);
	return _mm_add_ps(_mm_add_ps(x2, y2), z2);
}

__forceinline int RaySphereIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& sphereOrigin, __m128 sphereRadius, __m128& t)
{
	// Eigen::Vector4f sphereTOrigin = sphereOrigin - rayOrigin;
	Vec3Packet sphereTOrigin = sphereOrigin - rayOrigin;
	
	//t = rayDirection.dot(sphereTOrigin);
	t = Dot(rayDirection, sphereTOrigin);

	// Eigen::Vector4f closestPoint = rayDirection * t;
	Vec3Packet closestPoint = rayDirection * t;

	// Eigen::Vector4f diff = closestPoint - sphereTOrigin;
	Vec3Packet diff = closestPoint - sphereTOrigin;

	//float dist = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
	__m128 dist = Length2(diff);

	// return dist <= sphereRadius * sphereRadius
	__m128 sphereRadiusSq = _mm_mul_ps(sphereRadius, sphereRadius);
	__m128 result = _mm_cmple_ps(dist, sphereRadiusSq);
	return _mm_movemask_ps(result);
}

#endif
