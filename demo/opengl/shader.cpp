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

#include "shader.h"

#include "../../core/types.h"
#include "../../core/maths.h"
#include "../../core/platform.h"
#include "../../core/tga.h"

#include <stdarg.h>
#include <stdio.h>

#define WITH_GLEW

void glAssert(const char* msg, long line, const char* file)
{
	struct glError
	{
		GLenum code;
		const char* name;
	};

	static const glError errors[] =
	{
		{ GL_NO_ERROR, "No Error" },
		{ GL_INVALID_ENUM, "Invalid Enum" },
		{ GL_INVALID_VALUE, "Invalid Value" },
		{ GL_INVALID_OPERATION, "Invalid Operation" },
#if OGL1
		{ GL_STACK_OVERFLOW, "Stack Overflow" },
		{ GL_STACK_UNDERFLOW, "Stack Underflow" },
		{ GL_OUT_OF_MEMORY, "Out Of Memory" }
#endif
	};

	GLenum e = glGetError();

	if (e == GL_NO_ERROR)
	{
		return;
	}
	else
	{
		const char* errorName = "Unknown error";

		// find error message
		for (uint32_t i = 0; i < sizeof(errors) / sizeof(glError); i++)
		{
			if (errors[i].code == e)
			{
				errorName = errors[i].name;
			}
		}

		printf("OpenGL: %s - error %s in %s at line %d\n", msg, errorName, file, int(line));
		assert(0);
	}
}

namespace OGL_Renderer
{
	void GlslPrintShaderLog(GLuint obj)
	{
#if !PLATFORM_IOS
		int infologLength = 0;
		int charsWritten = 0;
		char *infoLog;

		GLint result;
		glGetShaderiv(obj, GL_COMPILE_STATUS, &result);

		// only print log if compile fails
		if (result == GL_FALSE)
		{
			glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

			if (infologLength > 1)
			{
				infoLog = (char *)malloc(infologLength);
				glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
				printf("%s\n", infoLog);
				free(infoLog);
			}
		}
#endif
	}

	void PreProcessShader(const char* filename, std::string& source)
	{
		// load source
		FILE* f = fopen(filename, "r");

		if (!f)
		{
			printf("Could not open shader file for reading: %s\n", filename);
			return;
		}

		// add lines one at a time handling include files recursively
		while (!feof(f))
		{
			char buf[1024];

			if (fgets(buf, 1024, f) != NULL)
			{
				// test for #include
				if (strncmp(buf, "#include", 8) == 0)
				{
					const char* begin = strchr(buf, '\"');
					const char* end = strrchr(buf, '\"');

					if (begin && end && (begin != end))
					{
						// lookup file relative to current file
						PreProcessShader((StripFilename(filename) + std::string(begin + 1, end)).c_str(), source);
					}
				}
				else
				{
					// add line to output
					source += buf;
				}
			}
		}

		fclose(f);
	}

	GLuint CompileProgram(const char *vsource, const char *fsource, const char* gsource)
	{
		GLuint vertexShader = GLuint(-1);
		GLuint geometryShader = GLuint(-1);
		GLuint fragmentShader = GLuint(-1);

		GLuint program = glCreateProgram();

		if (vsource)
		{
			vertexShader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertexShader, 1, &vsource, 0);
			glCompileShader(vertexShader);
			GlslPrintShaderLog(vertexShader);
			glAttachShader(program, vertexShader);
		}

		if (fsource)
		{
			fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragmentShader, 1, &fsource, 0);
			glCompileShader(fragmentShader);
			GlslPrintShaderLog(fragmentShader);
			glAttachShader(program, fragmentShader);
		}

		if (gsource)
		{
			geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometryShader, 1, &gsource, 0);
			glCompileShader(geometryShader);
			GlslPrintShaderLog(geometryShader);

			// hack, force billboard gs mode
			glAttachShader(program, geometryShader);
			glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, 4);
			glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
			glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
		}

		glLinkProgram(program);

		// check if program linked
		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (!success) {
			char temp[256];
			glGetProgramInfoLog(program, 256, 0, temp);
			printf("Failed to link program:\n%s\n", temp);
			glDeleteProgram(program);
			program = 0;
		}

		return program;
	}

	void DrawPlane(const Vec4& p, bool color)
	{
		Vec3 u, v;
		BasisFromVector(Vec3(p.x, p.y, p.z), &u, &v);

		Vec3 c = Vec3(p.x, p.y, p.z)*-p.w;

		glBegin(GL_QUADS);

		if (color)
			glColor3fv(p*0.5f + Vec4(0.5f, 0.5f, 0.5f, 0.5f));

		float kSize = 200.0f;

		// draw a grid of quads, otherwise z precision suffers
		for (int x = -3; x <= 3; ++x)
		{
			for (int y = -3; y <= 3; ++y)
			{
				Vec3 coff = c + u*float(x)*kSize*2.0f + v*float(y)*kSize*2.0f;

				glTexCoord2f(1.0f, 1.0f);
				glNormal3f(p.x, p.y, p.z);
				glVertex3fv(coff + u*kSize + v*kSize);

				glTexCoord2f(0.0f, 1.0f);
				glNormal3f(p.x, p.y, p.z);
				glVertex3fv(coff - u*kSize + v*kSize);

				glTexCoord2f(0.0f, 0.0f);
				glNormal3f(p.x, p.y, p.z);
				glVertex3fv(coff - u*kSize - v*kSize);

				glTexCoord2f(1.0f, 0.0f);
				glNormal3f(p.x, p.y, p.z);
				glVertex3fv(coff + u*kSize - v*kSize);
			}
		}

		glEnd();
	}
}
