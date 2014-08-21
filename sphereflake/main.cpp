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

#define RT_W 1280
#define RT_H 720

#define WND_WIDTH 1280
#define WND_HEIGHT 720

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

int main(int argc, char* argv [])
{
	auto window = GLInitialize();

	SetupGL();
	std::unique_ptr<SSAO> ssao = std::make_unique<SSAO>(RT_W, RT_H, 1);

	glEnable(GL_TEXTURE_2D);
	CreateGBufferTextures();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	double lastTime = glfwGetTime();
	double fpsTimeAccum = 0.0;
	size_t fpsCounter = 0;
	camera.SetPosition(vec3(0, 0, 4));

	double lastXpos = 0.0;
	double lastYpos = 0.0;

	rts.SetView(camera.GetPosition(), camera.GetTopLeft(), camera.GetTopRight(), camera.GetBottomLeft());
	rts.Initialize();

	float ssaoFactor = 1.0f;

	while (!glfwWindowShouldClose(window))
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

		bool moving = false;

		if (glfwGetKey(window, GLFW_KEY_D))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(1, 0, 0) * cameraSpeed);
			moving = true;
		}

		if (glfwGetKey(window, GLFW_KEY_A))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(-1, 0, 0) * cameraSpeed);
			moving = true;
		}

		if (glfwGetKey(window, GLFW_KEY_S))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, 0, 1) * cameraSpeed);
			moving = true;
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, 0, -1) * cameraSpeed);
			moving = true;
		}
		
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, 1, 0) * cameraSpeed);
			moving = true;
		}

		if (glfwGetKey(window, GLFW_KEY_E))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, -1, 0) * cameraSpeed);
			moving = true;
		}

		if (moving)
		{
			ssaoFactor -= dt;
			if (ssaoFactor <= 0.5f)
			{
				ssaoFactor = 0.5f;
			}
		}
		else
		{
			ssaoFactor += dt;
			if (ssaoFactor >= 1.0f)
			{
				ssaoFactor = 1.0f;
			}
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
		ssao->Render();
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ssao->GetSSAOTexture());

		// finalize
		fbo->SetActiveDraw();

		program->Use();
		program->SetUniform("cameraPosition", camera.GetPosition());
		program->SetUniform("ssaoFactor", ssaoFactor);
		program->SetUniform("framebufferSize", vec2(fbo->GetWidth(), fbo->GetHeight()));

		DRAW_FULLSCREEN_QUAD();
		fbo->BlitToDefaultFramebuffer(WND_WIDTH, WND_HEIGHT);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ssao = nullptr;
	program = nullptr;
	fbo = nullptr;

	glfwTerminate();
	return 0;
}