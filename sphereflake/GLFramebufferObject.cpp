#pragma warning (push, 0)
#pragma warning (disable: 4530) // disable warnings from code not under our control

#include <string>
#include <memory>
#include <vector>
#include <iostream>

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

#include "GLFramebufferObject.h"

namespace SphereflakeRaytracer
{

	namespace GL
	{

		FramebufferObject::FramebufferObject(size_t width, size_t height) : m_Width(width), m_Height(height)
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			glGenTextures(1, &m_TextureHandle);
			glBindTexture(GL_TEXTURE_2D, m_TextureHandle);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)m_Width, (GLsizei)m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glGenFramebuffers(1, &m_Handle);
			glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_TextureHandle, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cout << "FBO error: incomplete framebuffer" << std::endl;
			}
		}

		FramebufferObject::~FramebufferObject()
		{
			glDeleteTextures(1, &m_TextureHandle);
			glDeleteFramebuffers(1, &m_Handle);
		}

		void FramebufferObject::SetActiveDraw()
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_Handle);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, (GLsizei)m_Width, (GLsizei)m_Height);
		}

	}

}