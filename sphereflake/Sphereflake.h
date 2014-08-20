#ifndef __RAYTRACE_SPHEREFLAKE_H
#define __RAYTRACE_SPHEREFLAKE_H

namespace SphereflakeRaytracer
{

	struct GBuffer
	{
		std::vector<vec4> positions;
		std::vector<vec4> normals;
	};

	class Sphereflake
	{

		public:
		int maxDepthReached = 0;
		long long raysPerSecond = 0;
		float closestSphereDistance = 0.0f;

		Sphereflake(size_t width, size_t height);

		~Sphereflake();

		void Initialize();

		void SetView(const vec3& origin, const vec3& topLeft, const vec3& topRight, const vec3& bottomLeft);

		const GBuffer& GetGBuffer()
		{
			return m_GBuffer;
		}

		private:
		void DoImagePart();

		void ComputeChildTransformations();

		size_t m_Width;
		size_t m_Height;
		GBuffer m_GBuffer;

		SIMD::Matrix4 m_ChildTransforms[9];

		std::vector<std::unique_ptr<std::thread>> m_Threads;

		bool m_Deinitialize = false;

		SIMD::Vec3Packet m_RayOrigin;
		SIMD::Vec3Packet m_TopLeft;
		SIMD::Vec3Packet m_TopRight;
		SIMD::Vec3Packet m_BottomLeft;

		__declspec(align(64)) __m256 RenderSphereflake
		(
			const SIMD::Vec3Packet& rayOrigin,
			const SIMD::Vec3Packet& rayDirection,
			const SIMD::Matrix4& parentTransform,
			float parentRadius,
			int depth,
			SIMD::VecType& minT,
			SIMD::Vec3Packet& position,
			SIMD::Vec3Packet& normal
		)
		{
			float radiusScalar = parentRadius / 3.0f;

#ifdef __ARCH_NO_AVX

			__m128 radius = _mm_set1_ps(radiusScalar);
			__m128 radiusSq = _mm_mul_ps(radius, radius);
			__m128 doubleRadiusSq = _mm_mul_ps(radius, SIMD::Constants::two);
			doubleRadiusSq = _mm_mul_ps(doubleRadiusSq, doubleRadiusSq);
			__m128 t;

#else

			__m256 radius = _mm256_set1_ps(radiusScalar);
			__m256 radiusSq = _mm256_mul_ps(radius, radius);
			__m256 doubleRadiusSq = _mm256_mul_ps(radius, SIMD::Constants::two);
			doubleRadiusSq = _mm256_mul_ps(doubleRadiusSq, doubleRadiusSq);
			__m256 t;

#endif

			SIMD::Vec3Packet sphereOrigin;
			sphereOrigin.Set(vec3(parentTransform.Extract(3)));

			auto result = RaySphereIntersection(rayOrigin, rayDirection, sphereOrigin, doubleRadiusSq, t);

#ifdef __ARCH_NO_AVX

			if (_mm_movemask_ps(result) == 0)
			{
				// all rays miss bounding sphere
				return result;
			}

			auto depthResult = _mm_cmplt_ps(_mm_sqrt_ps(_mm_div_ps(t, radius)), SIMD::Constants::sixty);
			auto tLessThanZeroResult = _mm_cmplt_ps(t, SIMD::Constants::zero);

			if (_mm_movemask_ps(_mm_or_ps(depthResult, tLessThanZeroResult)) == 0)
			{
				// sphere is behind all rays or depth is too large
				return result;
			}

#else

			if (_mm256_movemask_ps(result) == 0)
			{
				// all rays miss bounding sphere
				return result;
			}

			auto depthResult = _mm256_cmp_ps(_mm256_sqrt_ps(_mm256_div_ps(t, radius)), SIMD::Constants::sixty, _CMP_LT_OQ);
			auto tLessThanZeroResult = _mm256_cmp_ps(t, SIMD::Constants::zero, _CMP_LT_OQ);

			if (_mm256_movemask_ps(_mm256_or_ps(depthResult, tLessThanZeroResult)) == 0)
			{
				// sphere is behind all rays or depth is too large
				return result;
			}

#endif

			if (depth > maxDepthReached)
			{
				maxDepthReached = depth;
			}

			for (auto i = 0; i < 9; i++)
			{
				_mm_prefetch((const char*)&(m_ChildTransforms[i].m[0][0]), _MM_HINT_T0);
				_mm_prefetch((const char*)&(parentTransform.m[0][0]), _MM_HINT_T0);

				float scale = (4.0f / 3.0f) * radiusScalar;
				__m128 translationScale = _mm_set_ps(1.0f, scale, scale, scale);
				auto transform = m_ChildTransforms[i];
				transform.rows[3] = _mm_mul_ps(transform.rows[3], translationScale);
				auto worldTransform = parentTransform * transform;

				RenderSphereflake(rayOrigin, rayDirection, worldTransform, radiusScalar, depth + 1, minT, position, normal);
			}

			result = RaySphereIntersection(rayOrigin, rayDirection, sphereOrigin, radiusSq, t);

#ifdef __ARCH_NO_AVX

			// depth comparison
			auto minTResult = _mm_cmplt_ps(t, minT);
			result = _mm_and_ps(result, minTResult);

			if (_mm_movemask_ps(result) == 0)
			{
				// all rays don't pass depth test
				return result;
			}

			minT = _mm_or_ps(_mm_andnot_ps(result, minT), _mm_and_ps(result, t));

#else

			// depth comparison
			auto minTResult = _mm256_cmp_ps(t, minT, _CMP_LT_OQ);
			result = _mm256_and_ps(result, minTResult);

			if (_mm256_movemask_ps(result) == 0)
			{
				// all rays don't pass depth test
				return result;
			}

			minT = _mm256_or_ps(_mm256_andnot_ps(result, minT), _mm256_and_ps(result, t));

#endif

			// calculate resulting view-space position and normal
			auto selfPosition = rayDirection * t;
			auto selfNormal = selfPosition - sphereOrigin + rayOrigin;
			SIMD::Normalize(selfNormal);

			// mask results
			position = SIMD::Or(SIMD::AndNot(result, position), SIMD::And(result, selfPosition));
			normal = SIMD::Or(SIMD::AndNot(result, normal), SIMD::And(result, selfNormal));
			return result;
		}

	};

}

#endif
