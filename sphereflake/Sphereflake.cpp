#pragma warning (push, 0)
#pragma warning (disable: 4530) // disable warnings from code not under our control

#include <thread>
#include <random>
#include <memory>
#include <iostream>
#include <mmintrin.h>

#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include <gtx/simd_mat4.hpp>
#include <gtx/simd_vec4.hpp>
#include <gtx/transform.hpp>

using namespace glm;

#define GLFW_DLL
#include <GLFW/glfw3.h>

#pragma warning (pop)

#include "Sobol.h"

#ifdef __ARCH_NO_AVX
#include "SIMD_SSE.h"
#else
#include "SIMD_AVX.h"
#endif

#include "Sphereflake.h"
#include "Util.h"

extern GLFWwindow* window;

namespace SphereflakeRaytracer
{

	Sphereflake::Sphereflake(size_t width, size_t height) :
		m_Width(width),
		m_Height(height),
		m_Deinitialize(false),
		m_MaxDepthReached(0),
		m_RaysPerSecond(0),
		m_ClosestSphereDistance(std::numeric_limits<float>::max())
	{
		m_GBuffer.positions.resize(width * height);
		m_GBuffer.normals.resize(width * height);

		ComputeChildTransformations();
	}

	Sphereflake::~Sphereflake()
	{
		m_Deinitialize = true;

		for (auto&& i : m_Threads)
		{
			i->join();
		}
	}

	void Sphereflake::Initialize()
	{
		auto threadCount = std::thread::hardware_concurrency();
		for (auto i = 0u; i < threadCount; i++)
		{
			m_Threads.push_back(std::make_shared<std::thread>(std::bind(&Sphereflake::DoImagePart, this)));
		}
	}

	void Sphereflake::SetView(const vec3& origin, const vec3& topLeft, const vec3& topRight, const vec3& bottomLeft)
	{
		m_RayOriginVec3 = origin;
		m_RayOrigin.Set(origin);
		m_TopLeft.Set(topLeft); 
		m_TopRight.Set(topRight);
		m_BottomLeft.Set(bottomLeft);
		m_RootTransform.Set(translate(-m_RayOriginVec3) * CreateRotationMatrix(vec3(90, 0, 0)));
	}

