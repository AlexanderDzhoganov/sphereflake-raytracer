--------------------------
Sphereflake Raytracer v1.0
--------------------------

--------
Overview
--------

This program renders the "Sphereflake" fractal. The rendering is done as a two-step process - a CPU raytracing step and a GPU screen-space post-processing step.
The first step involves a single ray-per-pixel rendering of the fractal into a G-Buffer containing screen-space position and normal data. 
After that a post-processing step on the GPU computes approximate ambient occlusion using a variant of the widespread Screen-space Ambient Occlusion (SSAO) algorithm.
The raytracing step uses frame-less rendering to allow moving about the scene in real-time.

-----------------------
Build requirements
-----------------------

Microsoft's Visual Studio 2012 (or later) or similarly capable versions of GCC and Clang.

GLFW 3.0.4 - cross-platform library for initializing OpenGL (http://sourceforge.net/projects/glfw/files/glfw/3.0.4/)
GLEW 1.11.0 - cross-platform library for loading OpenGL extensions (http://sourceforge.net/projects/glew/files/glew/1.11.0/)
GLM 0.9.5.4 - cross-platform library for linear algebra (http://sourceforge.net/projects/ogl-math/files/glm-0.9.5.4/)

Copies of all external dependencies have been provided as part of the software package in the <PACKAGE_ROOT>/lib/ folder.

----------------------
Run-time requirements
----------------------

AVX or SSE- capable CPU
OpenGL 4.2- capable GPU

Please ensure that no other CPU-intensive applications are running and that your CPU is adequately cooled as this software will utilize it at a 100%.
The author holds no responsibility for any damage caused to your hardware.

--------
Controls
--------

W, S, A, D - camera translation
Right click (hold) + mouse move - camera rotation

----------------------
Command-line arguments
----------------------

The executable accepts command-line arguments that set the size of the render viewport. The default size is 1280x720, which provides a decent trade-off between quality and performance.
The upper limit is set by the maximum allowed OpenGL framebuffer size, which according to the specification must be at least 16384x16384. 

--width=X - sets the width of the window
--height=Y - sets the height of the window
--fullscreen=true - starts as a fullscreen window on the primary monitor

--------------------------
Performance considerations
--------------------------

This software has been optimized for Intel's Haswell microarchitecture. All performance measurements were taken on a stock Haswell i7-4970k chip using Intel's VTune Amplifier XE 2013.
The shader code has been tested on an Nvidia GTX780 Ti and AMD Radeon HD7750.
For the best experience it is highly recommended to run this software on a 4-core, AVX 256-bit wide capable CPU.
No Haswell-specific instructions have been used (e.g. fmadd) and an SSE version (albeit much slower) has also been provided in case an AVX-capable chip is not available.

------------
Known issues
------------

- Clipping the camera through a sphere causes rendering artifacts and should be avoided.
- Low frames per second - sometimes the OpenGL thread doesn't get enough time to initialize and output starts stuttering. Either restart the application or wait for a few seconds until performance stabilizes.
The application should vsync at 60hz on a modern GPU.
- Ambient occlusion is wrong - the SSAO algorithm is a crude approximation of ambient occlusion and as such many defects are visible.

---------
Copyright
---------

Copyright (c) 2014, Alexander Dzhoganov (alexander.dzhoganov@gmail.com)
Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,
provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 