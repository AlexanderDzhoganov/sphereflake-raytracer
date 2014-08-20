#ifndef __RAYTRACE_SPHEREFLAKE_H
#define __RAYTRACE_SPHEREFLAKE_H

#define SPHEREFLAKE_MAX_DEPTH 16

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

		Sphereflake(size_t width, size_t height) : m_Width(width), m_Height(height)
		{
			m_GBuffer.positions.resize(width * height);
			m_GBuffer.normals.resize(width * height);

			PrecomputeChildTransforms();
		}

		~Sphereflake()
		{
			m_Deinitialize = true;

			for (auto&& i : m_Threads)
			{
				i->join();
			}
		}

		void Initialize()
		{
			auto threadCount = std::thread::hardware_concurrency();

			size_t splitW = m_Width;
			size_t splitH = m_Height / threadCount;

			for (auto i = 0u; i < threadCount; i++)
			{
				m_Threads.push_back(std::make_unique<std::thread>(std::bind(&Sphereflake::DoImagePart, this)));
			}
		}

		const GBuffer& GetGBuffer()
		{
			return m_GBuffer;
		}

		void UpdateCamera(Camera* camera)
		{
			m_RayOrigin.Set(camera->GetPosition());
			m_TopLeft.Set(camera->GetTopLeft());
			m_TopRight.Set(camera->GetTopRight());
			m_BottomLeft.Set(camera->GetBottomLeft());
		}

		private:
		void DoImagePart()
		{
			std::mt19937 mt;
			mt.seed((unsigned long)time(NULL));
			std::uniform_int_distribution<unsigned int> rnd(0);
			
#ifdef __ARCH_SSE

			auto width = _mm_set1_ps((float)m_Width);
			auto height = _mm_set1_ps((float)m_Height);

#else
			
			auto width = _mm256_set1_ps((float) m_Width);
			auto height = _mm256_set1_ps((float) m_Height);
			
#endif

			SSE::Vec3Packet position;
			SSE::Vec3Packet normal;
		//	mat4 transform = mat4(1.0f);
			
			SSE::Matrix4 transform;
			transform.Set(mat4(1.0));

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			unsigned long long sobolCounter = 0;

			for (;;)
			{
			
#ifdef __ARCH_SSE

				auto x0 = floorf(Sobol::Sample(sobolCounter, 0, rnd(mt)) * (m_Width - 1));
				auto y0 = floorf(Sobol::Sample(sobolCounter, 1, rnd(mt)) * (m_Height - 1));
				sobolCounter++;
			
				float xa[4] = { x0, x0 + 1, x0, x0 + 1 };
				float ya[4] = { y0, y0, y0 + 1, y0 + 1 };

				auto x = _mm_set_ps(xa[3], xa[2], xa[1], xa[0]);
				auto y = _mm_set_ps(ya[3], ya[2], ya[1], ya[0]);

				auto uvx = _mm_div_ps(x, width);
				auto uvy = _mm_div_ps(y, height);

				__m128 minT = _mm_set1_ps(std::numeric_limits<float>::max());

#else
				auto x0 = 1 + floorf(Sobol::Sample(sobolCounter, 0, rnd(mt)) * (m_Width - 2));
				auto y0 = 1 + floorf(Sobol::Sample(sobolCounter, 1, rnd(mt)) * (m_Height - 2));
				sobolCounter++;

				float xa[8] = { x0, x0 + 1, x0 + 1, x0    , x0    , x0 + 1, x0 - 1, x0 - 1};
				float ya[8] = { y0, y0 + 1, y0    , y0 + 1, y0 - 1, y0 - 1, y0    , y0 - 1};

				auto x = _mm256_set_ps(xa[7], xa[6], xa[5], xa[4], xa[3], xa[2], xa[1], xa[0]);
				auto y = _mm256_set_ps(ya[7], ya[6], ya[5], ya[4], ya[3], ya[2], ya[1], ya[0]);

				auto uvx = _mm256_div_ps(x, width);
				auto uvy = _mm256_div_ps(y, height);
				  
				__m256 minT = _mm256_set1_ps(std::numeric_limits<float>::max());

#endif

				auto directionHorizontalPart = m_TopLeft + (m_TopRight - m_TopLeft) * uvx;
				auto directionVerticalPart = (m_BottomLeft - m_TopLeft) * uvy;

				auto targetDirection = directionHorizontalPart + directionVerticalPart;
				auto rayDirection = targetDirection - m_RayOrigin;
				Normalize(rayDirection);

				position.Set(vec3(0.0f));
				normal.Set(vec3(0.0f));

				RenderSphereflake(m_RayOrigin, rayDirection, transform, 3.f, 0, minT, position, normal);

#ifdef __ARCH_SSE

				size_t loopCount = 4;

#else

				size_t loopCount = 8;

#endif

				for (auto q = 0u; q < loopCount; q++)
				{
					auto idx = (size_t)xa[q] + (size_t)ya[q] * m_Width;
					if (idx > m_GBuffer.positions.size())
					{
						continue;
					}

					m_GBuffer.positions[idx] = vec4(position.Extract(q), 1.0f);
					m_GBuffer.normals[idx] = vec4(normal.Extract(q), 1.0f);
					raysPerSecond++;
				}

				if (m_Deinitialize)
				{
					return;
				}
			}
		}

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

		void PrecomputeChildTransforms()
		{
			for (auto i = 0u; i < 6; i++)
			{
				float longitude = radians(90.f);
				float latitude = radians(60.0f * (float)i);

				vec3 displacement = normalize(SphericalToWorldCoodinates(longitude, latitude));

				auto transform = CreateRotationMatrix(vec3(90, 90 + i * 60, 0));
				transform[3][0] = displacement[0];
				transform[3][1] = displacement[1];
				transform[3][2] = displacement[2];

				m_ChildTransforms[i].Set(transform);
			}

			vec3 rotations[3];
			rotations[0] = vec3(0, 0, 0);
			rotations[1] = vec3(0, 0, 0);
			rotations[2] = vec3(60, 0, 0);

			for (auto i = 0u; i < 3; i++)
			{
				float longitude = radians(60.0f);
				float latitude = radians(30.0f + 120.0f * (float)i);

				vec3 displacement = normalize(SphericalToWorldCoodinates(longitude, latitude));

				auto transform = CreateRotationMatrix(rotations[i]);
				transform[3][0] = displacement[0];
				transform[3][1] = displacement[1];
				transform[3][2] = displacement[2];

				m_ChildTransforms[6 + i].Set(transform);
			}
		}

