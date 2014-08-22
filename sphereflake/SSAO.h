#ifndef __SSAO_H
#define __SSAO_H

#define NOISE_TEXTURE_SIZE 64

namespace SphereflakeRaytracer
{

	class SSAO
	{

		public:
		SSAO(size_t width, size_t height, int downScale);

		void SetSampleRadiusMultiplier(float m)
		{
			m_SSAOSampleRadius = 30.0f * m;
		}

		void Render();

		GLuint GetSSAOTexture()
		{
			return m_BlurVerticalTarget->GetTexture();
		}

		private:
		void GenerateNoiseTexture();

		void BindNoiseTexture();

		std::unique_ptr<GL::FramebufferObject> m_SSAOTarget = nullptr;
		std::unique_ptr<GL::FramebufferObject> m_BlurVerticalTarget = nullptr;
		std::unique_ptr<GL::FramebufferObject> m_BlurHorizontalTarget = nullptr;

		std::unique_ptr<GL::Program> m_SSAOProgram = nullptr;
		std::unique_ptr<GL::Program> m_BlurProgram = nullptr;

		std::vector<vec3> m_Kernel;
		GLuint m_NoiseTexture;

		float m_SSAOSampleRadius = 30.0f;
		float m_SSAOIntensity = 0.51f;
		float m_SSAOScale = 3.28f;
		float m_SSAOBias = 0.23f;
		float m_NormalThreshold = 2.47;
		float m_DepthThreshold = 0.01;

	};

}

#endif
