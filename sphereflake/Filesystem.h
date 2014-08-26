#ifndef __SPHEREFLAKERAYTRACER_FILESYSTEM_H
#define __SPHEREFLAKERAYTRACER_FILESYSTEM_H

namespace SphereflakeRaytracer
{

	namespace Filesystem
	{

		inline bool ReadAllText(const std::string& path, std::string& result)
		{
			auto f = std::ifstream(path);
			if(!f.is_open())
			{
				return false;
			}

			result = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
			
			return true;
		}

	}

}

#endif
