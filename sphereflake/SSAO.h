#ifndef __SSAO_H
#define __SSAO_H

#define NOISE_TEXTURE_SIZE 64

extern GLFWwindow* window;

namespace SphereflakeRaytracer
{

	class SSAO
	{

		public:
		SSAO(size_t width, size_t height, int downScale)
		{
			GenerateNoiseTexture();
			m_SSAOTarget = std::make_unique<GL::FramebufferObject>(width / downScale, height / downScale);
			m_BlurVerticalTarget = std::make_unique<GL::FramebufferObject>(width, height);
			m_BlurHorizontalTarget= std::make_unique<GL::FramebufferObject>(width, height);

			m_SSAOProgram = std::make_unique<GL::Program>
			(
				Filesystem::ReadAllText("post_vertex.glsl"),
				Filesystem::ReadAllText("post_ssao.glsl")
			);

			m_BlurProgram = std::make_unique<GL::Program>
			(
				Filesystem::ReadAllText("post_vertex.glsl"),
				Filesystem::ReadAllText("post_ssao_blur.glsl")
			);
		}

		void SetSampleRadiusMultiplier(float m)
		{
			m_SSAOSampleRadius = 30.0f * m;
		}

		void Render()
		{
			if (glfwGetKey(window, GLFW_KEY_T))
			{
				m_SSAOSampleRadius += 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_Y))
			{
				m_SSAOSampleRadius -= 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_G))
			{
				m_SSAOIntensity += 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_H))
			{
				m_SSAOIntensity -= 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_B))
			{
				m_SSAOScale += 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_N))
			{
				m_SSAOScale -= 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_U))
			{
				m_SSAOBias += 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_I))
			{
				m_SSAOBias -= 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_J))
			{
				m_NormalThreshold += 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_K))
			{
				m_NormalThreshold -= 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_M))
			{
				m_DepthThreshold += 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET))
			{
				m_DepthThreshold -= 0.01f;
			}

			if (glfwGetKey(window, GLFW_KEY_SPACE))
			{
				std::cout << "SSAOSampleRadius: " << m_SSAOSampleRadius << std::endl;
				std::cout << "SSAOIntensity: " << m_SSAOIntensity << std::endl;
				std::cout << "SSAOScale: " << m_SSAOScale << std::endl;
				std::cout << "SSAOBias: " << m_SSAOBias << std::endl;
				std::cout << "normalThreshold: " << m_NormalThreshold << std::endl;
				std::cout << "depthThreshold: " << m_DepthThreshold << std::endl;
			}

			BindNoiseTexture();
			m_SSAOTarget->SetActiveDraw();
			m_SSAOProgram->Use();
			m_SSAOProgram->SetUniform("framebufferSize", vec2(m_SSAOTarget->GetWidth(), m_SSAOTarget->GetHeight()));

			m_SSAOProgram->SetUniform("SSAOSampleRadius", m_SSAOSampleRadius);
			m_SSAOProgram->SetUniform("SSAOIntensity", m_SSAOIntensity);
			m_SSAOProgram->SetUniform("SSAOScale", m_SSAOScale);
			m_SSAOProgram->SetUniform("SSAOBias", m_SSAOBias);

			DRAW_FULLSCREEN_QUAD();
			
			m_BlurHorizontalTarget->SetActiveDraw();

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, m_SSAOTarget->GetTexture());
			
			m_BlurProgram->Use();
			m_BlurProgram->SetUniform("normalThreshold", m_NormalThreshold);
			m_BlurProgram->SetUniform("depthThreshold", m_DepthThreshold);
			m_BlurProgram->SetUniform("framebufferSize", vec2(m_BlurHorizontalTarget->GetWidth(), m_BlurHorizontalTarget->GetHeight()));
			m_BlurProgram->SetUniform("blurDirection", vec2(1.0, 0.0));

			DRAW_FULLSCREEN_QUAD();

			m_BlurVerticalTarget->SetActiveDraw();
			glBindTexture(GL_TEXTURE_2D, m_BlurHorizontalTarget->GetTexture());
			
			m_BlurProgram->SetUniform("framebufferSize", vec2(m_BlurVerticalTarget->GetWidth(), m_BlurVerticalTarget->GetHeight()));
			m_BlurProgram->SetUniform("normalThreshold", m_NormalThreshold);
			m_BlurProgram->SetUniform("depthThreshold", m_DepthThreshold);
			m_BlurProgram->SetUniform("blurDirection", vec2(0.0, 1.0));

			DRAW_FULLSCREEN_QUAD();
		}

		GLuint GetSSAOTexture()
		{
			return m_BlurVerticalTarget->GetTexture();
		}

		const std::unique_ptr<GL::FramebufferObject>& GetSSAOTarget()
		{
			return m_BlurHorizontalTarget;
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
