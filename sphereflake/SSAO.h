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
			m_SSAOSampleRadius = 8.0f * m;
		}

		void Render();

		GLuint GetSSAOTexture()
		{
			return m_BlurVerticalTarget->GetTexture();
		}

		private:
		void GenerateNoiseTexture();

		void BindNoiseTexture();

		std::shared_ptr<GL::FramebufferObject> m_SSAOTarget;
		std::shared_ptr<GL::FramebufferObject> m_BlurVerticalTarget;
		std::shared_ptr<GL::FramebufferObject> m_BlurHorizontalTarget;

		std::shared_ptr<GL::Program> m_SSAOProgram;
		std::shared_ptr<GL::Program> m_BlurProgram;

		std::vector<vec3> m_Kernel;
		GLuint m_NoiseTexture;

		float m_SSAOSampleRadius;
		float m_SSAOIntensity;
		float m_SSAOScale;
		float m_SSAOBias;
		float m_NormalThreshold;
		float m_DepthThreshold;

	};

}

#endif
