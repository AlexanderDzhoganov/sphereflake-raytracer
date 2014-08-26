Sphereflake Raytracer v1.0

--------
Overview
--------

This program renders the "Sphereflake" fractal. The rendering is done as a two-step process - a CPU raytracing step and a GPU screen-space post-processing step.
The first step involves a single ray-per-pixel sampling of the fractal into a G-Buffer containing screen-space position and normal data. 
After that a post-processing step on the GPU computes approximate ambient occlusion using a variant of the widespread SSAO algorithm.
The raytracing step uses frame-less rendering to allow moving about the scene in real-time.

------------------
Build requirements
------------------

Microsoft's Visual Studio 2012 (or later) or similarly capable versions of GCC and Clang.

GLFW 3.0.4 - cross-platform library for initializing OpenGL (http://sourceforge.net/projects/glfw/files/glfw/3.0.4/)
GLEW 1.11.0 - cross-platform library for loading OpenGL extensions (http://sourceforge.net/projects/glew/files/glew/1.11.0/)
GLM 0.9.5.4 - cross-platform library for linear algebra (http://sourceforge.net/projects/ogl-math/files/glm-0.9.5.4/)

Copies of all external dependencies have been provided in the <ROOT>/lib folder.
MSVC compiled binaries can be found in <ROOT>/bin/.

----------------------
Run-time requirements
----------------------

64-bit AVX or SSE- capable CPU
OpenGL 4.2- capable GPU

The dependencies glew32.dll and glfw3.dll, and the Shaders/ folder have to be available in the working directory of the executable.
Please ensure that no other CPU-intensive applications are running and that your CPU is adequately cooled.

--------
Controls
--------

W, S - translate camera forward/ backwards
A, D - translate camera left/ right
Q, E - translate camera up/ down
Right click (hold) + mouse move - rotate camera

----------------------
Command-line arguments
----------------------

The executable accepts command-line arguments that set the size of the render target and the desktop window.
The default size is 1280x720, which provides a decent trade-off between quality and performance.
Resolutions up to 16384x16384 are supported.

--width=X - width of the output window
--height=Y - height of the output window
--fullscreen - initializes a full-screen window on the primary monitor

--------------------------
Performance considerations
--------------------------

This software is optimized for Intel's Haswell microarchitecture.
All performance measurements were done on a stock Haswell i7-4790k chip using Intel's VTune Amplifier XE 2013.
The GLSL shader code has been tested on modern chips from all 3 vendors.
For the best experience it is highly recommended to run this on a 4-core, 256- wide AVX capable CPU.
No Haswell-specific instructions have been used (e.g. fmadd) and an SSE version (albeit much slower) has also been provided in case an AVX-capable chip is not available.

------------
Known issues
------------

- Moving the camera origin inside a sphere causes artifacts
	The collision check necessary to fix this is too expensive for such little gain.
	It requires deliberate effort on the side of the user to achieve this as the camera speed scales with the distance to the closest visible sphere in the fractal.
- High frametime
	Sometimes the OpenGL thread doesn't stabilize before the raytracing threads spin up and the output stutters for a while.
	Either restart the application or wait for a few seconds until performance stabilizes. A modern GPU should easily achieve stable 60 frames per second.
- Lighting is wrong 
	SSAO is a crude approximation of ambient occlusion and many defects are easily spotted.