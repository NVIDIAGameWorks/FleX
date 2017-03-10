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

// stores column vectors in column major order
template <typename T>
class XMatrix44
{
public:

	CUDA_CALLABLE XMatrix44() { memset(columns, 0, sizeof(columns)); }
	CUDA_CALLABLE XMatrix44(const T* d) { assert(d); memcpy(columns, d, sizeof(*this)); }
	CUDA_CALLABLE XMatrix44(T c11, T c21, T c31, T c41,
				 T c12, T c22, T c32, T c42,
			    T c13, T c23, T c33, T c43,
				 T c14, T c24, T c34, T c44)
	{
		columns[0][0] = c11;
		columns[0][1] = c21;
		columns[0][2] = c31;
		columns[0][3] = c41;

		columns[1][0] = c12;
		columns[1][1] = c22;
		columns[1][2] = c32;
		columns[1][3] = c42;

		columns[2][0] = c13;
		columns[2][1] = c23;
		columns[2][2] = c33;
		columns[2][3] = c43;

		columns[3][0] = c14;
		columns[3][1] = c24;
		columns[3][2] = c34;
		columns[3][3] = c44;
	}

	CUDA_CALLABLE XMatrix44(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4)
	{
		columns[0][0] = c1.x;
		columns[0][1] = c1.y;
		columns[0][2] = c1.z;
		columns[0][3] = c1.w;

		columns[1][0] = c2.x;
		columns[1][1] = c2.y;
		columns[1][2] = c2.z;
		columns[1][3] = c2.w;

		columns[2][0] = c3.x;
		columns[2][1] = c3.y;
		columns[2][2] = c3.z;
		columns[2][3] = c3.w;

		columns[3][0] = c4.x;
		columns[3][1] = c4.y;
		columns[3][2] = c4.z;
		columns[3][3] = c4.w;
	}

	CUDA_CALLABLE operator T* () { return &columns[0][0]; }
	CUDA_CALLABLE operator const T* () const { return &columns[0][0]; }

	// right multiply
	CUDA_CALLABLE XMatrix44<T> operator * (const XMatrix44<T>& rhs) const
	{
		XMatrix44<T> r;
		MatrixMultiply(*this, rhs, r);
		return r;
	}

	// right multiply
	CUDA_CALLABLE XMatrix44<T>& operator *= (const XMatrix44<T>& rhs)
	{
		XMatrix44<T> r;
		MatrixMultiply(*this, rhs, r);
		*this = r;

		return *this;
	}

	// scalar multiplication
	CUDA_CALLABLE XMatrix44<T>& operator *= (const T& s)
	{
		for (int c=0; c < 4; ++c)
		{
			for (int r=0; r < 4; ++r)
			{
				columns[c][r] *= s;
			}
		}

		return *this;
	}

	CUDA_CALLABLE void MatrixMultiply(const T* __restrict lhs, const T* __restrict rhs, T* __restrict result) const
	{
		assert(lhs != rhs);
		assert(lhs != result);
		assert(rhs != result);
		
		for (int i=0; i < 4; ++i)
		{
			for (int j=0; j < 4; ++j)
			{
				result[j*4+i]  = rhs[j*4+0]*lhs[i+0]; 
				result[j*4+i] += rhs[j*4+1]*lhs[i+4];
				result[j*4+i] += rhs[j*4+2]*lhs[i+8];
				result[j*4+i] += rhs[j*4+3]*lhs[i+12];
			}
		}
	}	

	CUDA_CALLABLE void SetCol(int index, const Vec4& c)
	{
		columns[index][0] = c.x;
		columns[index][1] = c.y;
		columns[index][2] = c.z;
		columns[index][3] = c.w;
	}

	// convenience overloads
	CUDA_CALLABLE void SetAxis(uint32_t index, const XVector3<T>& a)
	{
		columns[index][0] = a.x;
		columns[index][1] = a.y;
		columns[index][2] = a.z;
		columns[index][3] = 0.0f;
	}

	CUDA_CALLABLE void SetTranslation(const Point3& p)
	{
		columns[3][0] = p.x;	
		columns[3][1] = p.y;
		columns[3][2] = p.z;
		columns[3][3] = 1.0f;
	}

