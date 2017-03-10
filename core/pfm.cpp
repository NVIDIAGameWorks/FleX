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
// Copyright (c) 2013-2016 NVIDIA Corporation. All rights reserved.

#include "pfm.h"

#include <cassert>
#include <stdio.h>
#include <string.h>
#include <algorithm>

#if _WIN32
#pragma warning(disable: 4996)  // secure io
#pragma warning(disable: 4100)  // unreferenced param
#pragma warning(disable: 4324)  // structure was padded due to __declspec(align())
#endif

namespace
{
	// RAII wrapper to handle file pointer clean up
	struct FilePointer
	{
		FilePointer(FILE* ptr) : p(ptr) {}
		~FilePointer() { if (p) fclose(p); }

		operator FILE*() { return p; }

		FILE* p;
	};
}

bool PfmLoad(const char* filename, PfmImage& image)
{
	FilePointer f = fopen(filename, "rb");
	if (!f)
		return false;

	memset(&image, 0, sizeof(PfmImage));
	
	const uint32_t kBufSize = 1024;
	char buffer[kBufSize];
	
	if (!fgets(buffer, kBufSize, f))
		return false;
	
	if (strcmp(buffer, "PF\n") != 0)
		return false;
	
	if (!fgets(buffer, kBufSize, f))
		return false;

	image.m_depth = 1;
	sscanf(buffer, "%d %d %d", &image.m_width, &image.m_height, &image.m_depth);

	if (!fgets(buffer, kBufSize, f))
		return false;
	
	sscanf(buffer, "%f", &image.m_maxDepth);
	
	uint32_t dataStart = ftell(f);
	fseek(f, 0, SEEK_END);
	uint32_t dataEnd = ftell(f);
	fseek(f, dataStart, SEEK_SET);
	
	uint32_t dataSize = dataEnd-dataStart;

	if (dataSize != sizeof(float)*image.m_width*image.m_height*image.m_depth)
		return false;
	
	image.m_data = new float[dataSize/4];
	
	if (fread(image.m_data, dataSize, 1, f) != 1)
		return false;
	
	return true;
}

void PfmSave(const char* filename, const PfmImage& image)
{
	FilePointer f = fopen(filename, "wb");
	if (!f)
		return;

	fprintf(f, "PF\n");
	if (image.m_depth > 1)
		fprintf(f, "%d %d %d\n", image.m_width, image.m_height, image.m_depth);
	else
		fprintf(f, "%d %d\n", image.m_width, image.m_height);

	fprintf(f, "%f\n", *std::max_element(image.m_data, image.m_data+(image.m_width*image.m_height*image.m_depth)));

	fwrite(image.m_data, image.m_width*image.m_height*image.m_depth*sizeof(float), 1, f);
}





