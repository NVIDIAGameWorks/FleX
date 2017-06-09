/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once


enum NvFlexResult
{
	eNvFlexSuccess = 0,
	eNvFlexFail = 1
};

typedef int NvFlexInt;
typedef float NvFlexFloat;
typedef unsigned int NvFlexUint;
typedef unsigned long long NvFlexUint64;

struct NvFlexDim
{
	NvFlexUint x, y, z;
};

struct NvFlexUint3
{
	NvFlexUint x, y, z;
};

struct NvFlexUint4
{
	NvFlexUint x, y, z, w;
};

struct NvFlexFloat3
{
	float x, y, z;
};

struct NvFlexFloat4
{
	float x, y, z, w;
};

struct NvFlexFloat4x4
{
	NvFlexFloat4 x, y, z, w;
};

enum NvFlexFormat
{
	eNvFlexFormat_unknown = 0,

	eNvFlexFormat_r32_typeless = 1,

	eNvFlexFormat_r32_float = 2,
	eNvFlexFormat_r32g32_float = 3,
	eNvFlexFormat_r32g32b32a32_float = 4,

	eNvFlexFormat_r16_float = 5,
	eNvFlexFormat_r16g16_float = 6,
	eNvFlexFormat_r16g16b16a16_float = 7,

	eNvFlexFormat_r32_uint = 8,
	eNvFlexFormat_r32g32_uint = 9,
	eNvFlexFormat_r32g32b32a32_uint = 10,

	eNvFlexFormat_r8_unorm = 11,
	eNvFlexFormat_r8g8_unorm = 12,
	eNvFlexFormat_r8g8b8a8_unorm = 13,

	eNvFlexFormat_r16_unorm = 14,
	eNvFlexFormat_r16g16_unorm = 15,
	eNvFlexFormat_r16g16b16a16_unorm = 16,

	eNvFlexFormat_d32_float = 17,
	eNvFlexFormat_d24_unorm_s8_uint = 18,

	eNvFlexFormat_r8_snorm = 19,
	eNvFlexFormat_r8g8_snorm = 20,
	eNvFlexFormat_r8g8b8a8_snorm = 21,

	eNvFlexFormat_max
};