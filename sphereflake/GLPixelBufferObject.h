#ifndef __SPHEREFLAKERAYTRACER_GLPIXELBUFFEROBJECT_H
#define __SPHEREFLAKERAYTRACER_GLPIXELBUFFEROBJECT_H

namespace SphereflakeRaytracer
{

	namespace GL
	{

		class PixelBufferObject
		{

			public:
			PixelBufferObject()
			{
				glGenBuffers(1, &m_Handle);
			}

			~PixelBufferObject()
			{
				glDeleteBuffers(1, &m_Handle);
			}

			template <typename T>
			void BufferData(const std::vector<T>& data)
			{
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_Handle);
				glBufferData(GL_PIXEL_UNPACK_BUFFER, data.size() * sizeof(T), data.data(), GL_STREAM_DRAW);
			}

			GLuint GetHandle()
			{
				return m_Handle;
			}

			private:
			GLuint m_Handle;

		};

	}

}

#endif
