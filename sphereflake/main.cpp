#pragma warning (push, 0)
#pragma warning (disable: 4530) // disable warnings from code not under our control

#include <iostream>
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

#define RT_W 1920
#define RT_H 1080

#define WND_WIDTH 1920
#define WND_HEIGHT 1080

#include "Filesystem.h"
#include "GLProgram.h"
#include "GLPixelBufferObject.h"
#include "GLTexture2D.h"
#include "GLFramebufferObject.h"
#include "GL.h"

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

Camera camera;
Sphereflake rts(RT_W, RT_H);

GLFWwindow* window;

GLFWwindow* GLInitialize(size_t width, size_t height)
{
	if (!glfwInit())
	{
		return nullptr;
	}

	/*	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);*/

	auto window = glfwCreateWindow(width, height, "Sphereflake", nullptr, nullptr);
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

void SetupGL()
{
	program = std::make_unique<GL::Program>(Filesystem::ReadAllText("post_vertex.glsl"), Filesystem::ReadAllText("post_final.glsl"));
}

std::unique_ptr<GL::Texture2D> positionsTexture;
std::unique_ptr<GL::PixelBufferObject> positionsPbo;

std::unique_ptr<GL::Texture2D> normalsTexture;
std::unique_ptr<GL::PixelBufferObject> normalsPbo;

void CreateGBufferTextures(size_t width, size_t height)
{
	positionsTexture = std::make_unique<GL::Texture2D>(width, height, GL::Texture2DFormat::RGBA_FLOAT, GL::Texture2DFilter::NEAREST, GL::Texture2DWrapMode::CLAMP_TO_EDGE);
	normalsTexture = std::make_unique<GL::Texture2D>(width, height, GL::Texture2DFormat::RGBA_FLOAT, GL::Texture2DFilter::NEAREST, GL::Texture2DWrapMode::CLAMP_TO_EDGE);

	positionsPbo = std::make_unique<GL::PixelBufferObject>();
	normalsPbo = std::make_unique<GL::PixelBufferObject>();
}

int main(int argc, char* argv [])
{
	window = GLInitialize(WND_WIDTH, WND_HEIGHT);

	SetupGL();
	std::unique_ptr<SSAO> ssao = std::make_unique<SSAO>(RT_W, RT_H, 1);

	glEnable(GL_TEXTURE_2D);
	CreateGBufferTextures(RT_W, RT_H);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	double lastTime = glfwGetTime();
	double fpsTimeAccum = 0.0;
	size_t fpsCounter = 0;
	camera.SetPosition(vec3(-5.4098f, -7.2139f, 1.19006f));
	camera.SetPitch(-1.371f);
	camera.SetYaw(0.921999f);
	camera.SetRoll(0.0f);

	double lastXpos = 0.0;
	double lastYpos = 0.0;

	rts.SetView(camera.GetPosition(), camera.GetTopLeft(), camera.GetTopRight(), camera.GetBottomLeft());
	rts.Initialize();

	float ssaoFactor = 1.0f;

	while (!glfwWindowShouldClose(window))
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(window, 1);
		}

		if (glfwGetKey(window, GLFW_KEY_ENTER))
		{
			auto pos = camera.GetPosition();
			auto pitch = camera.GetPitch();
			auto yaw = camera.GetYaw();
			auto roll = camera.GetRoll();
			std::cout << pos.x << " " << pos.y << " " << pos.z << std::endl;
			std::cout << pitch << " " << yaw << " " << roll << std::endl;
		}

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
			ss << rts.maxDepthReached;
			rts.maxDepthReached = 0;
	
			ss << " rays per second: ";
			ss << rts.raysPerSecond / 1000;
			ss << "k";
			ss << " closest sphere: ";
			ss << rts.closestSphereDistance;
			rts.closestSphereDistance = std::numeric_limits<float>::max();
			rts.raysPerSecond = 0;

			glfwSetWindowTitle(window, ss.str().c_str());
			fpsTimeAccum = 0.0;
			fpsCounter = 0;
		}

		float cameraSpeed = 0.2f * (float)dt * min(rts.closestSphereDistance, 6.0f);
		if (glfwGetKey(window, GLFW_KEY_D))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(1, 0, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_A))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(-1, 0, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_S))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(0, 0, 1) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(0, 0, -1) * cameraSpeed);
		}
		
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(0, 1, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_E))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(0, -1, 0) * cameraSpeed);
		}

		double xpos;
		double ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		double deltax = (xpos - lastXpos);
		double deltay = (ypos - lastYpos);
		lastXpos = xpos;
		lastYpos = ypos;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
		{
			camera.SetYaw(camera.GetYaw() + (float)deltay * 0.001f);
			camera.SetPitch(camera.GetPitch() - (float)deltax * 0.001f);
		}

		// render sphereflake
		rts.SetView(camera.GetPosition(), camera.GetTopLeft(), camera.GetTopRight(), camera.GetBottomLeft());

		positionsPbo->BufferData(rts.GetGBuffer().positions);
		positionsTexture->Upload(positionsPbo);

		normalsPbo->BufferData(rts.GetGBuffer().normals);
		normalsTexture->Upload(normalsPbo);

		positionsTexture->Bind(0);
		normalsTexture->Bind(1);
		
		// render SSAO
		ssao->SetSampleRadiusMultiplier(rts.closestSphereDistance);
		ssao->Render();

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ssao->GetSSAOTexture());

		// finalize
		//fbo->SetActiveDraw();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		program->Use();
		program->SetUniform("cameraPosition", camera.GetPosition());
		program->SetUniform("framebufferSize", vec2(WND_WIDTH, WND_HEIGHT));
		
		DRAW_FULLSCREEN_QUAD();
		//fbo->BlitToDefaultFramebuffer(WND_WIDTH, WND_HEIGHT);
		//ssao->GetSSAOTarget()->BlitToDefaultFramebuffer(WND_WIDTH, WND_HEIGHT);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ssao = nullptr;
	program = nullptr;

	glfwTerminate();
	return 0;
}