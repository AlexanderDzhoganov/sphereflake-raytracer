#ifndef __SPHEREFLAKERAYTRACER_FILESYSTEM_H
#define __SPHEREFLAKERAYTRACER_FILESYSTEM_H

namespace SphereflakeRaytracer
{

	namespace Filesystem
	{

		std::string ReadAllText(const std::string& path)
		{
			return std::string(std::istreambuf_iterator<char>(std::ifstream(path)), std::istreambuf_iterator<char>());
		}

	}

}

#endif
