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
#include <mmintrin.h>

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

#include "GL.h"
#include "SIMD.h"
#include "Camera.h"
#include "Sphereflake.h"
#include "SSAO.h"

using namespace SphereflakeRaytracer;

Camera camera;
Sphereflake rts(RT_W, RT_H);

void UploadGBufferTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GBufferPositionsTexture);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GBufferPositionsPBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4 * sizeof(float), rts.GetGBuffer().positions.data(), GL_STREAM_DRAW);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RT_W, RT_H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GBufferNormalsTexture);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GBufferNormalsPBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4 * sizeof(float), rts.GetGBuffer().normals.data(), GL_STREAM_DRAW);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RT_W, RT_H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

int main(int argc, char* argv [])
{
	auto window = GLInitialize();

	LoadProgram();
	SSAO ssao;

	glEnable(GL_TEXTURE_2D);
	CreateBuffers();
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

	rts.UpdateCamera(&camera);
	rts.Initialize();

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
			rts.raysPerSecond = 0;

			glfwSetWindowTitle(window, ss.str().c_str());
			fpsTimeAccum = 0.0;
			fpsCounter = 0;
		}

		float cameraSpeed = 0.7f * (float)dt;

		if (glfwGetKey(window, GLFW_KEY_D))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() * vec3(1, 0, 0) * cameraSpeed);
		}
		
		if (glfwGetKey(window, GLFW_KEY_A))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(-1, 0, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_S))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, 0, 1) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, 0, -1) * cameraSpeed);
		}
		
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, 1, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_E))
		{
			camera.SetPosition(camera.GetPosition() + camera.GetOrientation() *vec3(0, -1, 0) * cameraSpeed);
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

		//SetUniformFloat("time", glfwGetTime());
		SetUniformVec3("cameraPosition", camera.GetPosition());
		SetUniformMat4("viewProjection", camera.GetViewProjectionMatrix());
		SetUniformMat4("view", camera.GetViewMatrix());
		SetUniformMat3("invTranspView3x3", inverse(transpose(mat3(camera.GetViewMatrix()))));
		//SetUniformFloat("fbWidth", width);
		//SetUniformFloat("fbHeight", height);

		rts.UpdateCamera(&camera);

		UploadGBufferTextures();
		ssao.BindNoiseTexture();
		DrawFullscreenQuad();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}