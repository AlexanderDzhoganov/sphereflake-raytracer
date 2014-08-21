#ifndef __SPHEREFLAKERAYTRACER_GLFRAMEBUFFEROBJECT_H
#define __SPHEREFLAKERAYTRACER_GLFRAMEBUFFEROBJECT_H

#define DRAW_FULLSCREEN_QUAD() glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

namespace SphereflakeRaytracer
{

	namespace GL
	{

		class FramebufferObject
		{

			public:
			FramebufferObject(size_t width, size_t height);
			~FramebufferObject();

			GLuint GetTexture()
			{
				return m_TextureHandle;
			}

			template <typename T>
			void Blit(const T& targetFBO)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Handle);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFBO->m_Handle);
				
				GLenum filter = m_Width == targetFBO->m_Width && m_Height == targetFBO->m_Height ? GL_NEAREST : GL_LINEAR;
				glBlitFramebuffer(0, 0, (GLint)m_Width, (GLint)m_Height, 0, 0, (GLint)targetFBO->m_Width, (GLint)targetFBO->m_Height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}

			void BlitToDefaultFramebuffer(size_t targetWidth, size_t targetHeight)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Handle);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				GLenum filter = m_Width == targetWidth && m_Height == targetHeight ? GL_NEAREST : GL_LINEAR;
				glBlitFramebuffer(0, 0, (GLint)m_Width, (GLint)m_Height, 0, 0, (GLint)targetWidth, (GLint)targetHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}

			size_t GetWidth() const
			{
				return m_Width;
			}

			size_t GetHeight() const
			{
				return m_Height;
			}

			void SetActiveDraw();

			private:
			size_t m_Width;
			size_t m_Height;

			GLuint m_Handle;
			GLuint m_TextureHandle;
	
		};

	}

}

#endif
