#pragma warning (push, 0)
#pragma warning (disable: 4530) // disable warnings from code not under our control

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <random>
#include <memory>

#include <GL/glew.h>

#define GLFW_DLL
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

#define WND_WIDTH 1280
#define WND_HEIGHT 720

#include "StringUtil.h"
#include "Filesystem.h"
#include "CommandLine.h"

#include "GLProgram.h"
#include "GLPixelBufferObject.h"
#include "GLTexture2D.h"
#include "GLFramebufferObject.h"

#ifdef __ARCH_NO_AVX
#include <tmmintrin.h>
#include "SIMD_SSE.h"
#else
#include <immintrin.h>
#include "SIMD_AVX.h"
#endif

#include "Camera.h"
#include "Sphereflake.h"
#include "SSAO.h"

using namespace SphereflakeRaytracer;

class SphereflakeRaytracerMain
{

	public:
	SphereflakeRaytracerMain(size_t width, size_t height, bool fullscreen) : m_Width(width), m_Height(height), m_Fullscreen(fullscreen)
	{
		InitializeOpenGL(width, height, fullscreen);
		InitializeGBufferTextures();

		m_Camera = std::make_unique<Camera>(m_Width, m_Height);
		m_Camera->SetPosition(vec3(-5.4098f, -7.2139f, 1.19006f));
		m_Camera->SetPitch(-1.371f);
		m_Camera->SetYaw(0.921999f);
		m_Camera->SetRoll(0.0f);

		m_Sphereflake = std::make_unique<Sphereflake>(m_Width, m_Height);
		m_Sphereflake->Initialize();

		m_FinalPassProgram = std::make_unique<GL::Program>(Filesystem::ReadAllText("post_vertex.glsl"), Filesystem::ReadAllText("post_final.glsl"));
		m_SSAO = std::make_unique<SSAO>(width, height, 1);
	}

	~SphereflakeRaytracerMain()
	{
		m_Camera = nullptr;
		m_Sphereflake = nullptr;
		m_FinalPassProgram = nullptr;
		m_SSAO = nullptr;

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Run()
	{
		DoMainLoop();
	}

	private:
	void InitializeOpenGL(size_t width, size_t height, bool fullscreen)
	{
		if (!glfwInit())
		{
			std::cout << "failed to initialize GLFW" << std::endl;
			return;
		}

		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

		m_Window = glfwCreateWindow((int)width, (int)height, "Sphereflake", fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
		if (m_Window == nullptr)
		{
			std::cout << "failed to create GLFW window" << std::endl;
			glfwTerminate();
			return;
		}

		glfwMakeContextCurrent(m_Window);
		glewExperimental = GL_TRUE;
		glewInit();

		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		
		glfwGetFramebufferSize(m_Window, &m_ViewportWidth, &m_ViewportHeight);
		
		GLint versionMinor, versionMajor;
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);

		std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

		if (versionMajor < 4 && versionMinor < 2)
		{
			std::cout << "Version mismatch, required OpenGL version >= 4.2" << std::endl;
			exit(1);
		}

		glfwSwapInterval(1); // sync @ 60hz
	}

	void InitializeGBufferTextures()
	{
		m_PositionsTexture = std::make_unique<GL::Texture2D>(m_Width, m_Height, GL::Texture2DFormat::RGBA_FLOAT, GL::Texture2DFilter::NEAREST, GL::Texture2DWrapMode::CLAMP_TO_EDGE);
		m_PositionsPbo = std::make_unique<GL::PixelBufferObject>();

		m_NormalsTexture = std::make_unique<GL::Texture2D>(m_Width, m_Height, GL::Texture2DFormat::RGBA_FLOAT, GL::Texture2DFilter::NEAREST, GL::Texture2DWrapMode::CLAMP_TO_EDGE);
		m_NormalsPbo = std::make_unique<GL::PixelBufferObject>();
	}

	void ProcessInput(double dt)
	{
		if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(m_Window, 1);
		}

		float cameraSpeed = 0.2f * (float)dt * min(m_Sphereflake->closestSphereDistance, 6.0f);
		if (glfwGetKey(m_Window, GLFW_KEY_D))
		{
			m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetOrientation() * vec3(1, 0, 0) * cameraSpeed);
		}

