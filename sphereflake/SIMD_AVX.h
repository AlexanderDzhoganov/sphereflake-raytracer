#ifndef __SIMD_H
#define __SIMD_H

namespace SphereflakeRaytracer
{

	namespace SSE
	{

		namespace Constants
		{

			const __m256 minusOne = _mm256_set1_ps(-1.0f);
			const __m256 zero = _mm256_set1_ps(0.0f);
			const __m256 oneThird = _mm256_set1_ps(1.0f / 3.0f);
			const __m256 oneHalf = _mm256_set1_ps(1.0f / 2.0f);
			const __m256 one = _mm256_set1_ps(1.0f);
			const __m256 two = _mm256_set1_ps(2.0f);
			const __m256 three = _mm256_set1_ps(3.0f);
			const __m256 hundred = _mm256_set1_ps(100.0f);

		}

		struct Vec3Packet
		{

			union
			{
				__m256 x; struct { float x0; float x1; float x2; float x3; float x4; float x5; float x6; float x7; };
			};

			union
			{
				__m256 y; struct { float y0; float y1; float y2; float y3; float y4; float y5; float y6; float y7; };
			};

			union
			{
				__m256 z; struct { float z0; float z1; float z2; float z3; float z4; float z5; float z6; float z7; };
			};

			void Set(const vec3& v)
			{
				x = _mm256_set1_ps(v.x);
				y = _mm256_set1_ps(v.y);
				z = _mm256_set1_ps(v.z);
			}

			void Set(const vec3& a, const vec3& b, const vec3& c, const vec3& d, const vec3& e, const vec3& f, const vec3& g, const vec3& h)
			{
				x = _mm256_set_ps(a.x, b.x, c.x, d.x, e.x, f.x, g.x, h.x);
				y = _mm256_set_ps(a.y, b.y, c.y, d.y, e.y, f.y, g.y, h.y);
				z = _mm256_set_ps(a.z, b.z, c.z, d.z, e.z, f.z, g.z, h.z);
			}

			vec3 Extract(size_t index)
			{
				switch (index)
				{
				case 0: return vec3(x0, y0, z0);
				case 1: return vec3(x1, y1, z1);
				case 2: return vec3(x2, y2, z2);
				case 3: return vec3(x3, y3, z3);
				case 4: return vec3(x4, y4, z4);
				case 5: return vec3(x5, y5, z5);
				case 6: return vec3(x6, y6, z6);
				case 7: return vec3(x7, y7, z7);
				}

				return vec3(0.f);
			}

		};

