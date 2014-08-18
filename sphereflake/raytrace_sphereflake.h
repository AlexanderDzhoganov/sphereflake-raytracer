#ifndef __RAYTRACE_SPHEREFLAKE_H
#define __RAYTRACE_SPHEREFLAKE_H

struct Color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct GBuffer
{
	std::vector<Eigen::Vector3f> positions;
	std::vector<Eigen::Vector3f> normals;
};

std::vector<float> sinLookup;
std::vector<float> cosLookup;

#define MAX_DEPTH 16

void PrecomputeTrig()
{
	for (auto i = 0u; i < 361; i++)
	{
		sinLookup.push_back(sin(radians((float)i)));
		cosLookup.push_back(cos(radians((float)i)));
	}
}

Eigen::Matrix4f createRotation(Eigen::Vector3f rot)
{
	const float& sinx = sinLookup[rot[0]];
	const float& siny = sinLookup[rot[1]];
	const float& sinz = sinLookup[rot[2]];

	const float& cosx = cosLookup[rot[0]];
	const float& cosy = cosLookup[rot[1]];
	const float& cosz = cosLookup[rot[2]];

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

size_t spheresDrawn = 0;

class RaytraceSphereflake
{

	public:
	RaytraceSphereflake(size_t width, size_t height) : m_Width(width), m_Height(height)
	{
		m_Bitmap.resize(4 * width * height);

		m_GBuffer.positions.resize(width * height);
		m_GBuffer.normals.resize(width * height);

		PrecomputeTrig();
		PrecomputeChildTransforms();

		auto threadCount = std::thread::hardware_concurrency();

		size_t splitW = m_Width;
		size_t splitH = m_Height / threadCount;

		for (auto i = 0u; i < threadCount; i++)
		{
			m_Threads.push_back(std::make_unique<std::thread>(std::bind(&RaytraceSphereflake::DoImagePart, this, 0, splitH * i, m_Width, splitH)));
		}
	}

	~RaytraceSphereflake() 
	{
		m_Deinitialize = true;

		for (auto&& i: m_Threads)
		{
			i->join();
		}
	}

	const std::vector<Color>& GetBitmap()
	{
		return m_Bitmap;
	}

	const GBuffer& GetGBuffer()
	{
		return m_GBuffer;
	}
	
	bool m_Deinitialize = false;
	std::mutex m_Mutex;
	volatile int finishedThreads = 0;
	Eigen::Vector3f rayOrigin;
	Eigen::Vector3f topLeft;
	Eigen::Vector3f topRight;
	Eigen::Vector3f bottomLeft;

	void DoFrame(Camera* camera)
	{
		auto cameraPos = camera->getPositionWithZoom();
		auto cameraTopLeft = camera->getTopLeft();
		auto cameraTopRight = camera->getTopRight();
		auto cameraBottomLeft = camera->getBottomLeft();

		rayOrigin = Eigen::Vector3f(cameraPos.x, cameraPos.y, cameraPos.z);
		topLeft = Eigen::Vector3f(cameraTopLeft.x, cameraTopLeft.y, cameraTopLeft.z);
		topRight = Eigen::Vector3f(cameraTopRight.x, cameraTopRight.y, cameraTopRight.z);
		bottomLeft = Eigen::Vector3f(cameraBottomLeft.x, cameraBottomLeft.y, cameraBottomLeft.z);
	}

	void DoImagePart(size_t _x, size_t _y, size_t width, size_t height)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		for (;;)
		{
			for (auto x = _x; x < _x + width; x++)
			{
				for (auto y = _y; y < _y + height; y++)
				{
					vec2 uv = vec2((float) x / (float) m_Width, (float) y / (float) m_Height);

					Eigen::Vector3f targetDirection = topLeft + (topRight - topLeft) * uv[0] + (bottomLeft - topLeft) * uv[1];
					Eigen::Vector3f rayDirection = (targetDirection - rayOrigin).normalized();

					Eigen::Vector3f position = Eigen::Vector3f(0, 0, 0);
					Eigen::Vector3f normal = Eigen::Vector3f(0, 0, 0);

					Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
					float minT = 1e32;

					RenderSphereFlake(rayOrigin, rayDirection, transform, 0, 3.0f, position, normal, minT);
					
					m_GBuffer.positions[x + y * m_Width] = position;
					m_GBuffer.normals[x + y * m_Width] = normal;
				}
			}

			if (m_Deinitialize)
			{
				return;
			}
		}
	}

	inline bool RaySphereIntersection(const Eigen::Vector3f& rayOrigin, const Eigen::Vector3f& rayDirection, const Eigen::Vector3f& sphereOrigin, float sphereRadius, float& t)
	{
		Eigen::Vector3f sphereTOrigin = sphereOrigin - rayOrigin;
		t = rayDirection.dot(sphereTOrigin);
		Eigen::Vector3f closestPoint = rayDirection * t;
		Eigen::Vector3f diff = closestPoint - sphereTOrigin;
		float dist = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
		return dist <= sphereRadius * sphereRadius;
	}

	private:
	std::vector<Eigen::Matrix4f> childTransforms;

	void PrecomputeChildTransforms()
	{
		for (auto i = 0u; i < 6; i++)
		{
			int s = i * 60;
			int t = 90;
			const auto& coss = cosLookup[s];
			const auto& sins = sinLookup[s];
			const auto& cost = cosLookup[t];
			const auto& sint = sinLookup[t];

			Eigen::Vector3f displacement(coss * sint, sins * sint, cost);
			displacement.normalize();

			auto transform = createRotation(Eigen::Vector3f(90, (90 + i * 60) % 360, 0));
			transform(0, 3) = displacement[0];
			transform(1, 3) = displacement[1];
			transform(2, 3) = displacement[2];
			childTransforms.push_back(transform);
		}

		for (auto i = 0u; i < 3; i++)
		{
			int s = (30 + i * 120) % 360;
			int t = 30;
			const auto& coss = cosLookup[s];
			const auto& sins = sinLookup[s];
			const auto& cost = cosLookup[t];
			const auto& sint = sinLookup[t];

			Eigen::Vector3f displacement(coss * sint, sins * sint, cost);
			displacement.normalize();

			auto transform = createRotation(Eigen::Vector3f(0.0, 360.0f - i * 60, 270.0f));
			transform(0, 3) = displacement[0];
			transform(1, 3) = displacement[1];
			transform(2, 3) = displacement[2];
			childTransforms.push_back(transform);
		}
	}
	
	bool RenderSphereFlake(const Eigen::Vector3f& rayOrigin, const Eigen::Vector3f& rayDir, const Eigen::Matrix4f& parentTransform, int depth, float parentRadius, Eigen::Vector3f& position, Eigen::Vector3f& normal, float& minT)
	{
		auto radius = (1.0 / 3.0) * parentRadius;
		Eigen::Vector3f sphereOrigin(parentTransform(0, 3), parentTransform(1, 3), parentTransform(2, 3));

		float t;
		if (!RaySphereIntersection(rayOrigin, rayDir, sphereOrigin, radius * 2.0f, t))
		{
			return false;
		}

		bool intersectsMain = false;
		if (RaySphereIntersection(rayOrigin, rayDir, sphereOrigin, radius, t))
		{
			if (-t < minT)
			{
				intersectsMain = true;
				spheresDrawn++;

				Eigen::Vector3f sphereTOrigin = sphereOrigin - rayOrigin;
				position = rayOrigin + rayDir * t;
				normal = (rayDir * t - sphereOrigin).normalized();
				minT = -t;
			}
		}

		if (depth >= MAX_DEPTH)
		{
			return intersectsMain;
		}

		for (auto i = 0; i < 6; i++)
		{
			auto transform = childTransforms[i];
			float translationScale = radius + (1.0 / 3.0) * radius;
			transform(0, 3) *= translationScale;
			transform(1, 3) *= translationScale;
			transform(2, 3) *= translationScale;

			if (RenderSphereFlake(rayOrigin, rayDir, parentTransform * transform, depth + 1, radius, position, normal, minT))
			{
				return false;
			}
		}
		
		for (auto i = 0; i < 3; i++)
		{
			auto transform = childTransforms[6 + i];
			float translationScale = radius + (1.0 / 3.0) * radius;
			transform(0, 3) *= translationScale;
			transform(1, 3) *= translationScale;
			transform(2, 3) *= translationScale;
		
			if (RenderSphereFlake(rayOrigin, rayDir, parentTransform * transform, depth + 1, radius, position, normal, minT))
			{
				return false;
			}
		}

		return false;
	}

	size_t m_Width;
	size_t m_Height;
	std::vector<Color> m_Bitmap;
	GBuffer m_GBuffer;

	std::vector<std::unique_ptr<std::thread>> m_Threads;

};

#endif
