#ifndef __RAYTRACE_SPHEREFLAKE_H
#define __RAYTRACE_SPHEREFLAKE_H

#define MAX_DEPTH 32

struct GBuffer
{
	std::vector<vec4> positions;
	std::vector<vec4> normals;
};

class RaytraceSphereflake
{

	public:
	int maxDepthReached = 0;
	long long raysPerSecond = 0;

	RaytraceSphereflake(size_t width, size_t height) : m_Width(width), m_Height(height)
	{
		m_GBuffer.positions.resize(width * height);
		m_GBuffer.normals.resize(width * height);

		Precomputem_ChildTransforms();
	}

	~RaytraceSphereflake() 
	{
		m_Deinitialize = true;

		for (auto&& i: m_Threads)
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
			m_Threads.push_back(std::make_unique<std::thread>(std::bind(&RaytraceSphereflake::DoImagePartSSE, this)));
		}
	}

	const GBuffer& GetGBuffer()
	{
		return m_GBuffer;
	}

	void UpdateCamera(Camera* camera)
	{
		m_RayOrigin.Set(camera->getPosition());
		m_TopLeft.Set(camera->getTopLeft());
		m_TopRight.Set(camera->getTopRight());
		m_BottomLeft.Set(camera->getBottomLeft());
	}

	private:
	void DoImagePartSSE()
	{
		std::mt19937 mt;
		mt.seed(time(NULL));

		std::uniform_int_distribution<int> widthRand(0, m_Width - 2);
		std::uniform_int_distribution<int> heightRand(0, m_Height - 2);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		for (;;)
		{
			auto x0 = widthRand(mt);
			auto y0 = heightRand(mt);

			size_t xa[4] = { x0, x0 + 1, x0, x0 + 1};
			size_t ya[4] = { y0, y0, y0 + 1, y0 + 1};

			auto x = _mm_set_ps(xa[3], xa[2], xa[1], xa[0]);
			auto y = _mm_set_ps(ya[3], ya[2], ya[1], ya[0]);

			auto width = _mm_set1_ps(m_Width);
			auto height = _mm_set1_ps(m_Height);

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

			Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
			__m128 minT = _mm_set1_ps(std::numeric_limits<float>::max());

			auto result = RenderSphereFlakeSSE(m_RayOrigin, rayDirection, transform, 3.f, 0, minT, position, normal);

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

	void Precomputem_ChildTransforms()
	{
		for (auto i = 0u; i < 6; i++)
		{
			float s = radians((float)i * 60.0f);
			float t = radians(90.0f);
			const auto& coss = cosf(s);
			const auto& sins = sinf(s);
			const auto& cost = cosf(t);
			const auto& sint = sinf(t);

			Eigen::Vector4f displacement(coss * sint, sins * sint, cost, 0.0f);
			displacement.normalize();

			auto transform = createRotation(Eigen::Vector4f(90.0f, (90 + i * 60) % 360, 0.0f, 0.0f));
			transform(0, 3) = displacement[0];
			transform(1, 3) = displacement[1];
			transform(2, 3) = displacement[2];
			m_ChildTransforms.push_back(transform);
		}

		for (auto i = 0u; i < 3; i++)
		{
			float s = radians((float)((30 + i * 120) % 360));
			float t = radians(30.0f);
			const auto& coss = cosf(s);
			const auto& sins = sinf(s);
			const auto& cost = cosf(t);
			const auto& sint = sinf(t);

			Eigen::Vector4f displacement(coss * sint, sins * sint, cost, 0.0f);
			displacement.normalize();

			auto transform = createRotation(Eigen::Vector4f(0.0, 360.0f - i * 60.0f, 270.0f, 0.0f));
			transform(0, 3) = displacement[0];
			transform(1, 3) = displacement[1];
			transform(2, 3) = displacement[2];
			m_ChildTransforms.push_back(transform);
		}
	}

	Eigen::Matrix4f createRotation(Eigen::Vector4f rot)
	{
		float sinx = sinf(rot[0]);
		float siny = sinf(rot[1]);
		float sinz = sinf(rot[2]);

		float cosx = cosf(rot[0]);
		float cosy = cosf(rot[1]);
		float cosz = cosf(rot[2]);

		Eigen::Matrix4f result = Eigen::Matrix4f::Identity();
		result(0, 0) = cosy * cosz;
		result(1, 0) = cosx * sinz + sinx * siny * cosz;
		result(2, 0) = sinx * sinz - cosx * siny * cosz;

		result(0, 1) = -cosy * sinz;
		result(1, 1) = cosx * cosz - sinx * siny * sinz;
		result(2, 1) = sinx * cosz + cosx * siny * sinz;

		result(0, 2) = siny;
		result(1, 2) = -sinx * cosy;
		result(2, 2) = cosx * cosy;
		return result;
	}

	__m128 RenderSphereFlakeSSE
	(
		const Vec3Packet& rayOrigin,
		const Vec3Packet& rayDirection,
		const Eigen::Matrix4f& parentTransform,
		float parentRadius,
		int depth,
		__m128& minT,
		Vec3Packet& position,
		Vec3Packet& normal
	)
	{
		static const __m128 oneThird = _mm_set1_ps(1.0 / 3.0);
		static const __m128 two = _mm_set1_ps(2.0);
		static const __m128 hundred = _mm_set1_ps(100.0f);
		static const __m128 zero = _mm_set1_ps(0.0f);

		if (depth >= MAX_DEPTH)
		{
			return zero;
		}

		float radiusScalar = parentRadius / 3.0;
		__m128 radius = _mm_set1_ps(radiusScalar);
		__m128 radiusSq = _mm_mul_ps(radius, radius);

		__m128 doubleRadiusSq = _mm_mul_ps(radius, two);
		doubleRadiusSq = _mm_mul_ps(doubleRadiusSq, doubleRadiusSq);

		vec3 sphereOrigin(parentTransform(0, 3), parentTransform(1, 3), parentTransform(2, 3));
		Vec3Packet sphereOriginPacket;
		sphereOriginPacket.Set(sphereOrigin);

		__m128 t;
		auto result = RaySphereIntersectionSSE(rayOrigin, rayDirection, sphereOriginPacket, doubleRadiusSq, t);
		if (_mm_movemask_ps(result) == 0)
		{
			return result;
		}

		auto depthResult = _mm_cmplt_ps(_mm_sqrt_ps(_mm_div_ps(t, radius)), hundred);
		if (_mm_movemask_ps(depthResult) == 0)
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
			float translationScale = (4.0 / 3.0) * radiusScalar;
			transform(0, 3) *= translationScale;
			transform(1, 3) *= translationScale;
			transform(2, 3) *= translationScale;
			auto worldTransform = parentTransform * transform;

			RenderSphereFlakeSSE(rayOrigin, rayDirection, worldTransform, radiusScalar, depth + 1, minT, position, normal);
		}

		result = RaySphereIntersectionSSE(rayOrigin, rayDirection, sphereOriginPacket, radiusSq, t);

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

	std::vector<Eigen::Matrix4f> m_ChildTransforms;
	std::vector<std::unique_ptr<std::thread>> m_Threads;

	bool m_Deinitialize = false;

	Vec3Packet m_RayOrigin;
	Vec3Packet m_TopLeft;
	Vec3Packet m_TopRight;
	Vec3Packet m_BottomLeft;

};

#endif
