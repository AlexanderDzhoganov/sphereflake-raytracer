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
	std::vector<vec3> positions;
	std::vector<vec3> normals;
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

mat4 createRotation(vec3 rot)
{
	const float& sinx = sinLookup[rot.x];
	const float& siny = sinLookup[rot.y];
	const float& sinz = sinLookup[rot.z];

	const float& cosx = cosLookup[rot.x];
	const float& cosy = cosLookup[rot.y];
	const float& cosz = cosLookup[rot.z];

	mat4 result = mat4(1.0);
	result[0] = vec4(cosy * cosz, cosx * sinz + sinx * siny * cosz, sinx * sinz - cosx * siny * cosz, 0.0);
	result[1] = vec4(-cosy * sinz, cosx * cosz - sinx * siny * sinz, sinx * cosz + cosx * siny * sinz, 0.0);
	result[2] = vec4(siny, -sinx * cosy, cosx * cosy, 0.0);
	result[3] = vec4(0.0, 0.0, 0.0, 1.0);
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
	vec3 rayOrigin;
	vec3 topLeft;
	vec3 topRight;
	vec3 bottomLeft;

	void DoFrame(Camera* camera)
	{
		rayOrigin = camera->getPositionWithZoom();
		topLeft = camera->getTopLeft();
		topRight = camera->getTopRight();
		bottomLeft = camera->getBottomLeft();
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

					vec3 targetDirection = topLeft + (topRight - topLeft) * uv.x + (bottomLeft - topLeft) * uv.y;
					vec3 rayDirection = normalize(targetDirection - rayOrigin);

					vec3 position;
					vec3 normal;
					DoRay(rayOrigin, rayDirection, position, normal);
					m_GBuffer.positions[x + y * m_Width] = position;
					m_GBuffer.normals[x + y * m_Width] = normal;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			if (m_Deinitialize)
			{
				return;
			}
		}
	}

	inline bool RaySphereIntersection(const vec3& rayOrigin, const vec3& rayDirection, const vec3& sphereOrigin, float sphereRadius, float& t)
	{
		vec3 sphereTOrigin = sphereOrigin - rayOrigin;
		t = dot(rayDirection, sphereTOrigin);
		vec3 closestPoint = rayDirection * t;
		float dist = length2(closestPoint - sphereTOrigin);
		return dist <= sphereRadius * sphereRadius;
	}

	private:
	inline bool DoRay(const vec3& origin, const vec3& direction, vec3& position, vec3& normal)
	{
		mat4 transform(1.0);
		float minT = 1e32;
		return RenderSphereFlake(origin, direction, transform, 0, 3.0f, position, normal, minT);
	}
	
	bool RenderSphereFlake(const vec3& rayOrigin, const vec3& rayDir, const mat4& parentTransform, int depth, float parentRadius, vec3& position, vec3& normal, float& minT)
	{
		auto radius = (1.0 / 3.0) * parentRadius;
		vec3 sphereOrigin(parentTransform[3]);

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
				vec3 sphereTOrigin = sphereOrigin - rayOrigin;
				intersectsMain = true;
				position = rayOrigin + rayDir * t;
				normal = normalize(rayDir * t - sphereOrigin);
				minT = -t;
				spheresDrawn++;
			}
		}

		if (depth >= MAX_DEPTH)
		{
			return intersectsMain;
		}

		for (auto i = 0; i < 6; i++)
		{
			int s = i * 60;
			int t = 90;
			const auto& coss = cosLookup[s];
			const auto& sins = sinLookup[s];
			const auto& cost = cosLookup[t];
			const auto& sint = sinLookup[t];

			vec3 displacement(coss * sint, sins * sint, cost);
			displacement *= radius + (1.0 / 3.0) * radius;

			auto transform = createRotation(vec3(90, (90 + i * 60) % 360, 0));
			transform[3] = vec4(displacement, 1.0);
			
			if (RenderSphereFlake(rayOrigin, rayDir, parentTransform * transform, depth + 1, radius, position, normal, minT))
			{
				return false;
			}
		}
		
		for (auto i = 0; i < 3; i++)
		{
			int s = (30 + i * 120) % 360;
			int t = 30;
			const auto& coss = cosLookup[s];
			const auto& sins = sinLookup[s];
			const auto& cost = cosLookup[t];
			const auto& sint = sinLookup[t];
		
			vec3 displacement(coss * sint, sins * sint, cost);
			displacement = normalize(displacement);
			displacement *= radius + (1.0 / 3.0) * radius;
		
			auto transform = createRotation(vec3(0.0, 360.0f - i * 60, 270.0f));
			transform[3] = vec4(displacement, 1.0);
		
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
