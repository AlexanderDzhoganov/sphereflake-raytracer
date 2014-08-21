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
