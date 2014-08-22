#ifndef __SPHEREFLAKERAYTRACER_FILESYSTEM_H
#define __SPHEREFLAKERAYTRACER_FILESYSTEM_H

namespace SphereflakeRaytracer
{

	namespace Filesystem
	{

		inline std::string ReadAllText(const std::string& path)
		{
			auto f = std::ifstream(path);
			return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
		}

	}

}

#endif
