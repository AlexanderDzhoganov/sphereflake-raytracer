#ifndef __RAYTRACE_SPHEREFLAKE_H
#define __RAYTRACE_SPHEREFLAKE_H

#define MAX_DEPTH 2

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

			std::uniform_int_distribution<size_t> widthRand(0, m_Width - 2);
			std::uniform_int_distribution<size_t> heightRand(0, m_Height - 2);

			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			for (;;)
			{
				auto x0 = widthRand(mt);
				auto y0 = heightRand(mt);

				size_t xa[4] = { x0, x0 + 1, x0, x0 + 1 };
				size_t ya[4] = { y0, y0, y0 + 1, y0 + 1 };

				auto x = _mm_set_ps((float)xa[3], (float)xa[2], (float)xa[1], (float)xa[0]);
				auto y = _mm_set_ps((float)ya[3], (float)ya[2], (float)ya[1], (float)ya[0]);

				auto width = _mm_set1_ps((float)m_Width);
				auto height = _mm_set1_ps((float)m_Height);

				auto uvx = _mm_div_ps(x, width);
				auto uvy = _mm_div_ps(y, height);

				auto directionHorizontalPart = m_TopLeft + (m_TopRight - m_TopLeft) * uvx;
				auto directionVerticalPart = (m_BottomLeft - m_TopLeft) * uvy;

				auto targetDirection = directionHorizontalPart + directionVerticalPart;
				auto rayDirection = targetDirection - m_RayOrigin;
				Normalize(rayDirection);

				Vec3Packet position;
				position.Set(vec3(0.0f));

				Vec3Packet normal;
				normal.Set(vec3(0.0f));

				mat4 transform = mat4(1.0f);
				__m128 minT = _mm_set1_ps(std::numeric_limits<float>::max());

				auto result = RenderSphereflake(m_RayOrigin, rayDirection, transform, 3.f, 0, minT, position, normal);

				for (auto q = 0u; q < 4; q++)
				{
					auto idx = xa[q] + ya[q] * m_Width;
					auto rposition = vec4(position.Extract(q), 1.0f);
					auto rnormal = vec4(normal.Extract(q), 1.0f);
					m_GBuffer.positions[idx] = rposition;
					m_GBuffer.normals[idx] = rnormal;
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
				m_ChildTransforms[i] = transform;
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
				m_ChildTransforms[6 + i] = transform;
			}
		}

		mat4 CreateRotationMatrix(const vec3& rot)
		{
			return glm::rotate(radians(rot.x), vec3(1, 0, 0)) *
				glm::rotate(radians(rot.y), vec3(0, 1, 0)) *
				glm::rotate(radians(rot.z), vec3(0, 0, 1));
		}

		inline __m128 RenderSphereflake
			(
			const Vec3Packet& rayOrigin,
			const Vec3Packet& rayDirection,
			const mat4& parentTransform,
			float parentRadius,
			int depth,
			__m128& minT,
			Vec3Packet& position,
			Vec3Packet& normal
			)
		{
			if (depth >= MAX_DEPTH)
			{
				return SSEConstants::zero;
			}

			float radiusScalar = parentRadius / 3.0f;
			__m128 radius = _mm_set1_ps(radiusScalar);
			__m128 radiusSq = _mm_mul_ps(radius, radius);

			__m128 doubleRadiusSq = _mm_mul_ps(radius, SSEConstants::two);
			doubleRadiusSq = _mm_mul_ps(doubleRadiusSq, doubleRadiusSq);

			const vec3& sphereOrigin = vec3(parentTransform[3]);

			Vec3Packet sphereOriginPacket;
			sphereOriginPacket.Set(sphereOrigin);

			__m128 t;
			auto result = RaySphereIntersection(rayOrigin, rayDirection, sphereOriginPacket, doubleRadiusSq, t);
			if (_mm_movemask_ps(result) == 0)
			{
				return result;
			}

			auto depthResult = _mm_cmplt_ps(_mm_sqrt_ps(_mm_div_ps(t, radius)), SSEConstants::hundred);
			auto tLessThanZeroResult = _mm_cmplt_ps(t, SSEConstants::zero);

			if (_mm_movemask_ps(_mm_or_ps(depthResult, tLessThanZeroResult)) == 0)
			{
				return result;
			}

			if (depth > maxDepthReached)
			{
				maxDepthReached = depth;
			}

			for (auto i = 0; i < 9; i++)
			{
				auto transform = m_ChildTransforms[i];
				float translationScale = (4.0f / 3.0f) * radiusScalar;
				transform[3][0] *= translationScale;
				transform[3][1] *= translationScale;
				transform[3][2] *= translationScale;
				auto worldTransform = parentTransform * transform;

				RenderSphereflake(rayOrigin, rayDirection, worldTransform, radiusScalar, depth + 1, minT, position, normal);
			}

			result = RaySphereIntersection(rayOrigin, rayDirection, sphereOriginPacket, radiusSq, t);

			auto minTResult = _mm_cmplt_ps(t, minT);
			result = _mm_and_ps(result, minTResult);

			if (_mm_movemask_ps(result) == 0)
			{
				return result;
			}

			minT = _mm_or_ps(_mm_andnot_ps(result, minT), _mm_and_ps(result, t));

			auto rPosition = rayOrigin + (rayDirection * t);

			auto pPositionX = _mm_andnot_ps(result, position.x);
			auto pPositionY = _mm_andnot_ps(result, position.y);
			auto pPositionZ = _mm_andnot_ps(result, position.z);

			position.x = _mm_or_ps(pPositionX, _mm_and_ps(result, rPosition.x));
			position.y = _mm_or_ps(pPositionY, _mm_and_ps(result, rPosition.y));
			position.z = _mm_or_ps(pPositionZ, _mm_and_ps(result, rPosition.z));

			auto rNormal = rPosition - sphereOriginPacket;
			Normalize(rNormal);

			auto pNormalX = _mm_andnot_ps(result, normal.x);
			auto pNormalY = _mm_andnot_ps(result, normal.y);
			auto pNormalZ = _mm_andnot_ps(result, normal.z);

			normal.x = _mm_or_ps(pNormalX, _mm_and_ps(result, rNormal.x));
			normal.y = _mm_or_ps(pNormalY, _mm_and_ps(result, rNormal.y));
			normal.z = _mm_or_ps(pNormalZ, _mm_and_ps(result, rNormal.z));
			return result;
		}

		size_t m_Width;
		size_t m_Height;
		GBuffer m_GBuffer;

		mat4 m_ChildTransforms[9];
		std::vector<std::unique_ptr<std::thread>> m_Threads;

		bool m_Deinitialize = false;

		Vec3Packet m_RayOrigin;
		Vec3Packet m_TopLeft;
		Vec3Packet m_TopRight;
		Vec3Packet m_BottomLeft;

	};

}

#endif
