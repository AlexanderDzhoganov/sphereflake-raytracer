#ifndef __SPHEREFLAKERAYTRACER_COMMANDLINE_H
#define __SPHEREFLAKERAYTRACER_COMMANDLINE_H

#define COMMANDLINE_GET_KEY(KEY, EXPECTED_VAL) (CommandLine::Instance().GetValue(KEY) == EXPECTED_VAL)
#define COMMANDLINE_HAS_KEY(KEY) (CommandLine::Instance().HasKey(KEY))
#define COMMANDLINE_GET_FLOAT_VALUE(KEY) (stof(CommandLine::Instance().GetValue(KEY)))
#define COMMANDLINE_GET_INT_VALUE(KEY) (stoi(CommandLine::Instance().GetValue(KEY)))

#define LOG_VERBOSE(s, ...) ((CONFIG_KEY("log", "verbose")) ? Log(__FUNCTION__, s, __VA_ARGS__) : 0)

namespace SphereflakeRaytracer
{

	class CommandLine
	{

		public:
		static CommandLine& Instance()
		{
			static CommandLine instance;
			return instance;
		}

		bool ParseCommandLine(int argc, char** argv)
		{
			std::vector<std::string> keyValuePairs;
			for (int i = 1; i < argc; i++)
			{
				keyValuePairs.emplace_back(argv[i]);
			}

			for (auto& keyValue : keyValuePairs)
			{
				if (keyValue.find('=') == -1)
				{
					m_CommandLine[keyValue.substr(2, keyValue.size() - 2)] = "true";
					continue;
				}

				auto splitKeyValue = split(keyValue, '=');
				auto key = splitKeyValue[0].substr(2, splitKeyValue[0].size() - 2);
				auto value = splitKeyValue[1];

				m_CommandLine[key] = value;
			}

			for (auto& pair : m_CommandLine)
			{
				std::cout << pair.first << " = " << pair.second << std::endl;
			}

			return true;
		}

		const std::string& GetValue(const std::string& key)
		{
			return m_CommandLine[key];
		}

		bool HasKey(const std::string& key)
		{
			return m_CommandLine.find(key) != m_CommandLine.end();
		}

		const std::string& GetInputFilename()
		{
			return m_InputFilename;
		}

		private:
		std::unordered_map<std::string, std::string> m_CommandLine;
		std::string m_InputFilename;

	};

}

#endif