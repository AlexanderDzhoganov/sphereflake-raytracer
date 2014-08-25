#ifndef __SPHEREFLAKERAYTRACER_STRINGUTIL_H
#define __SPHEREFLAKERAYTRACER_STRINGUTIL_H

namespace SphereflakeRaytracer
{

	inline std::vector<std::string> split(const std::string& s, char delim)
	{
		std::vector<std::string> elems;
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim))
		{
			elems.push_back(item);
		}

		return elems;
	}

	inline size_t splitNewlines(const std::string& s, std::vector<std::string>& lines)
	{
		char* ptr = (char*)s.c_str();
		char* start = ptr;

		for (auto i = 0u; i < s.size(); i++)
		{
			if (ptr[i] == '\n')
			{
				ptr[i] = '\0';
				auto length = (ptr + i) - start;
				lines.emplace_back(start, length);
				start = ptr + i + 1;
			}
		}
		lines.emplace_back(start);
		return lines.size();
	}

}

#endif
