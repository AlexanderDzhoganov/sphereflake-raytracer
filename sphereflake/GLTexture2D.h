#ifndef __SPHEREFLAKERAYTRACER_GLTEXTURE2D_H
#define __SPHEREFLAKERAYTRACER_GLTEXTURE2D_H

namespace SphereflakeRaytracer
{
	
	namespace GL
	{

		enum class Texture2DFormat
		{
			RGBA_UNSIGNED_BYTE = 0,
			RGBA_FLOAT = 1,
		};

		enum class Texture2DFilter
		{
			NEAREST = 0,
			LINEAR,
		};

		enum class Texture2DWrapMode
		{
			CLAMP_TO_EDGE = 0,
			REPEAT
		};

		class Texture2D
		{

			public:
			Texture2D(size_t width, size_t height, Texture2DFormat format, Texture2DFilter filter, Texture2DWrapMode wrapMode) : m_Width(width), m_Height(height), m_Format(format)
			{
				glGenTextures(1, &m_Handle);
				glBindTexture(GL_TEXTURE_2D, m_Handle);
				SetFilter(filter);
				SetWrapMode(wrapMode);
			}

			void Bind(size_t index)
			{
				glActiveTexture((GLenum)(GL_TEXTURE0 + index));
				glBindTexture(GL_TEXTURE_2D, m_Handle);
			}

			template <typename T>
			void Upload(const T& pbo)
			{
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo->GetHandle());
				glBindTexture(GL_TEXTURE_2D, m_Handle);

				GLenum internalFormat = GL_RGBA;
				GLenum format = GL_RGBA;
				GLenum dataFormat = GL_UNSIGNED_BYTE;

				switch (m_Format)
				{
				case Texture2DFormat::RGBA_FLOAT:
					internalFormat = GL_RGBA32F;
					format = GL_RGBA;
					dataFormat = GL_FLOAT;
					break;
				case Texture2DFormat::RGBA_UNSIGNED_BYTE:
					internalFormat = GL_RGBA;
					format = GL_RGBA;
					dataFormat = GL_UNSIGNED_BYTE;
					break;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei)m_Width, (GLsizei)m_Height, 0, format, dataFormat, 0);
			}

			private:
			void SetFilter(Texture2DFilter filter)
			{
				switch (filter)
				{
				case Texture2DFilter::LINEAR:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					break;
				case Texture2DFilter::NEAREST:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					break;
				}
			}

			void SetWrapMode(Texture2DWrapMode wrapMode)
			{
				switch (wrapMode)
				{
				case Texture2DWrapMode::CLAMP_TO_EDGE:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					break;
				case Texture2DWrapMode::REPEAT:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					break;
				}
			}

			GLuint m_Handle;

			size_t m_Width;
			size_t m_Height;
			Texture2DFormat m_Format;

		};

	}

}

#endif
