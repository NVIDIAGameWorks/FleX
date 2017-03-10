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

#define ENABLE_VERBOSE_OUTPUT		0
#define ENABLE_APIC_CAPTURE			0
#define ENABLE_PERFALYZE_CAPTURE	0

#if ENABLE_VERBOSE_OUTPUT
#define VERBOSE(a) a##;
#else
#define VERBOSE(a)
#endif

#define Super __super

// basically just a collection of macros and types
#ifndef UNUSED
#define UNUSED(x) (void)x;
#endif

#define NOMINMAX

#if !PLATFORM_OPENCL
#include <cassert>
#endif

#include "types.h"

#if !PLATFORM_SPU && !PLATFORM_OPENCL
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#endif

// disable some warnings
#if _WIN32
#pragma warning(disable: 4996)  // secure io
#pragma warning(disable: 4100)  // unreferenced param
#pragma warning(disable: 4324)  // structure was padded due to __declspec(align())
#endif

// alignment helpers
#define DEFAULT_ALIGNMENT 16

#if PLATFORM_LINUX
#define ALIGN_N(x) 
#define ENDALIGN_N(x) __attribute__ ((aligned (x)))
#else
#define ALIGN_N(x) __declspec(align(x))
#define END_ALIGN_N(x)
#endif

#define ALIGN ALIGN_N(DEFAULT_ALIGNMENT)
#define END_ALIGN END_ALIGN_N(DEFAULT_ALIGNMENT)

inline bool IsPowerOfTwo(int n)
{
	return (n&(n-1))==0;
}

// align a ptr to a power of tow
template <typename T>
inline T* AlignPtr(T* p, uint32_t alignment)
{
	assert(IsPowerOfTwo(alignment));

	// cast to safe ptr type
	uintptr_t up = reinterpret_cast<uintptr_t>(p);
	return (T*)((up+(alignment-1)) & ~(alignment-1));
}

// align an unsigned value to a power of two
inline uint32_t Align(uint32_t val, uint32_t alignment)
{
	assert(IsPowerOfTwo(alignment));

	return (val+(alignment-1))& ~(alignment-1);
}

inline bool IsAligned(void* p, uint32_t alignment)
{
	return (((uintptr_t)p) & (alignment-1)) == 0;
}

// Endian helpers
template <typename T>
T ByteSwap(const T& val)
{
	T copy = val;
	uint8_t* p = reinterpret_cast<uint8_t*>(&copy);
	
	std::reverse(p, p+sizeof(T));

	return copy;
}

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN WIN32
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN PLATFORM_PS3 || PLATFORM_SPU
#endif

#if BIG_ENDIAN
#define ToLittleEndian(x) ByteSwap(x)
#else
#define ToLittleEndian(x) x
#endif

//#include "platform.h"

//#define sizeof_array(x) (sizeof(x)/sizeof(*x))
template <typename T, size_t N>
size_t sizeof_array(const T (&)[N])
{
	return N;
}

// functor designed for use in the stl
template <typename T>
class free_ptr : public std::unary_function<T*, void>
{
public:

	void operator()(const T* ptr)
	{
		delete ptr;
	}
};
