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
		Sphereflake(size_t width, size_t height);

		~Sphereflake();

		void Initialize();

		void SetView(const vec3& origin, const vec3& topLeft, const vec3& topRight, const vec3& bottomLeft);

		const GBuffer& GetGBuffer() const
		{
			return m_GBuffer;
		}

		int GetMaxDepthReached() const
		{
			return m_MaxDepthReached;
		}

		void ResetMaxDepthReached()
		{
			m_MaxDepthReached = 0;
		}

		long long GetRaysPerSecond() const
		{
			return m_RaysPerSecond;
		}

		void ResetRaysPerSecond()
		{
			m_RaysPerSecond = 0;
		}

		float GetClosestSphereDistance() const
		{
			return m_ClosestSphereDistance;
		}

		void ResetClosestSphereDistance()
		{
			m_ClosestSphereDistance = std::numeric_limits<float>::max();
		}

		private:
		SIMD::Vec3Packet m_RayOrigin;
		SIMD::Vec3Packet m_TopLeft;
		SIMD::Vec3Packet m_TopRight;
		SIMD::Vec3Packet m_BottomLeft;
		SIMD::Matrix4 m_RootTransform;
		SIMD::Matrix4 m_ChildTransforms[9];

		void DoImagePart();

		void ComputeChildTransformations();

		size_t m_Width;
		size_t m_Height;
		GBuffer m_GBuffer;

		std::vector<std::shared_ptr<std::thread>> m_Threads;

		bool m_Deinitialize;

		int m_MaxDepthReached;
		long long m_RaysPerSecond;;
		float m_ClosestSphereDistance;

		vec3 m_RayOriginVec3;

		SIMD::VecType IntersectSphereflake
		(
			const SIMD::Vec3Packet& rayDirection,
			const SIMD::Matrix4& parentTransform,
			SIMD::VecType& minT,
			SIMD::Vec3Packet& position,
			SIMD::Vec3Packet& normal,
			float parentRadius,
			int depth
		)
		{
			float radiusScalar = parentRadius / 3.0f;

#ifdef __ARCH_NO_AVX

			__m128 radius = _mm_set1_ps(radiusScalar);
			__m128 doubleRadiusSq = _mm_mul_ps(radius, SIMD::Constants::two);
			doubleRadiusSq = _mm_mul_ps(doubleRadiusSq, doubleRadiusSq);
			__m128 t;

#else

			__m256 radius = _mm256_broadcast_ss(&radiusScalar);
			__m256 doubleRadiusSq = _mm256_mul_ps(radius, SIMD::Constants::two);
			doubleRadiusSq = _mm256_mul_ps(doubleRadiusSq, doubleRadiusSq);
			__m256 t;

#endif

			SIMD::Vec3Packet sphereOrigin;
			sphereOrigin.Set(vec3(parentTransform.Extract(3)));

			// intersect with the bounding volume of the current depth
			auto result = RaySphereIntersection(rayDirection, sphereOrigin, doubleRadiusSq, t);

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

			auto depthResult = _mm256_cmp_ps(_mm256_sqrt_ps(_mm256_div_ps(t, radius)), SIMD::Constants::seventy, _CMP_LT_OQ);
			auto tLessThanZeroResult = _mm256_cmp_ps(t, SIMD::Constants::zero, _CMP_LT_OQ);

			if (_mm256_movemask_ps(_mm256_or_ps(depthResult, tLessThanZeroResult)) == 0)
			{
				// sphere is behind all rays or depth is too large
				return result;
			}

#endif

			if (depth > m_MaxDepthReached)
			{
				m_MaxDepthReached = depth;
			}

			float scale = (4.0f / 3.0f) * radiusScalar;
			__m128 translationScale = _mm_set_ps(1.0f, scale, scale, scale);

			for (auto i = 0; i < 9; i++)
			{
				auto transform = m_ChildTransforms[i];
				transform.rows[3] = _mm_mul_ps(transform.rows[3], translationScale);
				auto worldTransform = parentTransform * transform;

				IntersectSphereflake(rayDirection, worldTransform, minT, position, normal, radiusScalar, depth + 1);
			}

#ifdef __ARCH_NO_AVX

			__m128 radiusSq = _mm_mul_ps(radius, radius);

#else
			
			__m256 radiusSq = _mm256_mul_ps(radius, radius);

#endif


			result = RaySphereIntersection(rayDirection, sphereOrigin, radiusSq, t);

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
			auto selfNormal = selfPosition - sphereOrigin;
			SIMD::Normalize(selfNormal);

			// mask results
			position = SIMD::Or(SIMD::AndNot(result, position), SIMD::And(result, selfPosition));
			normal = SIMD::Or(SIMD::AndNot(result, normal), SIMD::And(result, selfNormal));
			return result;
		}

	};

}

#endif
