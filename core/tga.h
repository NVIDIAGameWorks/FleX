
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

#pragma once

#include "types.h"

#include <algorithm>
#include <stdio.h>

struct TgaImage
{
	uint32_t SampleClamp(int x, int y) const
	{
		uint32_t ix = std::min(std::max(0, x), int(m_width-1));
		uint32_t iy = std::min(std::max(0, y), int(m_height-1));

		return m_data[iy*m_width + ix];
	}

	uint16_t m_width;
	uint16_t m_height;

	// pixels are always assumed to be 32 bit
	uint32_t* m_data;
};

bool TgaSave(const char* filename, const TgaImage& image, bool rle=false);
bool TgaSave(FILE* file, const TgaImage& image, bool rle=false);
bool TgaLoad(const char* filename, TgaImage& image);
void TgaFree(const TgaImage& image);
