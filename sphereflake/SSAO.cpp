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

#include "Filesystem.h"
#include "GLProgram.h"
#include "GLPixelBufferObject.h"
#include "GLTexture2D.h"
#include "GLFramebufferObject.h"
#include "GL.h"

#ifdef __ARCH_NO_AVX
#include <tmmintrin.h>
#include "SIMD_SSE.h"
#else
#include <immintrin.h>
#include "SIMD_AVX.h"
#endif

#include "SSAO.h"