		if (glfwGetKey(m_Window, GLFW_KEY_A))
		{
			m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetOrientation() * vec3(-1, 0, 0) * cameraSpeed);
		}

		if (glfwGetKey(m_Window, GLFW_KEY_S))
		{
			m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetOrientation() * vec3(0, 0, 1) * cameraSpeed);
		}

		if (glfwGetKey(m_Window, GLFW_KEY_W))
		{
			m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetOrientation() * vec3(0, 0, -1) * cameraSpeed);
		}

		if (glfwGetKey(m_Window, GLFW_KEY_Q))
		{
			m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetOrientation() * vec3(0, 1, 0) * cameraSpeed);
		}

		if (glfwGetKey(m_Window, GLFW_KEY_E))
		{
			m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetOrientation() * vec3(0, -1, 0) * cameraSpeed);
		}

		double xpos;
		double ypos;
		glfwGetCursorPos(m_Window, &xpos, &ypos);
		float deltax = (float)xpos - m_MouseLastXPos;
		float deltay = (float)ypos - m_MouseLastYPos;
		m_MouseLastXPos = (float)xpos;
		m_MouseLastYPos = (float)ypos;

		if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_2))
		{
			m_Camera->SetYaw(m_Camera->GetYaw() + (float)deltay * 0.001f);
			m_Camera->SetPitch(m_Camera->GetPitch() - (float)deltax * 0.001f);
		}
	}

	void DoMainLoop()
	{
		double lastTime = glfwGetTime();
		double fpsTimeAccum = 0.0;
		size_t fpsCounter = 0;

		while (!glfwWindowShouldClose(m_Window))
		{
			double time = glfwGetTime();
			double dt = time - lastTime;
			lastTime = time;

			fpsCounter++;
			fpsTimeAccum += dt;
			if (fpsTimeAccum > 1.0)
			{
				std::stringstream ss;
				ss << "sphereflake fps: ";
				ss << fpsCounter;
				ss << " depth: ";
				ss << m_Sphereflake->maxDepthReached;
				m_Sphereflake->maxDepthReached = 0;

				ss << " rays per second: ";
				ss << m_Sphereflake->raysPerSecond / 1000;
				ss << "k";
				ss << " closest sphere: ";
				ss << m_Sphereflake->closestSphereDistance;
				m_Sphereflake->closestSphereDistance = std::numeric_limits<float>::max();
				m_Sphereflake->raysPerSecond = 0;

				glfwSetWindowTitle(m_Window, ss.str().c_str());
				fpsTimeAccum = 0.0;
				fpsCounter = 0;
			}

			ProcessInput(dt);
			Render();
		}
	}

	void Render()
	{
		// render sphereflake
		m_Sphereflake->SetView(m_Camera->GetPosition(), m_Camera->GetTopLeft(), m_Camera->GetTopRight(), m_Camera->GetBottomLeft());

		m_PositionsPbo->BufferData(m_Sphereflake->GetGBuffer().positions);
		m_PositionsTexture->Upload(m_PositionsPbo);

		m_NormalsPbo->BufferData(m_Sphereflake->GetGBuffer().normals);
		m_NormalsTexture->Upload(m_NormalsPbo);

		m_PositionsTexture->Bind(0);
		m_NormalsTexture->Bind(1);

		// render SSAO
		m_SSAO->SetSampleRadiusMultiplier(m_Sphereflake->closestSphereDistance);
		m_SSAO->Render();

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_SSAO->GetSSAOTexture());

		// finalize
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		m_FinalPassProgram->Use();
		m_FinalPassProgram->SetUniform("cameraPosition", m_Camera->GetPosition());
		m_FinalPassProgram->SetUniform("framebufferSize", vec2(m_Width, m_Height));

		glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);

		DRAW_FULLSCREEN_QUAD();

		glfwSwapBuffers(m_Window);
		glfwPollEvents();
	}

	size_t m_Width = 0;
	size_t m_Height = 0;
	bool m_Fullscreen = false;

	int m_ViewportWidth = 0;
	int m_ViewportHeight = 0;

	float m_MouseLastXPos = 0.0f;
	float m_MouseLastYPos = 0.0f;

	std::unique_ptr<GL::Program> m_FinalPassProgram = nullptr;

	std::unique_ptr<Camera> m_Camera = nullptr;
	std::unique_ptr<Sphereflake> m_Sphereflake = nullptr;
	std::unique_ptr<SSAO> m_SSAO = nullptr;

	std::unique_ptr<GL::Texture2D> m_PositionsTexture = nullptr;
	std::unique_ptr<GL::PixelBufferObject> m_PositionsPbo = nullptr;

	std::unique_ptr<GL::Texture2D> m_NormalsTexture = nullptr;
	std::unique_ptr<GL::PixelBufferObject> m_NormalsPbo = nullptr;

	GLFWwindow* m_Window = nullptr;

};

int main(int argc, char* argv[])
{
	CommandLine::Instance().ParseCommandLine(argc, argv);

	auto wndWidth = WND_WIDTH;
	auto wndHeight = WND_HEIGHT;

	if (COMMANDLINE_HAS_KEY("width") && COMMANDLINE_HAS_KEY("height"))
	{
		wndWidth = COMMANDLINE_GET_INT_VALUE("width");
		wndHeight = COMMANDLINE_GET_INT_VALUE("height");
	}

	SphereflakeRaytracerMain rt(wndWidth, wndHeight, false);
	rt.Run();
	return 0;
}