		__forceinline Vec3Packet operator+(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_add_ps(a.x, b.x);
			result.y = _mm256_add_ps(a.y, b.y);
			result.z = _mm256_add_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet operator-(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_sub_ps(a.x, b.x);
			result.y = _mm256_sub_ps(a.y, b.y);
			result.z = _mm256_sub_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet operator*(const Vec3Packet& a, __m256 scalar)
		{
			Vec3Packet result;
			result.x = _mm256_mul_ps(a.x, scalar);
			result.y = _mm256_mul_ps(a.y, scalar);
			result.z = _mm256_mul_ps(a.z, scalar);
			return result;
		}

		__forceinline Vec3Packet operator/(const Vec3Packet& a, __m256 scalar)
		{
			Vec3Packet result;
			result.x = _mm256_div_ps(a.x, scalar);
			result.y = _mm256_div_ps(a.y, scalar);
			result.z = _mm256_div_ps(a.z, scalar);
			return result;
		}

		__forceinline __m256 Dot(const Vec3Packet& a, const Vec3Packet& b)
		{
			auto x2 = _mm256_mul_ps(a.x, b.x);
			auto y2 = _mm256_mul_ps(a.y, b.y);
			auto z2 = _mm256_mul_ps(a.z, b.z);
			return _mm256_add_ps(_mm256_add_ps(x2, y2), z2);
		}

		__forceinline __m256 Length2(const Vec3Packet& a)
		{
			auto temp = _mm256_mul_ps(a.x, a.x);
			auto y2 = _mm256_mul_ps(a.y, a.y);
			auto z2 = _mm256_mul_ps(a.z, a.z);

			temp = _mm256_add_ps(temp, y2);
			temp = _mm256_add_ps(temp, z2);
			return temp;
		}

		__forceinline void Normalize(Vec3Packet& a)
		{
			auto x2 = _mm256_mul_ps(a.x, a.x);
			auto y2 = _mm256_mul_ps(a.y, a.y);
			auto z2 = _mm256_mul_ps(a.z, a.z);

			x2 = _mm256_add_ps(x2, y2);
			x2 = _mm256_add_ps(x2, z2);

			__m256 nr = _mm256_rsqrt_ps(x2);
			__m256 muls = _mm256_mul_ps(_mm256_mul_ps(x2, nr), nr);
			x2 = _mm256_mul_ps(_mm256_mul_ps(Constants::oneHalf, nr), _mm256_sub_ps(Constants::three, muls));

			a.x = _mm256_mul_ps(a.x, x2);
			a.y = _mm256_mul_ps(a.y, x2);
			a.z = _mm256_mul_ps(a.z, x2);
		}

		__forceinline Vec3Packet BitwiseAnd(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_and_ps(a.x, b.x);
			result.y = _mm256_and_ps(a.y, b.y);
			result.z = _mm256_and_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet BitwiseAnd(__m256 a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_and_ps(a, b.x);
			result.y = _mm256_and_ps(a, b.y);
			result.z = _mm256_and_ps(a, b.z);
			return result;
		}

		__forceinline Vec3Packet BitwiseAndNot(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_andnot_ps(a.x, b.x);
			result.y = _mm256_andnot_ps(a.y, b.y);
			result.z = _mm256_andnot_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet BitwiseAndNot(__m256 a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_andnot_ps(a, b.x);
			result.y = _mm256_andnot_ps(a, b.y);
			result.z = _mm256_andnot_ps(a, b.z);
			return result;
		}

		__forceinline Vec3Packet BitwiseOr(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_or_ps(a.x, b.x);
			result.y = _mm256_or_ps(a.y, b.y);
			result.z = _mm256_or_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet BitwiseOr(__m256 a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_or_ps(a, b.x);
			result.y = _mm256_or_ps(a, b.y);
			result.z = _mm256_or_ps(a, b.z);
			return result;
		}

		__forceinline __m256 RayPlaneIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& planeNormal, __m256& t)
		{
			t = _mm256_div_ps(_mm256_mul_ps(Dot(rayOrigin, planeNormal), Constants::minusOne), Dot(rayDirection, planeNormal));
			auto result = _mm256_cmp_ps(t, Constants::zero, _CMP_GE_OQ);
			return result;
		}

		__forceinline __m256 RaySphereIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& sphereOrigin, __m256 sphereRadiusSq, __m256& t)
		{
			Vec3Packet L = sphereOrigin - rayOrigin;
			__m256 tca = Dot(L, rayDirection);
			auto result = _mm256_cmp_ps(tca, Constants::zero, _CMP_GE_OQ);

			if (_mm256_movemask_ps(result) == 0)
			{
				return result;
			}

			auto d2 = _mm256_sub_ps(Dot(L, L), _mm256_mul_ps(tca, tca));

			result = _mm256_cmp_ps(d2, sphereRadiusSq, _CMP_LE_OQ);
			if (_mm256_movemask_ps(result) == 0)
			{
				return result;
			}

			__m256 thc = _mm256_sqrt_ps(_mm256_sub_ps(sphereRadiusSq, d2));

			__m256 t0 = _mm256_add_ps(tca, thc);
			__m256 t1 = _mm256_sub_ps(tca, thc);

			auto tresult = _mm256_cmp_ps(t0, t1, _CMP_LE_OQ);
			t = _mm256_or_ps(_mm256_and_ps(tresult, t0), _mm256_andnot_ps(tresult, t1));

			return result;
		}

		__forceinline __m256 RayBoundingSphereIntersection(const Vec3Packet& rayOrigin, const Vec3Packet& rayDirection, const Vec3Packet& sphereOrigin, __m256 sphereRadiusSq)
		{
			Vec3Packet L = sphereOrigin - rayOrigin;
			__m256 tca = Dot(L, rayDirection);

			auto result = _mm256_cmp_ps(tca, Constants::zero, _CMP_GE_OQ);
			if (_mm256_movemask_ps(result) == 0)
			{
				return result;
			}

			auto d2 = _mm256_sub_ps(Dot(L, L), _mm256_mul_ps(tca, tca));
			return _mm256_cmp_ps(d2, sphereRadiusSq, _CMP_LE_OQ);
		}

	}

}

#endif