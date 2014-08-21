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

	std::unique_ptr<GL::Texture2D> positionsTexture;
	std::unique_ptr<GL::PixelBufferObject> positionsPbo;

	std::unique_ptr<GL::Texture2D> normalsTexture;
	std::unique_ptr<GL::PixelBufferObject> normalsPbo;

	void CreateGBufferTextures()
	{
		positionsTexture = std::make_unique<GL::Texture2D>(RT_W, RT_H, GL::Texture2DFormat::RGBA_FLOAT, GL::Texture2DFilter::NEAREST, GL::Texture2DWrapMode::CLAMP_TO_EDGE);
		normalsTexture = std::make_unique<GL::Texture2D>(RT_W, RT_H, GL::Texture2DFormat::RGBA_FLOAT, GL::Texture2DFilter::NEAREST, GL::Texture2DWrapMode::CLAMP_TO_EDGE);

		positionsPbo = std::make_unique<GL::PixelBufferObject>();
		normalsPbo = std::make_unique<GL::PixelBufferObject>();
	}

}

#endif
