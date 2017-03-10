// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 20132017 NVIDIA Corporation. All rights reserved.

#pragma once

#include "../../core/maths.h"

#if _WIN32

#define ENABLE_SIMPLE_FLUID 0

#include "../../external/glew/include/gl/glew.h"
#include "../../external/SDL2-2.0.4/include/SDL.h"

#pragma comment (lib, "glu32.lib")    /* link OpenGL Utility lib     */
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */

#include <gl/GL.h>
#include <gl/GLU.h>

// Begin Add Android Support
#elif ANDROID
#include <GL/Regal.h>
// End Add Android Support

#elif __linux__
#include <external/glew/include/GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>

#elif __APPLE__
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 
#include <opengl/gl3.h>
#include <glut/glut.h>
#elif PLATFORM_IOS

#if OGL1
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#else
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif

#endif

#include <vector>

#if defined(NDEBUG)
#define glVerify(x) x
#else
#define glVerify(x) {x; glAssert(#x, __LINE__, __FILE__);}
void glAssert(const char* msg, long line, const char* file);
#endif

GLuint CompileProgram(const char *vsource=NULL, const char *fsource=NULL, const char* gsource=NULL);

void DrawPlane(const Vec4& p, bool color=true);