	CUDA_CALLABLE const Vec3& GetAxis(int i) const { return *reinterpret_cast<const Vec3*>(&columns[i]); }
	CUDA_CALLABLE const Vec4& GetCol(int i) const { return *reinterpret_cast<const Vec4*>(&columns[i]); }
	CUDA_CALLABLE const Point3& GetTranslation() const { return *reinterpret_cast<const Point3*>(&columns[3]); }

	CUDA_CALLABLE Vec4 GetRow(int i) const { return Vec4(columns[0][i], columns[1][i], columns[2][i], columns[3][i]); }

	float columns[4][4];

	static XMatrix44<T> kIdentity;

};

// right multiply a point assumes w of 1
template <typename T>
CUDA_CALLABLE Point3 Multiply(const XMatrix44<T>& mat, const Point3& v)
{
	Point3 r;
	r.x = v.x*mat[0] + v.y*mat[4] + v.z*mat[8] + mat[12];
	r.y = v.x*mat[1] + v.y*mat[5] + v.z*mat[9] + mat[13];
	r.z = v.x*mat[2] + v.y*mat[6] + v.z*mat[10] + mat[14];

	return r;
}

// right multiply a vector3 assumes a w of 0
template <typename T>
CUDA_CALLABLE XVector3<T> Multiply(const XMatrix44<T>& mat, const XVector3<T>& v)
{
	XVector3<T> r;
	r.x = v.x*mat[0] + v.y*mat[4] + v.z*mat[8];
	r.y = v.x*mat[1] + v.y*mat[5] + v.z*mat[9];
	r.z = v.x*mat[2] + v.y*mat[6] + v.z*mat[10];

	return r;
}

// right multiply a vector4
template <typename T>
CUDA_CALLABLE XVector4<T> Multiply(const XMatrix44<T>& mat, const XVector4<T>& v)
{
	XVector4<T> r;
	r.x = v.x*mat[0] + v.y*mat[4] + v.z*mat[8] + v.w*mat[12];
	r.y = v.x*mat[1] + v.y*mat[5] + v.z*mat[9] + v.w*mat[13];
	r.z = v.x*mat[2] + v.y*mat[6] + v.z*mat[10] + v.w*mat[14];
	r.w = v.x*mat[3] + v.y*mat[7] + v.z*mat[11] + v.w*mat[15];

	return r;
}

template <typename T>
CUDA_CALLABLE Point3 operator*(const XMatrix44<T>& mat, const Point3& v)
{
	return Multiply(mat, v);
}

template <typename T>
CUDA_CALLABLE XVector4<T> operator*(const XMatrix44<T>& mat, const XVector4<T>& v)
{
	return Multiply(mat, v);
}

template <typename T>
CUDA_CALLABLE XVector3<T> operator*(const XMatrix44<T>& mat, const XVector3<T>& v)
{
	return Multiply(mat, v);
}

template<typename T>
CUDA_CALLABLE inline XMatrix44<T> Transpose(const XMatrix44<T>& m)
{
	XMatrix44<float> inv;

	// transpose
	for (uint32_t c=0; c < 4; ++c)
	{
		for (uint32_t r=0; r < 4; ++r)
		{
			inv.columns[c][r] = m.columns[r][c];
		}
	}

	return inv;
}

template <typename T>
CUDA_CALLABLE XMatrix44<T> AffineInverse(const XMatrix44<T>& m)
{
	XMatrix44<T> inv;
	
	// transpose upper 3x3
	for (int c=0; c < 3; ++c)
	{
		for (int r=0; r < 3; ++r)
		{
			inv.columns[c][r] = m.columns[r][c];
		}
	}
	
	// multiply -translation by upper 3x3 transpose
	inv.columns[3][0] = -Dot3(m.columns[3], m.columns[0]);
	inv.columns[3][1] = -Dot3(m.columns[3], m.columns[1]);
	inv.columns[3][2] = -Dot3(m.columns[3], m.columns[2]);
	inv.columns[3][3] = 1.0f;

	return inv;	
}

CUDA_CALLABLE inline XMatrix44<float> Outer(const Vec4& a, const Vec4& b)
{
	return XMatrix44<float>(a*b.x, a*b.y, a*b.z, a*b.w);
}

// convenience
typedef XMatrix44<float> Mat44;
typedef XMatrix44<float> Matrix44;