	void Sphereflake::DoImagePart()
	{
		std::mt19937 mt;
		mt.seed((unsigned long) time(NULL));
		std::uniform_int_distribution<unsigned int> rnd(0);

#ifdef __ARCH_NO_AVX

		auto width = _mm_set1_ps((float) m_Width);
		auto height = _mm_set1_ps((float) m_Height);

#else

		auto width = _mm256_set1_ps((float) m_Width);
		auto height = _mm256_set1_ps((float) m_Height);

#endif

		SIMD::Vec3Packet position;
		SIMD::Vec3Packet normal;

		float floatMax = std::numeric_limits<float>::max();
		unsigned long long sobolCounter = 0;

		float spinUp = 1.0f;

		for (;;)
		{

#ifdef __ARCH_NO_AVX

			auto x0 = floorf(Sobol::Sample(sobolCounter, 0, rnd(mt)) * (m_Width - 1));
			auto y0 = floorf(Sobol::Sample(sobolCounter, 1, rnd(mt)) * (m_Height - 1));
			sobolCounter++;

			float xa[4] = { x0, x0 + 1, x0, x0 + 1 };
			float ya[4] = { y0, y0, y0 + 1, y0 + 1 };

			auto x = _mm_set_ps(xa[3], xa[2], xa[1], xa[0]);
			auto y = _mm_set_ps(ya[3], ya[2], ya[1], ya[0]);

			auto uvx = _mm_div_ps(x, width);
			auto uvy = _mm_div_ps(y, height);

			union
			{
				__m128 minT;
				float minTArray[4];
			};

			minT = _mm_set1_ps(floatMax);

#else
			auto x0 = 1 + floorf(Sobol::Sample(sobolCounter, 0, rnd(mt)) * (m_Width - 2));
			auto y0 = 1 + floorf(Sobol::Sample(sobolCounter, 1, rnd(mt)) * (m_Height - 2));
			sobolCounter++;

			float xa[8] = { x0, x0 + 1, x0 + 1, x0, x0, x0 + 1, x0 - 1, x0 - 1 };
			float ya[8] = { y0, y0 + 1, y0, y0 + 1, y0 - 1, y0 - 1, y0, y0 - 1 };

			auto x = _mm256_set_ps(xa[7], xa[6], xa[5], xa[4], xa[3], xa[2], xa[1], xa[0]);
			auto y = _mm256_set_ps(ya[7], ya[6], ya[5], ya[4], ya[3], ya[2], ya[1], ya[0]);

			auto uvx = _mm256_div_ps(x, width);
			auto uvy = _mm256_div_ps(y, height);

			union
			{
				__m256 minT;
				float minTArray[8];
			};

			minT = _mm256_broadcast_ss(&floatMax);

#endif

			auto directionHorizontalPart = m_TopLeft + (m_TopRight - m_TopLeft) * uvx;
			auto directionVerticalPart = (m_BottomLeft - m_TopLeft) * uvy;

			auto targetDirection = directionHorizontalPart + directionVerticalPart;
			auto rayDirection = targetDirection - m_RayOrigin;
			Normalize(rayDirection);

			position.Set(vec3(0.0f));
			normal.Set(vec3(0.0f));

			auto transform = m_RootTransform;
			IntersectSphereflake(rayDirection, transform, minT, position, normal, 3.0f, 0);

#ifdef __ARCH_NO_AVX

			size_t loopCount = 4;

#else

			size_t loopCount = 8;

#endif
			m_RaysPerSecond += loopCount;

			for (auto q = 0u; q < loopCount; q++)
			{
				auto idx = (size_t) xa[q] + (size_t) ya[q] * m_Width;
				if (idx > m_GBuffer.positions.size())
				{
					continue;
				}

				m_GBuffer.positions[idx] = vec4(position.Extract(q), 1.0f);
				m_GBuffer.normals[idx] = vec4(normal.Extract(q), 1.0f);

				if (minTArray[q] < m_ClosestSphereDistance)
				{
					m_ClosestSphereDistance = minTArray[q];
				}
			}

			if(spinUp > 0.0f) // gradually spin-up the threads so we don't upset the GL thread
			{
				std::this_thread::sleep_for(std::chrono::microseconds((int)spinUp * 1000));
				spinUp -= spinUp / 1000.0f;
			}
			
			if (m_Deinitialize)
			{
				return;
			}
		}
	}

	void Sphereflake::ComputeChildTransformations()
	{
		for (auto i = 0u; i < 6; i++)
		{
			float longitude = radians(90.f);
			float latitude = radians(60.0f * (float) i);

			vec3 displacement = normalize(SphericalToWorldCoodinates(longitude, latitude));

			auto transform = CreateRotationMatrix(vec3(90, 90 + i * 60, 0));
			transform[3][0] = displacement[0];
			transform[3][1] = displacement[1];
			transform[3][2] = displacement[2];

			m_ChildTransforms[i].Set(transform);
		}

		static vec3 rotations[3] = { vec3(325, 45, 15), vec3(145, 230, 165), vec3(60, 0, 0) };

		for (auto i = 0u; i < 3; i++)
		{
			float longitude = radians(30.0f);
			float latitude = radians(30.0f + 120.0f * (float) i);

			vec3 displacement = normalize(SphericalToWorldCoodinates(longitude, latitude));

			auto transform = CreateRotationMatrix(rotations[i]);
			transform[3][0] = displacement[0];
			transform[3][1] = displacement[1];
			transform[3][2] = displacement[2];

			m_ChildTransforms[6 + i].Set(transform);
		}
	}

}