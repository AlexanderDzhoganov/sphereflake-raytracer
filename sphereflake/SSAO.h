#ifndef __SSAO_H
#define __SSAO_H

#define KERNEL_SIZE 32
#define NOISE_TEXTURE_SIZE 512

namespace SphereflakeRaytracer
{

	class SSAO
	{

		public:
		SSAO(size_t width, size_t height)
		{
			GenerateNoiseTexture();
			m_SSAOTarget = std::make_unique<GL::FramebufferObject>(width, height);

			m_SSAOProgram = std::make_unique<GL::Program>
			(
				Filesystem::ReadAllText("post_vertex.glsl"),
				Filesystem::ReadAllText("post_ssao.glsl")
			);
		}

		void Render()
		{
			BindNoiseTexture();
			m_SSAOTarget->SetActiveDraw();
			m_SSAOProgram->Use();
			m_SSAOProgram->SetUniform("framebufferSize", vec2(m_SSAOTarget->GetWidth(), m_SSAOTarget->GetHeight()));

			DRAW_FULLSCREEN_QUAD();
		}

		GLuint GetSSAOTexture()
		{
			return m_SSAOTarget->GetTexture();
		}

		private:
		void GenerateNoiseTexture()
		{
			std::vector<vec4> noiseMap(NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE);
			std::mt19937 mtengine;
			mtengine.seed(12512);
			std::uniform_real_distribution<float> distNeg11(-1, 1);

			for (auto i = 0; i < NOISE_TEXTURE_SIZE * NOISE_TEXTURE_SIZE; i++)
			{
				noiseMap[i] = normalize
				(
					vec4
					(
						distNeg11(mtengine),
						distNeg11(mtengine),
						distNeg11(mtengine),
						distNeg11(mtengine)
					)
				);
			}

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

			glGenTextures(1, &m_NoiseTexture);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NOISE_TEXTURE_SIZE, NOISE_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, noiseMap.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		void BindNoiseTexture()
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
		}

		std::unique_ptr<GL::FramebufferObject> m_SSAOTarget = nullptr;
		std::unique_ptr<GL::Program> m_SSAOProgram = nullptr;
		std::unique_ptr<GL::Program> m_BlurProgram = nullptr;

		std::vector<vec3> m_Kernel;
		GLuint m_NoiseTexture;

	};

}

#endif
