#ifndef __SSAO_H
#define __SSAO_H

#define KERNEL_SIZE 32
#define NOISE_TEXTURE_SIZE 512

namespace SphereflakeRaytracer
{

	class SSAO
	{

		public:
		SSAO()
		{

			ComputeKernel();
			GenerateNoiseTexture();
			SetUniforms();
		}

		void BindNoiseTexture()
		{
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
		}

		private:
		void ComputeKernel()
		{
			m_Kernel.resize(KERNEL_SIZE);
			std::mt19937 mtengine;
			mtengine.seed(6126126);
			std::uniform_real_distribution<float> dist01(0, 1);
			std::uniform_real_distribution<float> distNeg11(-1, 1);

			for (auto i = 0; i < KERNEL_SIZE; i++)
			{
				m_Kernel[i] = dist01(mtengine) * normalize
					(
					vec3
					(
					distNeg11(mtengine),
					distNeg11(mtengine),
					dist01(mtengine)
					)
					);

				auto scale = (float)i / (float)KERNEL_SIZE;
				scale = 0.1f + 0.9f * scale * scale;
				m_Kernel[i] *= scale;
			}
		}

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
					0.0f, 0.0f
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

		void SetUniforms()
		{
			SetUniformVec3Array("rotationKernel[0]", m_Kernel);
		}

		std::vector<vec3> m_Kernel;
		GLuint m_NoiseTexture;

	};

}

#endif
