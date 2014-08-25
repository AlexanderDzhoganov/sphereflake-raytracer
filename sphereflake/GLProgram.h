#ifndef __SPHEREFLAKERAYTRACER_GLPROGRAM_H
#define __SPHEREFLAKERAYTRACER_GLPROGRAM_H

namespace SphereflakeRaytracer
{

	namespace GL
	{

		class Program
		{

			public:
			Program(const std::string& vertexSource, const std::string& fragmentSource);
			~Program();

			bool IsLinked()
			{
				return m_IsLinked;
			}

			void Use()
			{
				glUseProgram(m_Handle);
			}

			void SetUniform(const std::string& name, const std::vector<vec3>& v)
			{
				glUniform3fv(GetUniformLocation(name), (GLsizei)v.size(), (GLfloat*)v.data());
			}

			void SetUniform(const std::string& name, const vec2& v)
			{
				glUniform2fv(GetUniformLocation(name), 1, value_ptr(v));
			}

			void SetUniform(const std::string& name, const vec3& v)
			{
				glUniform3fv(GetUniformLocation(name), 1, value_ptr(v));
			}

			void SetUniform(const std::string& name, const vec4& v)
			{
				glUniform4fv(GetUniformLocation(name), 1, value_ptr(v));
			}

			void SetUniform(const std::string& name, const mat3& m)
			{
				glUniformMatrix3fv(GetUniformLocation(name), 1, false, value_ptr(m));
			}

			void SetUniform(const std::string& name, const mat4& m)
			{
				glUniformMatrix4fv(GetUniformLocation(name), 1, false, value_ptr(m));
			}

			void SetUniform(const std::string& name, int i)
			{
				glUniform1i(GetUniformLocation(name), i);
			}

			void SetUniform(const std::string& name, float f)
			{
				glUniform1f(GetUniformLocation(name), f);
			}

			private:
			void Link();
			GLuint CompileShader(const std::string& source, GLenum type);

			int GetUniformLocation(const std::string& name);

			GLuint m_Handle;
			bool m_IsLinked;
			std::string m_VertexSource;
			std::string m_FragmentSource;

		};

	}

}

#endif
