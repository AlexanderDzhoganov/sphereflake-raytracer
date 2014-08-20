#ifndef __SIMD_H
#define __SIMD_H

namespace SphereflakeRaytracer
{

	namespace SSEConstants
	{

		const __m128 minusOne = _mm_set1_ps(-1.0f);
		const __m128 zero = _mm_set1_ps(0.0f);
		const __m128 oneThird = _mm_set1_ps(1.0f / 3.0f);
		const __m128 oneHalf = _mm_set1_ps(1.0f / 2.0f);
		const __m128 one = _mm_set1_ps(1.0f);
		const __m128 two = _mm_set1_ps(2.0f);
		const __m128 three = _mm_set1_ps(3.0f);
		const __m128 hundred = _mm_set1_ps(100.0f);

	}

	struct Vec3Packet
	{

		union
		{
			__m128 x; struct { float x0; float x1; float x2; float x3; };
		};

		union
		{
			__m128 y; struct { float y0; float y1; float y2; float y3; };
		};

		union
		{
			__m128 z; struct { float z0; float z1; float z2; float z3; };
		};

		void Set(const glm::vec3& v)
		{
			x = _mm_set_ps(v.x, v.x, v.x, v.x);
			y = _mm_set_ps(v.y, v.y, v.y, v.y);
			z = _mm_set_ps(v.z, v.z, v.z, v.z);
		}

		void Set(const vec3& a, const vec3& b, const vec3& c, const vec3& d)
		{
			x = _mm_set_ps(a.x, b.x, c.x, d.x);
			y = _mm_set_ps(a.y, b.y, c.y, d.y);
			z = _mm_set_ps(a.z, b.z, c.z, d.z);
		}

		vec3 Extract(size_t index)
		{
			switch (index)
			{
			case 0: return vec3(x0, y0, z0);
			case 1: return vec3(x1, y1, z1);
			case 2: return vec3(x2, y2, z2);
			case 3: return vec3(x3, y3, z3);
			}

			return vec3(0.f);
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
		auto temp = _mm_mul_ps(a.x, a.x);
		auto y2 = _mm_mul_ps(a.y, a.y);
		auto z2 = _mm_mul_ps(a.z, a.z);

		temp = _mm_add_ps(temp, y2);
		temp = _mm_add_ps(temp, z2);
		return temp;
	}

	__forceinline void Normalize(Vec3Packet& a)
	{
		auto x2 = _mm_mul_ps(a.x, a.x);
		auto y2 = _mm_mul_ps(a.y, a.y);
		auto z2 = _mm_mul_ps(a.z, a.z);

		x2 = _mm_add_ps(x2, y2);
		x2 = _mm_add_ps(x2, z2);

		__m128 nr = _mm_rsqrt_ps(x2);
		__m128 muls = _mm_mul_ps(_mm_mul_ps(x2, nr), nr);
		x2 = _mm_mul_ps(_mm_mul_ps(SSEConstants::oneHalf, nr), _mm_sub_ps(SSEConstants::three, muls));

		a.x = _mm_mul_ps(a.x, x2);
		a.y = _mm_mul_ps(a.y, x2);
		a.z = _mm_mul_ps(a.z, x2);
	}

	__forceinline __m128 RayPlaneIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& planeNormal, __m128& t)
	{
		t = _mm_div_ps(_mm_mul_ps(Dot(rayOrigin, planeNormal), SSEConstants::minusOne), Dot(rayDirection, planeNormal));
		auto result = _mm_cmpge_ps(t, SSEConstants::zero);
		return result;
	}

	__forceinline __m128 RaySphereIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& sphereOrigin, __m128 sphereRadiusSq, __m128& t)
	{
		Vec3Packet L = sphereOrigin - rayOrigin;
		__m128 tca = Dot(L, rayDirection);
		auto result = _mm_cmpge_ps(tca, SSEConstants::zero);

		if (_mm_movemask_ps(result) == 0)
		{
			return result;
		}

		auto d2 = _mm_sub_ps(Dot(L, L), _mm_mul_ps(tca, tca));

		result = _mm_cmple_ps(d2, sphereRadiusSq);
		if (_mm_movemask_ps(result) == 0)
		{
			return result;
		}

		__m128 thc = _mm_sqrt_ps(_mm_sub_ps(sphereRadiusSq, d2));

		__m128 t0 = _mm_add_ps(tca, thc);
		__m128 t1 = _mm_sub_ps(tca, thc);

		auto tresult = _mm_cmple_ps(t0, t1);
		t = _mm_or_ps(_mm_and_ps(tresult, t0), _mm_andnot_ps(tresult, t1));

		return result;
	}

	__forceinline __m128 RayBoundingSphereIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& sphereOrigin, __m128 sphereRadiusSq)
	{
		Vec3Packet L = sphereOrigin - rayOrigin;
		__m128 tca = Dot(L, rayDirection);

		auto result = _mm_cmpge_ps(tca, SSEConstants::zero);
		if (_mm_movemask_ps(result) == 0)
		{
			return result;
		}

		auto d2 = _mm_sub_ps(Dot(L, L), _mm_mul_ps(tca, tca));
		return _mm_cmple_ps(d2, sphereRadiusSq);
	}

}

#endif
