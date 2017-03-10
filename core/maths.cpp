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

#include "maths.h"

uint32_t seed1;
uint32_t seed2;

void RandInit()
{
	seed1 = 315645664;
	seed2 = seed1 ^ 0x13ab45fe;
}

static float s_identity[4][4] = { { 1.0f, 0.0f, 0.0f, 0.0f },
{ 0.0f, 1.0f, 0.0f, 0.0f },
{ 0.0f, 0.0f, 1.0f, 0.0f },
{ 0.0f, 0.0f, 0.0f, 1.0f } };

template <>
XMatrix44<float> XMatrix44<float>::kIdentity(s_identity[0]);


Colour::Colour(Colour::Preset p)
{
	switch (p)
	{
	case kRed:
		*this = Colour(1.0f, 0.0f, 0.0f);
		break;
	case kGreen:
		*this = Colour(0.0f, 1.0f, 0.0f);
		break;
	case kBlue:
		*this = Colour(0.0f, 0.0f, 1.0f);
		break;
	case kWhite:
		*this = Colour(1.0f, 1.0f, 1.0f);
		break;
	case kBlack:
		*this = Colour(0.0f, 0.0f, 0.0f);
		break;
	};
}
