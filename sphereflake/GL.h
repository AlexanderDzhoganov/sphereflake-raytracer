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

		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

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

		glfwSwapInterval(1); // sync @ 60hz
		return window;
	}

	std::unique_ptr<GL::Program> program = nullptr;
	std::unique_ptr<GL::FramebufferObject> fbo = nullptr;

	void SetupGL()
	{
		program = std::make_unique<GL::Program>(Filesystem::ReadAllText("post_vertex.glsl"), Filesystem::ReadAllText("post_final.glsl"));
		fbo = std::make_unique<GL::FramebufferObject>(RT_W, RT_H);
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
