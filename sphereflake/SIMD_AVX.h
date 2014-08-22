#ifndef __SIMD_H
#define __SIMD_H

#define ALIGN32 __declspec(align(32))

#pragma warning(disable : 4201) // disable warnings for nameless unions

namespace SphereflakeRaytracer
{

	namespace SIMD
	{
		
		using VecType = __m256;

		namespace Constants
		{

			ALIGN32 const __m256 minusOne = _mm256_set1_ps(-1.0f);
			ALIGN32 const __m256 zero = _mm256_set1_ps(0.0f);
			ALIGN32 const __m256 oneThird = _mm256_set1_ps(1.0f / 3.0f);
			ALIGN32 const __m256 oneHalf = _mm256_set1_ps(1.0f / 2.0f);
			ALIGN32 const __m256 one = _mm256_set1_ps(1.0f);
			ALIGN32 const __m256 two = _mm256_set1_ps(2.0f);
			ALIGN32 const __m256 three = _mm256_set1_ps(3.0f);
			ALIGN32 const __m256 seventy = _mm256_set1_ps(70.f);

		}

		struct Matrix4
		{

			ALIGN32 union
			{
				float m[4][4];
				__m128 rows[4];
			};

			void Set(const vec4& row0, const vec4& row1, const vec4& row2, const vec4& row3)
			{
				rows[0] = _mm_set_ps(row0.w, row0.z, row0.y, row0.x);
				rows[1] = _mm_set_ps(row1.w, row1.z, row1.y, row1.x);
				rows[2] = _mm_set_ps(row2.w, row2.z, row2.y, row2.x);
				rows[3] = _mm_set_ps(row3.w, row3.z, row3.y, row3.x);
			}

			void Set(const mat4& m)
			{
				Set(m[0], m[1], m[2], m[3]);
			}

			vec4 Extract(size_t row) const
			{
				return vec4(m[row][0], m[row][1], m[row][2], m[row][3]);
			}

		};

		// 4x4 matrix multiplication code adapted from http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
		__forceinline Matrix4 operator*(const Matrix4& a, const Matrix4& b)
		{
			Matrix4 result;
			__m128 a_line, b_line, r_line;

			for (int i = 0; i<16; i += 4)
			{
				a_line = _mm_load_ps((float*)&(a.m));
				b_line = _mm_set1_ps(((float*)&(b.m))[i]);
				r_line = _mm_mul_ps(a_line, b_line);

				for (int j = 1; j<4; j++)
				{
					a_line = _mm_load_ps(&(((float*)a.m)[j * 4]));
					b_line = _mm_set1_ps(((float*)&b)[i + j]);
					r_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), r_line);
				}
				_mm_store_ps(&(((float*)&result.m)[i]), r_line);
			}

