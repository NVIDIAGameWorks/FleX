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

#include "maths.h"

struct Matrix22
{
	CUDA_CALLABLE Matrix22() {}
	CUDA_CALLABLE Matrix22(float a, float b, float c, float d)
	{
		cols[0] = Vec2(a, c);
		cols[1] = Vec2(b, d);
	}

	CUDA_CALLABLE Matrix22(const Vec2& c1, const Vec2& c2)
	{
		cols[0] = c1;
		cols[1] = c2;
	}

	CUDA_CALLABLE float operator()(int i, int j) const { return static_cast<const float*>(cols[j])[i]; }
	CUDA_CALLABLE float& operator()(int i, int j) { return static_cast<float*>(cols[j])[i]; }

	Vec2 cols[2];

	static inline Matrix22 Identity() { static const Matrix22 sIdentity(Vec2(1.0f, 0.0f), Vec2(0.0f, 1.0f)); return sIdentity; }
};

CUDA_CALLABLE inline Matrix22 Multiply(float s, const Matrix22& m)
{
	Matrix22 r = m;
	r.cols[0] *= s;
	r.cols[1] *= s;	
	return r;
}

CUDA_CALLABLE inline Matrix22 Multiply(const Matrix22& a, const Matrix22& b)
{
	Matrix22 r;
	r.cols[0] = a.cols[0]*b.cols[0].x + a.cols[1]*b.cols[0].y;
	r.cols[1] = a.cols[0]*b.cols[1].x + a.cols[1]*b.cols[1].y;
	return r;
}

CUDA_CALLABLE inline Matrix22 Add(const Matrix22& a, const Matrix22& b)
{
	return Matrix22(a.cols[0]+b.cols[0], a.cols[1]+b.cols[1]);
}

CUDA_CALLABLE inline Vec2 Multiply(const Matrix22& a, const Vec2& x)
{
	return a.cols[0]*x.x + a.cols[1]*x.y;
}

CUDA_CALLABLE inline Matrix22 operator*(float s, const Matrix22& a) { return Multiply(s, a); }
CUDA_CALLABLE inline Matrix22 operator*(const Matrix22& a, float s) { return Multiply(s, a); }
CUDA_CALLABLE inline Matrix22 operator*(const Matrix22& a, const Matrix22& b) { return Multiply(a, b); }
CUDA_CALLABLE inline Matrix22 operator+(const Matrix22& a, const Matrix22& b) { return Add(a, b); }
CUDA_CALLABLE inline Matrix22 operator-(const Matrix22& a, const Matrix22& b) { return Add(a, -1.0f*b); }
CUDA_CALLABLE inline Matrix22& operator+=(Matrix22& a, const Matrix22& b) { a = a+b; return a; }
CUDA_CALLABLE inline Matrix22& operator-=(Matrix22& a, const Matrix22& b) { a = a-b; return a; }
CUDA_CALLABLE inline Matrix22& operator*=(Matrix22& a, float s) { a = a*s; return a; }
CUDA_CALLABLE inline Vec2 operator*(const Matrix22& a, const Vec2& x) { return Multiply(a, x); }

CUDA_CALLABLE inline float Determinant(const Matrix22& m)
{
	return m(0,0)*m(1,1)-m(1,0)*m(0,1);
}

CUDA_CALLABLE inline Matrix22 Inverse(const Matrix22& m, float& det)
{
	det = Determinant(m); 

	if (fabsf(det) > FLT_EPSILON)
	{
		Matrix22 inv;
		inv(0,0) =  m(1,1);
		inv(1,1) =  m(0,0);
		inv(0,1) = -m(0,1);
		inv(1,0) = -m(1,0);

		return Multiply(1.0f/det, inv);	
	}
	else
	{
		det = 0.0f;
		return m;
	}
}

CUDA_CALLABLE inline Matrix22 Transpose(const Matrix22& a)
{
	Matrix22 r;
	r(0,0) = a(0,0);
	r(0,1) = a(1,0);
	r(1,0) = a(0,1);
	r(1,1) = a(1,1);
	return r;
}

CUDA_CALLABLE inline float Trace(const Matrix22& a)
{
	return a(0,0)+a(1,1);
}

CUDA_CALLABLE inline Matrix22 RotationMatrix(float theta)
{
	return Matrix22(Vec2(cosf(theta), sinf(theta)), Vec2(-sinf(theta), cosf(theta)));
}

// outer product of a and b, b is considered a row vector
CUDA_CALLABLE inline Matrix22 Outer(const Vec2& a, const Vec2& b)
{
	return Matrix22(a*b.x, a*b.y);
}

CUDA_CALLABLE inline Matrix22 QRDecomposition(const Matrix22& m)
{
	Vec2 a = Normalize(m.cols[0]);
	Matrix22 q(a, PerpCCW(a)); 

	return q;
}

CUDA_CALLABLE inline Matrix22 PolarDecomposition(const Matrix22& m)
{
	/*
	//iterative method
	 
	float det;
	Matrix22 q = m;

	for (int i=0; i < 4; ++i)
	{
		q = 0.5f*(q + Inverse(Transpose(q), det));
	}
	*/	

	Matrix22 q = m + Matrix22(m(1,1), -m(1,0), -m(0,1), m(0,0));

	float s = Length(q.cols[0]);
	q.cols[0] /= s;
	q.cols[1] /= s;

	return q;
}


