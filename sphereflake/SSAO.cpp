#pragma warning (push, 0)
#pragma warning (disable: 4530) // disable warnings from code not under our control

#include <iostream>
#include <vector>
#include <memory>
#include <random>
#include <fstream>

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

#include "Util.h"
#include "Filesystem.h"
#include "GLProgram.h"
#include "GLPixelBufferObject.h"
#include "GLTexture2D.h"
#include "GLFramebufferObject.h"

#ifdef __ARCH_NO_AVX
#include <tmmintrin.h>
#include "SIMD_SSE.h"
#else
#include <immintrin.h>
#include "SIMD_AVX.h"
#endif

#include "SSAO.h"

namespace SphereflakeRaytracer
{

	SSAO::SSAO(size_t width, size_t height, int downScale) :
		m_SSAOSampleRadius(0.0f),
		m_SSAOIntensity(0.51f),
		m_SSAOScale(3.28f),
		m_SSAOBias(0.23f),
		m_NormalThreshold(2.47f),
		m_DepthThreshold(0.01f)
	{
		GenerateNoiseTexture();
		m_SSAOTarget = std::make_unique<GL::FramebufferObject>(width / downScale, height / downScale);
		m_BlurVerticalTarget = std::make_unique<GL::FramebufferObject>(width, height);
		m_BlurHorizontalTarget = std::make_unique<GL::FramebufferObject>(width, height);

		m_SSAOProgram = std::make_unique<GL::Program>
		(
			Filesystem::ReadAllText("Shaders/post_vertex.glsl"),
			Filesystem::ReadAllText("Shaders/post_ssao.glsl")
		);

		m_BlurProgram = std::make_unique<GL::Program>
		(
			Filesystem::ReadAllText("Shaders/post_vertex.glsl"),
			Filesystem::ReadAllText("Shaders/post_ssao_blur.glsl")
		);
	}

	void SSAO::Render()
	{
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

	void SSAO::GenerateNoiseTexture()
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

	void SSAO::BindNoiseTexture()
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
	}

}