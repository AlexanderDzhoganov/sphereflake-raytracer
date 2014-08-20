#ifndef __GL_H
#define __GL_H

namespace SphereflakeRaytracer
{

	GLFWwindow* GLInitialize()
	{
		if (!glfwInit())
		{
			return nullptr;
		}

		auto window = glfwCreateWindow(WND_WIDTH, WND_HEIGHT, "Sphereflake", NULL, NULL);
		if (!window)
		{
			glfwTerminate();
			return nullptr;
		}

		glfwMakeContextCurrent(window);
		glewExperimental = GL_TRUE;
		glewInit();

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

		glfwSwapInterval(1); // vsync
		return window;
	}

	GLuint program;

	GLuint CreateShader(const std::string& source, GLenum shaderType)
	{
		auto shader = glCreateShader(shaderType);
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

			std::cout << "Shader compile error" << std::endl;
			std::cout << log << std::endl;

			glDeleteShader(shader);
			return 0;
		}

		return shader;
	}

	void LoadProgram()
	{
		std::ifstream ifsVertex("vertex.glsl");
		std::string vertSource((std::istreambuf_iterator<char>(ifsVertex)), std::istreambuf_iterator<char>());

		std::ifstream ifsFrag("ssao.glsl");
		std::string fragSource((std::istreambuf_iterator<char>(ifsFrag)), std::istreambuf_iterator<char>());

		program = glCreateProgram();
		auto vertShader = CreateShader(vertSource, GL_VERTEX_SHADER);
		auto fragShader = CreateShader(fragSource, GL_FRAGMENT_SHADER);

		glAttachShader(program, vertShader);
		glAttachShader(program, fragShader);
		glLinkProgram(program);

		GLint linkStatus = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE)
		{
			GLint logLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

			std::string log;
			log.resize(logLength);

			glGetProgramInfoLog(program, (GLsizei)log.length(), &logLength, (GLchar*)log.c_str());

			std::cout << "Program link error" << std::endl;
			std::cout << log << std::endl;

			glDeleteProgram(program);
		}

		glUseProgram(program);
	}

	int GetUniformLocation(const std::string& name)
	{
		auto loc = glGetUniformLocation(program, name.c_str());
		if (loc == -1)
		{
			//		std::cout << "uniform not found " << name << std::endl;
		}

		return loc;
	}

	void SetUniformVec3Array(const std::string& name, const std::vector<vec3>& v)
	{
		auto loc = GetUniformLocation(name);
		if (loc == -1) return;

		glUniform3fv(loc, (GLsizei)v.size(), (GLfloat*)v.data());
	}

	void SetUniformVec3(const std::string& name, const vec3& v)
	{
		auto loc = GetUniformLocation(name);
		if (loc == -1) return;

		glUniform3fv(loc, 1, value_ptr(v));
	}

	void SetUniformMat3(const std::string& name, const mat3& m)
	{
		auto loc = GetUniformLocation(name);
		if (loc == -1) return;

		glUniformMatrix3fv(loc, 1, false, value_ptr(m));
	}

	void SetUniformMat4(const std::string& name, const mat4& m)
	{
		auto loc = GetUniformLocation(name);
		if (loc == -1) return;

		glUniformMatrix4fv(loc, 1, false, value_ptr(m));
	}

	void SetUniformFloat(const std::string& name, float f)
	{
		auto loc = glGetUniformLocation(program, name.c_str());
		if (loc == -1)
		{
			std::cout << "uniform not found " << name << std::endl;
			return;
		}

		glUniform1f(loc, f);
	}

	GLuint vertBuffer;
	GLuint indexBuffer;

	void CreateBuffers()
	{
		float vertices[] =
		{
			-1.0f, -1.0f, 0.0,
			1.0f, -1.0f, 0.0,
			1.0f, 1.0f, 0.0,
			-1.0f, 1.0f, 0.0
		};

		glGenBuffers(1, &vertBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(0);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}

	void DrawFullscreenQuad()
	{
		glDrawArrays(GL_QUADS, 0, 4);
	}

	GLuint GBufferPositionsTexture;
	GLuint GBufferPositionsPBO;

	GLuint GBufferNormalsTexture;
	GLuint GBufferNormalsPBO;

	void CreateGBufferTextures()
	{
		glGenTextures(1, &GBufferPositionsTexture);
		glBindTexture(GL_TEXTURE_2D, GBufferPositionsTexture);

		glGenBuffers(1, &GBufferPositionsPBO);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GBufferPositionsPBO);

		glGenTextures(1, &GBufferNormalsTexture);
		glBindTexture(GL_TEXTURE_2D, GBufferNormalsTexture);

		glGenBuffers(1, &GBufferNormalsPBO);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GBufferNormalsPBO);
	}

}

#endif
