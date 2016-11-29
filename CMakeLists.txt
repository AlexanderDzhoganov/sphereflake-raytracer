cmake_minimum_required(VERSION 3.2)
project(sphereflake)

set (CMAKE_CXX_STANDARD 11)

include(CheckCXXCompilerFlag)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

find_package(OpenGL REQUIRED)

add_definitions(-msse3)
add_definitions(-Ofast)
add_definitions(-march=native)
add_definitions(-flto)

add_definitions(-D__ARCH_NO_AVX)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(lib/glfw/include/)
include_directories(lib/glm/glm/)

add_subdirectory(lib/glfw)

file(GLOB SOURCES "sphereflake/*.h" "sphereflake/*.cpp")
add_executable(sphereflake-sse3 ${SOURCES})

target_link_libraries(sphereflake-sse3 glfw)

target_link_libraries(sphereflake-sse3 ${OPENGL_gl_LIBRARY})