#ifdef __ARCH_SSE

		inline __m128 RenderSphereflake
		(
			const SSE::Vec3Packet& rayOrigin,
			const SSE::Vec3Packet& rayDirection,
			const mat4& parentTransform,
			float parentRadius,
			int depth,
			__m128& minT,
			SSE::Vec3Packet& position,
			SSE::Vec3Packet& normal
		)

#else

		inline __m256 RenderSphereflake
		(
			const SSE::Vec3Packet& rayOrigin,
			const SSE::Vec3Packet& rayDirection,
			const SSE::Matrix4& parentTransform,
			float parentRadius,
			int depth,
			__m256& minT,
			SSE::Vec3Packet& position,
			SSE::Vec3Packet& normal
		)

#endif

		{
			if (depth >= SPHEREFLAKE_MAX_DEPTH)
			{
				return SSE::Constants::zero;
			}

			float radiusScalar = parentRadius / 3.0f;

#ifdef __ARCH_SSE

			__m128 radius = _mm_set1_ps(radiusScalar);
			__m128 radiusSq = _mm_mul_ps(radius, radius);
			__m128 doubleRadiusSq = _mm_mul_ps(radius, SSE::Constants::two);
			doubleRadiusSq = _mm_mul_ps(doubleRadiusSq, doubleRadiusSq);
			__m128 t;

#else

			__m256 radius = _mm256_set1_ps(radiusScalar);
			__m256 radiusSq = _mm256_mul_ps(radius, radius);
			__m256 doubleRadiusSq = _mm256_mul_ps(radius, SSE::Constants::two);
			doubleRadiusSq = _mm256_mul_ps(doubleRadiusSq, doubleRadiusSq);
			__m256 t;

#endif

			vec4 sphereOrigin = parentTransform.Extract(3);

			SSE::Vec3Packet sphereOriginPacket;
			sphereOriginPacket.Set(vec3(sphereOrigin));

			auto result = RaySphereIntersection(rayOrigin, rayDirection, sphereOriginPacket, doubleRadiusSq, t);

#ifdef __ARCH_SSE

			if (_mm_movemask_ps(result) == 0)
			{
				// all rays miss bounding sphere
				return result;
			}

			auto depthResult = _mm_cmplt_ps(_mm_sqrt_ps(_mm_div_ps(t, radius)), SSE::Constants::hundred);
			auto tLessThanZeroResult = _mm_cmplt_ps(t, SSE::Constants::zero);

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

			auto depthResult = _mm256_cmp_ps(_mm256_sqrt_ps(_mm256_div_ps(t, radius)), SSE::Constants::hundred, _CMP_LT_OQ);
			auto tLessThanZeroResult = _mm256_cmp_ps(t, SSE::Constants::zero, _CMP_LT_OQ);

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
				_mm_prefetch((const char*)m_ChildTransforms[i].m, _MM_HINT_T0);

				float scale = (4.0f / 3.0f) * radiusScalar;
				__m128 translationScale = _mm_set_ps(1.0f, scale, scale, scale);
				auto transform = m_ChildTransforms[i];
				transform.rows[3] = _mm_mul_ps(transform.rows[3], translationScale);
				auto worldTransform = parentTransform * transform;

				RenderSphereflake(rayOrigin, rayDirection, worldTransform, radiusScalar, depth + 1, minT, position, normal);
			}

			result = RaySphereIntersection(rayOrigin, rayDirection, sphereOriginPacket, radiusSq, t);

#ifdef __ARCH_SSE

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
			auto selfNormal = selfPosition - sphereOriginPacket + rayOrigin;
			SSE::Normalize(selfNormal);

			// mask results
			position = SSE::BitwiseOr(SSE::BitwiseAndNot(result, position), SSE::BitwiseAnd(result, selfPosition));
			normal = SSE::BitwiseOr(SSE::BitwiseAndNot(result, normal), SSE::BitwiseAnd(result, selfNormal));
			return result;
		}

		size_t m_Width;
		size_t m_Height;
		GBuffer m_GBuffer;

		SSE::Matrix4 m_ChildTransforms[9];

		std::vector<std::unique_ptr<std::thread>> m_Threads;

		bool m_Deinitialize = false;

		SSE::Vec3Packet m_RayOrigin;
		SSE::Vec3Packet m_TopLeft;
		SSE::Vec3Packet m_TopRight;
		SSE::Vec3Packet m_BottomLeft;

	};

}

#endif
