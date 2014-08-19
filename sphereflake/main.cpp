#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <thread>
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

using namespace glm;

#define EIGEN_NO_MALLOC
#include <Eigen/Dense>

#define RT_W 1280
#define RT_H 720

#define WND_WIDTH 1280
#define WND_HEIGHT 720

#include "SIMD.h"
#include "camera.h"
#include "raytrace_sphereflake.h"
#include "GL.h"
#include "SSAO.h"

Camera camera;
RaytraceSphereflake rts(RT_W, RT_H);

void UploadGBufferTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GBufferPositionsTexture);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GBufferPositionsPBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4 * sizeof(float), rts.GetGBuffer().positions.data(), GL_STREAM_DRAW);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RT_W, RT_H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GBufferNormalsTexture);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GBufferNormalsPBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4 * sizeof(float), rts.GetGBuffer().normals.data(), GL_STREAM_DRAW);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RT_W, RT_H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

int main(int argc, char* argv [])
{
	if (!glfwInit())
	{
		return -1;
	}

	auto window = glfwCreateWindow(WND_WIDTH, WND_HEIGHT, "Sphereflake", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

	glfwSwapInterval(1);

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
	camera.setPosition(vec3(0, 0, 4));

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

		float cameraSpeed = 0.7f * dt;

		if (glfwGetKey(window, GLFW_KEY_D))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() * vec3(1, 0, 0) * cameraSpeed);
		}
		
		if (glfwGetKey(window, GLFW_KEY_A))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(-1, 0, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_S))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, 1) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, -1) * cameraSpeed);
		}
		
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 1, 0) * cameraSpeed);
		}

		if (glfwGetKey(window, GLFW_KEY_E))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, -1, 0) * cameraSpeed);
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
			camera.setYaw(camera.getYaw() + deltay * 0.001);
			camera.setPitch(camera.getPitch() - deltax * 0.001);
		}

		//SetUniformFloat("time", glfwGetTime());
		SetUniformVec3("cameraPosition", camera.getPosition());
		SetUniformMat4("viewProjection", camera.getViewProjectionMatrix());
		SetUniformMat4("view", camera.getViewMatrix());
		SetUniformMat3("invTranspView3x3", inverse(transpose(mat3(camera.getViewMatrix()))));
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