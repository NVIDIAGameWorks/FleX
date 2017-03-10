// This code contain	s NVIDIA Confidential Information and is disclosed to you
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

#include "maths.h"

#include "quat.h"
#include "vec3.h"

struct Matrix33
{
	CUDA_CALLABLE Matrix33() {}
	CUDA_CALLABLE Matrix33(const Vec3& c1, const Vec3& c2, const Vec3& c3)
	{
		cols[0] = c1;
		cols[1] = c2;
		cols[2] = c3;
	}

	CUDA_CALLABLE Matrix33(const Quat& q)
	{
		cols[0] = Rotate(q, Vec3(1.0f, 0.0f, 0.0f));
		cols[1] = Rotate(q, Vec3(0.0f, 1.0f, 0.0f));
		cols[2] = Rotate(q, Vec3(0.0f, 0.0f, 1.0f));
	}

	CUDA_CALLABLE float operator()(int i, int j) const { return static_cast<const float*>(cols[j])[i]; }
	CUDA_CALLABLE float& operator()(int i, int j) { return static_cast<float*>(cols[j])[i]; }

	Vec3 cols[3];

	CUDA_CALLABLE static inline Matrix33 Identity() {  const Matrix33 sIdentity(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)); return sIdentity; }
};

CUDA_CALLABLE inline Matrix33 Multiply(float s, const Matrix33& m)
{
	Matrix33 r = m;
	r.cols[0] *= s;
	r.cols[1] *= s;	
	r.cols[2] *= s;
	return r;
}

CUDA_CALLABLE inline Vec3 Multiply(const Matrix33& a, const Vec3& x)
{
	return a.cols[0]*x.x + a.cols[1]*x.y + a.cols[2]*x.z;
}

CUDA_CALLABLE inline Vec3 operator*(const Matrix33& a, const Vec3& x) { return Multiply(a, x); }

CUDA_CALLABLE inline Matrix33 Multiply(const Matrix33& a, const Matrix33& b)
{
	Matrix33 r;
	r.cols[0] = a*b.cols[0];
	r.cols[1] = a*b.cols[1];
	r.cols[2] = a*b.cols[2];
	return r;
}

CUDA_CALLABLE inline Matrix33 Add(const Matrix33& a, const Matrix33& b)
{
	return Matrix33(a.cols[0]+b.cols[0], a.cols[1]+b.cols[1], a.cols[2]+b.cols[2]);
}

CUDA_CALLABLE inline float Determinant(const Matrix33& m)
{
	return Dot(m.cols[0], Cross(m.cols[1], m.cols[2]));
}

CUDA_CALLABLE inline Matrix33 Transpose(const Matrix33& a)
{
	Matrix33 r;
	for (uint32_t i=0; i < 3; ++i)
		for(uint32_t j=0; j < 3; ++j)
			r(i, j) = a(j, i);

	return r;
}

CUDA_CALLABLE inline float Trace(const Matrix33& a)
{
	return a(0,0)+a(1,1)+a(2,2);
}

CUDA_CALLABLE inline Matrix33 Outer(const Vec3& a, const Vec3& b)
{
	return Matrix33(a*b.x, a*b.y, a*b.z);
}

CUDA_CALLABLE inline Matrix33 Inverse(const Matrix33& a, bool& success)
{
	float s = Determinant(a);

	const float eps = 1.e-8f;

	if (fabsf(s) > eps)
	{
		Matrix33 b;
		
		b(0,0) = a(1,1)*a(2,2) - a(1,2)*a(2,1);      b(0,1) = a(0,2)*a(2,1) - a(0,1)*a(2,2);      b(0,2) = a(0,1)*a(1,2) - a(0,2)*a(1,1);
		b(1,0) = a(1,2)*a(2,0) - a(1,0)*a(2,2);      b(1,1) = a(0,0)*a(2,2) - a(0,2)*a(2,0);      b(1,2) = a(0,2)*a(1,0) - a(0,0)*a(1,2);
		b(2,0) = a(1,0)*a(2,1) - a(1,1)*a(2,0);      b(2,1) = a(0,1)*a(2,0) - a(0,0)*a(2,1);      b(2,2) = a(0,0)*a(1,1) - a(0,1)*a(1,0);
		
		success = true;

		return Multiply(1.0f/s, b);    
	}
	else
	{
		success = false;
		return Matrix33();
	}
}

CUDA_CALLABLE inline Matrix33 operator*(float s, const Matrix33& a) { return Multiply(s, a); }
CUDA_CALLABLE inline Matrix33 operator*(const Matrix33& a, float s) { return Multiply(s, a); }
CUDA_CALLABLE inline Matrix33 operator*(const Matrix33& a, const Matrix33& b) { return Multiply(a, b); }
CUDA_CALLABLE inline Matrix33 operator+(const Matrix33& a, const Matrix33& b) { return Add(a, b); }
CUDA_CALLABLE inline Matrix33 operator-(const Matrix33& a, const Matrix33& b) { return Add(a, -1.0f*b); }
CUDA_CALLABLE inline Matrix33& operator+=(Matrix33& a, const Matrix33& b) { a = a+b; return a; }
CUDA_CALLABLE inline Matrix33& operator-=(Matrix33& a, const Matrix33& b) { a = a-b; return a; }
CUDA_CALLABLE inline Matrix33& operator*=(Matrix33& a, float s) { a.cols[0]*=s; a.cols[1]*=s; a.cols[2]*=s;  return a; }

template <typename T>
CUDA_CALLABLE inline XQuat<T>::XQuat(const Matrix33& m)
{
	float tr = m(0, 0) + m(1, 1) + m(2, 2), h;
	if(tr >= 0)
	{
		h = sqrtf(tr + 1);
		w = 0.5f * h;
		h = 0.5f / h;

		x = (m(2, 1) - m(1, 2)) * h;
		y = (m(0, 2) - m(2, 0)) * h;
		z = (m(1, 0) - m(0, 1)) * h;
	}
	else
	{
		unsigned int i = 0;
		if(m(1, 1) > m(0, 0))
			i = 1;
		if(m(2, 2) > m(i, i))
			i = 2;
		switch(i)
		{
		case 0:
			h = sqrtf((m(0, 0) - (m(1, 1) + m(2, 2))) + 1);
			x = 0.5f * h;
			h = 0.5f / h;

			y = (m(0, 1) + m(1, 0)) * h;
			z = (m(2, 0) + m(0, 2)) * h;
			w = (m(2, 1) - m(1, 2)) * h;
			break;
		case 1:
			h = sqrtf((m(1, 1) - (m(2, 2) + m(0, 0))) + 1);
			y = 0.5f * h;
			h = 0.5f / h;

			z = (m(1, 2) + m(2, 1)) * h;
			x = (m(0, 1) + m(1, 0)) * h;
			w = (m(0, 2) - m(2, 0)) * h;
			break;
		case 2:
			h = sqrtf((m(2, 2) - (m(0, 0) + m(1, 1))) + 1);
			z = 0.5f * h;
			h = 0.5f / h;

			x = (m(2, 0) + m(0, 2)) * h;
			y = (m(1, 2) + m(2, 1)) * h;
			w = (m(1, 0) - m(0, 1)) * h;
			break;
		default: // Make compiler happy
			x = y = z = w = 0;
			break;
		}
	}
}

