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

#include <ostream>

#include "vec4.h"

class Point3
{
public:

	Point3() : x(0), y(0), z(0) {}
	Point3(float a) : x(a), y(a), z(a) {}
	Point3(const float* p) : x(p[0]), y(p[1]), z(p[2]) {}
	Point3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) 
	{
		Validate();	
	}	

	explicit Point3(const Vec3& v) : x(v.x), y(v.y), z(v.z) {}

	operator float* () { return &x; }
	operator const float* () const { return &x; };
	operator Vec4 () const { return Vec4(x, y, z, 1.0f); }

	void Set(float x_, float y_, float z_) { Validate(); x = x_; y = y_; z = z_;}

	Point3 operator * (float scale) const { Point3 r(*this); r *= scale; Validate(); return r; }
	Point3 operator / (float scale) const { Point3 r(*this); r /= scale; Validate(); return r; }
	Point3 operator + (const Vec3& v) const { Point3 r(*this); r += v; Validate(); return r; }
	Point3 operator - (const Vec3& v) const { Point3 r(*this); r -= v; Validate(); return r; }

	Point3& operator *=(float scale) {x *= scale; y *= scale; z*= scale; Validate(); return *this;}
	Point3& operator /=(float scale) {float s(1.0f/scale); x *= s; y *= s; z *= s; Validate(); return *this;}
	Point3& operator +=(const Vec3& v) {x += v.x; y += v.y; z += v.z; Validate(); return *this;}
	Point3& operator -=(const Vec3& v) {x -= v.x; y -= v.y; z -= v.z; Validate(); return *this;}

	bool operator != (const Point3& v) const { return (x != v.x || y != v.y || z != v.z); }

	// negate
	Point3 operator -() const { Validate(); return Point3(-x, -y, -z); }

	float x,y,z;

	void Validate() const
	{
	}
};

// lhs scalar scale
inline Point3 operator *(float lhs, const Point3& rhs)
{
	Point3 r(rhs);
	r *= lhs;
	return r;
}

inline Vec3 operator-(const Point3& lhs, const Point3& rhs)
{
	return Vec3(lhs.x-rhs.x,lhs.y-rhs.y, lhs.z-rhs.z);
}

inline Point3 operator+(const Point3& lhs, const Point3& rhs)
{
	return Point3(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z);
}

inline bool operator==(const Point3& lhs, const Point3& rhs)
{
	return (lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
}

inline std::ostream& operator << (std::ostream& s, const Point3& p)
{
	return s << p.x << "," << p.y << "," << p.z;
}

// component wise min max functions
inline Point3 Max(const Point3& a, const Point3& b)
{
    return Point3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

inline  Point3 Min(const Point3& a, const Point3& b)
{
    return Point3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