			return result;
		}

		struct Vec3Packet
		{

			ALIGN32 union
			{
				__m256 x; struct { float x0; float x1; float x2; float x3; float x4; float x5; float x6; float x7; };
			};

			ALIGN32 union
			{
				__m256 y; struct { float y0; float y1; float y2; float y3; float y4; float y5; float y6; float y7; };
			};

			ALIGN32 union
			{
				__m256 z; struct { float z0; float z1; float z2; float z3; float z4; float z5; float z6; float z7; };
			};

			void Set(const vec3& v)
			{
				x = _mm256_broadcast_ss(&(v.x));
				y = _mm256_broadcast_ss(&(v.y));
				z = _mm256_broadcast_ss(&(v.z));
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

		__forceinline Vec3Packet operator*(const Vec3Packet& a, const __m256& scalar)
		{
			Vec3Packet result;
			result.x = _mm256_mul_ps(a.x, scalar);
			result.y = _mm256_mul_ps(a.y, scalar);
			result.z = _mm256_mul_ps(a.z, scalar);
			return result;
		}

		__forceinline Vec3Packet operator/(const Vec3Packet& a, const __m256& scalar)
		{
			Vec3Packet result;
			result.x = _mm256_div_ps(a.x, scalar);
			result.y = _mm256_div_ps(a.y, scalar);
			result.z = _mm256_div_ps(a.z, scalar);
			return result;
		}

		__forceinline __m256 Dot(const Vec3Packet& a, const Vec3Packet& b)
		{
			__m256 x2 = _mm256_add_ps(_mm256_mul_ps(a.x, b.x), _mm256_mul_ps(a.y, b.y));
			x2 = _mm256_add_ps(x2, _mm256_mul_ps(a.z, b.z));
			return x2;
		}

		__forceinline void Normalize(Vec3Packet& a)
		{
			__m256 length = Dot(a, a);
			__m256 nr = _mm256_rsqrt_ps(length);
			__m256 muls = _mm256_mul_ps(_mm256_mul_ps(length, nr), nr);
			length = _mm256_mul_ps(_mm256_mul_ps(Constants::oneHalf, nr), _mm256_sub_ps(Constants::three, muls));

			a.x = _mm256_mul_ps(a.x, length);
			a.y = _mm256_mul_ps(a.y, length);
			a.z = _mm256_mul_ps(a.z, length);
		}

		__forceinline Vec3Packet And(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_and_ps(a.x, b.x);
			result.y = _mm256_and_ps(a.y, b.y);
			result.z = _mm256_and_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet And(const __m256& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_and_ps(a, b.x);
			result.y = _mm256_and_ps(a, b.y);
			result.z = _mm256_and_ps(a, b.z);
			return result;
		}

		__forceinline Vec3Packet AndNot(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_andnot_ps(a.x, b.x);
			result.y = _mm256_andnot_ps(a.y, b.y);
			result.z = _mm256_andnot_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet AndNot(const __m256& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_andnot_ps(a, b.x);
			result.y = _mm256_andnot_ps(a, b.y);
			result.z = _mm256_andnot_ps(a, b.z);
			return result;
		}

		__forceinline Vec3Packet Or(const Vec3Packet& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_or_ps(a.x, b.x);
			result.y = _mm256_or_ps(a.y, b.y);
			result.z = _mm256_or_ps(a.z, b.z);
			return result;
		}

		__forceinline Vec3Packet Or(const __m256& a, const Vec3Packet& b)
		{
			Vec3Packet result;
			result.x = _mm256_or_ps(a, b.x);
			result.y = _mm256_or_ps(a, b.y);
			result.z = _mm256_or_ps(a, b.z);
			return result;
		}

		__forceinline __m256 RaySphereIntersection
		(
			const Vec3Packet& rayDirection,
			const Vec3Packet& sphereOrigin,
			const __m256& sphereRadiusSq,
			__m256& t
		)
		{
			__m256 tca = Dot(sphereOrigin, rayDirection);
			
			auto result = _mm256_cmp_ps(tca, Constants::zero, _CMP_GE_OQ);
			if (_mm256_movemask_ps(result) == 0)
			{
				return result;
			}

			auto d2 = _mm256_sub_ps(Dot(sphereOrigin, sphereOrigin), _mm256_mul_ps(tca, tca));

			result = _mm256_cmp_ps(d2, sphereRadiusSq, _CMP_LE_OQ);
			if (_mm256_movemask_ps(result) == 0)
			{
				return result;
			}

			__m256 thc = _mm256_sqrt_ps(_mm256_sub_ps(sphereRadiusSq, d2));
			t = _mm256_add_ps(tca, thc);

			__m256 t0 = _mm256_add_ps(tca, thc);
			__m256 t1 = _mm256_sub_ps(tca, thc);

			auto tresult = _mm256_cmp_ps(t0, t1, _CMP_LE_OQ);
			t = _mm256_or_ps(_mm256_and_ps(tresult, t0), _mm256_andnot_ps(tresult, t1));

			return result;
		}

	}

}

#endif
