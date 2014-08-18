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
	std::vector<Eigen::Vector4f> positions;
	std::vector<Eigen::Vector4f> normals;
};

#define MAX_DEPTH 1

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

size_t spheresDrawn = 0;

class RaytraceSphereflake
{

	public:
	RaytraceSphereflake(size_t width, size_t height) : m_Width(width), m_Height(height)
	{
		m_Bitmap.resize(4 * width * height);

		m_GBuffer.positions.resize(width * height);
		m_GBuffer.normals.resize(width * height);

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
	Eigen::Vector4f rayOrigin;
	Eigen::Vector4f topLeft;
	Eigen::Vector4f topRight;
	Eigen::Vector4f bottomLeft;

	void DoFrame(Camera* camera)
	{
		auto cameraPos = camera->getPositionWithZoom();
		auto cameraTopLeft = camera->getTopLeft();
		auto cameraTopRight = camera->getTopRight();
		auto cameraBottomLeft = camera->getBottomLeft();

		rayOrigin = Eigen::Vector4f(cameraPos.x, cameraPos.y, cameraPos.z, 0.0f);
		topLeft = Eigen::Vector4f(cameraTopLeft.x, cameraTopLeft.y, cameraTopLeft.z, 0.0f);
		topRight = Eigen::Vector4f(cameraTopRight.x, cameraTopRight.y, cameraTopRight.z, 0.0f);
		bottomLeft = Eigen::Vector4f(cameraBottomLeft.x, cameraBottomLeft.y, cameraBottomLeft.z, 0.0f);
	}

	void DoImagePart(int _x, int _y, int width, int height)
	{
		std::mt19937 mt;
		mt.seed(time(NULL));

		std::uniform_int_distribution<int> widthRand(0, m_Width - 1);
		std::uniform_int_distribution<int> heightRand(0, m_Height - 1);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		for (;;)
		{
			auto x = widthRand(mt);
			auto y = heightRand(mt);

			vec2 uv = vec2((float) x / (float) m_Width, (float) y / (float) m_Height);

			Eigen::Vector4f targetDirection = topLeft + (topRight - topLeft) * uv[0] + (bottomLeft - topLeft) * uv[1];
			Eigen::Vector4f rayDirection = (targetDirection - rayOrigin).normalized();

			Eigen::Vector4f position = Eigen::Vector4f(0, 0, 0, 0);
			Eigen::Vector4f normal = Eigen::Vector4f(0, 0, 0, 0);

			Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
			float minT = std::numeric_limits<float>::max();

			RenderSphereFlake(rayOrigin, rayDirection, transform, 0, 3.0f, position, normal, minT);
					
			m_GBuffer.positions[x + y * m_Width] = position;
			m_GBuffer.normals[x + y * m_Width] = normal;

			raysPerSecond++;

			if (m_Deinitialize)
			{
				return;
			}
		}
	}

	inline bool RaySphereIntersection(const Eigen::Vector4f& rayOrigin, const Eigen::Vector4f& rayDirection, const Eigen::Vector4f& sphereOrigin, float sphereRadius, float& tOut)
	{
		Eigen::Vector4f sphereTOrigin = sphereOrigin - rayOrigin;
		float t = rayDirection.dot(sphereTOrigin);
		if (t < 0.0)
		{
			return false;
		}

		Eigen::Vector4f closestPoint = rayDirection * t;
		Eigen::Vector4f diff = closestPoint - sphereTOrigin;
		float dist = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
		if (dist <= sphereRadius * sphereRadius)
		{
			tOut = -t;
			return true;
		}

		return false;
	}

	int maxDepthReached = 0;
	long long raysPerSecond = 0;

	private:
	std::vector<Eigen::Matrix4f> childTransforms;

	void PrecomputeChildTransforms()
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
			childTransforms.push_back(transform);
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
			childTransforms.push_back(transform);
		}
	}
	
	bool RenderSphereFlake(const Eigen::Vector4f& rayOrigin, const Eigen::Vector4f& rayDir, const Eigen::Matrix4f& parentTransform, int depth, float parentRadius, Eigen::Vector4f& position, Eigen::Vector4f& normal, float& minT)
	{
		auto radius = (1.0 / 3.0) * parentRadius;

		Eigen::Vector4f sphereOrigin(parentTransform(0, 3), parentTransform(1, 3), parentTransform(2, 3), 0.0f);

		if (depth > maxDepthReached)
		{
			maxDepthReached = depth;
		}

		float t = 0.0f;
		if (!RaySphereIntersection(rayOrigin, rayDir, sphereOrigin, radius * 2.0f, t))
		{
			return false;
		}
		else if (t / radius > 1000)
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

				Eigen::Vector4f sphereTOrigin = sphereOrigin - rayOrigin;
				position = rayOrigin + rayDir * t;
				normal = (rayDir * t - sphereOrigin).normalized();
				minT = -t;
			}
		}

		if (depth >= MAX_DEPTH)
		{
			return intersectsMain;
		}

		for (auto i = 0; i < 9; i++)
		{
			auto transform = childTransforms[i];
			float translationScale = (4.0 / 3.0) * radius;
			transform(0, 3) *= translationScale;
			transform(1, 3) *= translationScale;
			transform(2, 3) *= translationScale;
			auto worldTransform = parentTransform * transform;

			if (RenderSphereFlake(rayOrigin, rayDir, worldTransform, depth + 1, radius, position, normal, minT))
			{
				continue;
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
