#pragma warning (push, 0)
#pragma warning (disable: 4530) // disable warnings from code not under our control

#include <string>
#include <vector>
#include <iostream>

#define GL_GLEXT_PROTOTYPES
#include "glcorearb.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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

#pragma warning (pop)

#include "GLProgram.h"

namespace SphereflakeRaytracer
{

	namespace GL
	{

		Program::Program(const std::string& vertexSource, const std::string& fragmentSource) :
			m_VertexSource(vertexSource),
			m_FragmentSource(fragmentSource),
			m_IsLinked(false)
		{
			Link();
		}

		Program::~Program()
		{
			glDeleteProgram(m_Handle);
		}

		void Program::Link()
		{
			m_Handle = glCreateProgram();

			auto vertexShader = CompileShader(m_VertexSource, GL_VERTEX_SHADER);
			auto fragmentShader = CompileShader(m_FragmentSource, GL_FRAGMENT_SHADER);

			glAttachShader(m_Handle, vertexShader);
			glAttachShader(m_Handle, fragmentShader);
			glLinkProgram(m_Handle);

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			GLint linkStatus = 0;
			glGetProgramiv(m_Handle, GL_LINK_STATUS, &linkStatus);
			if (linkStatus == GL_FALSE)
			{
				GLint logLength = 0;
				glGetProgramiv(m_Handle, GL_INFO_LOG_LENGTH, &logLength);

				std::string log;
				log.resize(logLength);

				glGetProgramInfoLog(m_Handle, (GLsizei)log.length(), &logLength, (GLchar*)log.c_str());

				std::cout << "Program link error:" << std::endl;
				std::cout << log << std::endl;

				glDeleteProgram(m_Handle);
				return;
			}

			m_IsLinked = true;
		}

		GLuint Program::CompileShader(const std::string& source, GLenum type)
		{
			auto shader = glCreateShader(type);
			const char* src[1] = { source.c_str() };

			glShaderSource(shader, 1, src, nullptr);
			glCompileShader(shader);

			GLint compileStatus = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
			if (compileStatus == GL_FALSE)
			{
				GLint logLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

				std::string log;
				log.resize(logLength);

				glGetShaderInfoLog(shader, (GLsizei)log.length(), &logLength, (char*)log.c_str());

				std::cout << "Shader compile error:" << std::endl;
				std::cout << log << std::endl;

				glDeleteShader(shader);
				return 0;
			}

			return shader;
		}

		int Program::GetUniformLocation(const std::string& name)
		{
			return glGetUniformLocation(m_Handle, name.c_str());
		}

	}

}