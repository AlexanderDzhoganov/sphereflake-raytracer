#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <random>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>

#include <GL/glew.h>

#define GLFW_DLL
#include <GLFW/glfw3.h>

#define GLM_FORCE_AVX
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include <gtx/simd_mat4.hpp>
#include <gtx/simd_vec4.hpp>

using namespace glm;

#define EIGEN_NO_MALLOC
#include <Eigen/Dense>

#include "matmult.h"
#include "camera.h"
#include "raytrace_sphereflake.h"

GLuint program;

#define RT_W 1280
#define RT_H 720

#define WND_WIDTH 1280
#define WND_HEIGHT 720

Camera camera;

RaytraceSphereflake rts(RT_W, RT_H);

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

		glGetShaderInfoLog(shader, log.length(), &logLength, (char*) log.c_str());

		std::cout << "Shader compile error" << std::endl;
		std::cout << log << std::endl;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

void ReloadShader()
{
	std::ifstream ifsVertex("vertex.glsl");
	std::string vertSource((std::istreambuf_iterator<char>(ifsVertex)), std::istreambuf_iterator<char>());

	std::ifstream ifsFrag("sphereflake.glsl");
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

		glGetProgramInfoLog(program, log.length(), &logLength, (GLchar*) log.c_str());

		std::cout << "Program link error" << std::endl;
		std::cout << log << std::endl;

		glDeleteProgram(program);
	}

	glUseProgram(program);
}

void SetUniformVec3(const std::string& name, const vec3& v)
{
	auto loc = glGetUniformLocation(program, name.c_str());
	if (loc == -1)
	{
		std::cout << "uniform not found " << name << std::endl;
		return;
	}

	glUniform3fv(loc, 1, value_ptr(v));
}

void SetCameraUniforms()
{
	SetUniformVec3("cameraPosition", camera.getPosition());
	SetUniformVec3("topLeft", camera.getTopLeft());
	SetUniformVec3("topRight", camera.getTopRight());
	SetUniformVec3("bottomLeft", camera.getBottomLeft());
}

GLuint vertBuffer;
GLuint indexBuffer;

void CreateBuffers()
{
	float vertices [] =
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

void RenderSphereflake()
{
	glDrawArrays(GL_QUADS, 0, 4);
}

void error_callback(int error, const char* description)
{
	std::cout << description << std::endl;
}

GLuint tex;
GLuint pbo;

void CreateTexture()
{
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
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

void UploadGBufferTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GBufferPositionsTexture);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4 * sizeof(float), rts.GetGBuffer().positions.data(), GL_STREAM_DRAW);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RT_W, RT_H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, GBufferNormalsTexture);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4 * sizeof(float), rts.GetGBuffer().normals.data(), GL_STREAM_DRAW);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RT_W, RT_H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void UploadTexture()
{
	glBufferData(GL_PIXEL_UNPACK_BUFFER, RT_W * RT_H * 4, rts.GetBitmap().data(), GL_STREAM_DRAW);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RT_W, RT_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

int main(int argc, char* argv [])
{
	glfwSetErrorCallback(&error_callback);

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

		glfwSwapInterval(0);
	ReloadShader();
	glEnable(GL_TEXTURE_2D);
	CreateBuffers();
	//CreateTexture();
	CreateGBufferTextures();

	glViewport(0, 0, WND_WIDTH, WND_HEIGHT);

	double lastTime = glfwGetTime();
	double fpsTimeAccum = 0.0;
	size_t fpsCounter = 0;
	camera.setPosition(vec3(0, 4, 0));
	camera.setRoll(radians(-90.0));

	double lastXpos = 0.0;
	double lastYpos = 0.0;

	//rts.DoImage(&camera);

	bool doRender = true;
	
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
			camera.setPosition(camera.getPosition() + camera.getOrientation() * vec3(-1, 0, 0) * cameraSpeed);
			doRender = true;
		}
		
		if (glfwGetKey(window, GLFW_KEY_A))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(1, 0, 0) * cameraSpeed);
			doRender = true;
		}

		if (glfwGetKey(window, GLFW_KEY_S))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, 1) * cameraSpeed);
			doRender = true;
		}

		if (glfwGetKey(window, GLFW_KEY_W))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, -1) * cameraSpeed);
			doRender = true;
		}
		
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, -1, 0) * cameraSpeed);
			doRender = true;
		}

		if (glfwGetKey(window, GLFW_KEY_E))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 1, 0) * cameraSpeed);
			doRender = true;
		}

		if (glfwGetKey(window, GLFW_KEY_Z))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, -1) * cameraSpeed * 0.5f);
			
			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
				camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, -1) * cameraSpeed * 0.005f);
			doRender = true;
		}

		if (glfwGetKey(window, GLFW_KEY_X))
		{
			camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, 1) * cameraSpeed * 0.5f);

			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
				camera.setPosition(camera.getPosition() + camera.getOrientation() *vec3(0, 0, 1) * cameraSpeed * 0.005f);

			doRender = true;
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
			camera.setYaw(camera.getYaw() + -deltax * 0.001);
			camera.setRoll(camera.getRoll() + -deltay * 0.001);
		}

		rts.DoFrame(&camera);
		//UploadTexture();
		UploadGBufferTextures();
		RenderSphereflake();
		glfwSwapBuffers(window);

	//	std::cout << "drawn " << spheresDrawn << " spheres" << std::endl;
		spheresDrawn = 0;

		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}