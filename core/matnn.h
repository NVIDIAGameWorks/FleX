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

template <int m, int n, typename T=double>
class XMatrix
{
public:

	XMatrix()
	{
		memset(data, 0, sizeof(*this));
	}

	XMatrix(const XMatrix<m, n>& a)
	{
		memcpy(data, a.data, sizeof(*this));
	}

	template <typename OtherT>
	XMatrix(const OtherT* ptr)
	{
		for (int j=0; j < n; ++j)
			for (int i=0; i < m; ++i)
				data[j][i] = *(ptr++);
	}		

	const XMatrix<m,n>& operator=(const XMatrix<m,n>& a)
	{	
		memcpy(data, a.data, sizeof(*this));
		return *this;
	}

	template <typename OtherT>
	void SetCol(int j, const XMatrix<m, 1, OtherT>& c)
	{
		for (int i=0; i < m; ++i)
			data[j][i] = c(i, 0);
	}

	template <typename OtherT>
	void SetRow(int i, const XMatrix<1, n, OtherT>& r)
	{
		for (int j=0; j < m; ++j)
			data[j][i] = r(0, j);
	}

	T& operator()(int row, int col) { return data[col][row]; }
	const T& operator()(int row, int col) const { return data[col][row]; }


	void SetIdentity()
	{
		for (int i=0; i < m; ++i)
		{
			for (int j=0; j < n; ++j)
			{
				if (i == j)
					data[i][j] = 1.0;
				else
					data[i][j] = 0.0;
			}
		}
	}

	// column major storage
	T data[n][m];
};

template <int m, int n, typename T>
XMatrix<m, n, T> operator-(const XMatrix<m, n, T>& lhs, const XMatrix<m, n, T>& rhs)
{
	XMatrix<m, n> d;

	for (int i=0; i < m; ++i)
		for (int j=0; j < n; ++j)
			d(i, j) = lhs(i,j)-rhs(i,j);
				

	return d;
}	

template <int m, int n, typename T>
XMatrix<m, n, T> operator+(const XMatrix<m, n, T>& lhs, const XMatrix<m, n, T>& rhs)
{
	XMatrix<m, n> d;

	for (int i=0; i < m; ++i)
		for (int j=0; j < n; ++j)
			d(i, j) = lhs(i,j)+rhs(i,j);
				

	return d;
}	


template <int m, int n, int o, typename T>
XMatrix<m, o> Multiply(const XMatrix<m, n, T>& lhs, const XMatrix<n, o, T>& rhs)
{
	XMatrix<m, o> ret;

	for (int i=0; i < m; ++i)
	{
		for (int j=0; j < o; ++j)
		{
			T sum = 0.0f;

			for (int k=0; k < n; ++k)
			{
				sum += lhs(i, k)*rhs(k, j);
			}

			ret(i, j) = sum;
		}
	}

	return ret;
}


template <int m, int n>
XMatrix<n, m> Transpose(const XMatrix<m, n>& a)
{
	XMatrix<n, m> ret;
	
	for (int i=0; i < m; ++i)
	{
		for (int j=0; j < n; ++j)
		{
			ret(j, i) = a(i, j);
		}
	}
	
	return ret;
}



// matrix to swap row i and j when multiplied on the right
template <int n>
XMatrix<n,n> Permutation(int i, int j)
{
	XMatrix<n, n> m;
	m.SetIdentity();

	m(i, i) = 0.0;
	m(i, j) = 1.0;

	m(j, j) = 0.0;
	m(j, i) = 1.0;

	return m;
}

template <int m, int n>
void PrintMatrix(const char* name, XMatrix<m, n> a)
{
	printf("%s = [\n", name);

	for (int i=0; i < m; ++i)
	{
		printf ("[ ");

		for (int j=0; j < n; ++j)
		{
			printf("% .4f", float(a(i, j)));
			
			if (j < n-1)
				printf(" ");
		}

		printf(" ]\n");
	}

	printf("]\n");
}

template <int n, typename T>
XMatrix<n, n, T> LU(const XMatrix<n ,n, T>& m, XMatrix<n,n, T>& L)
{
	XMatrix<n,n> U = m;
	L.SetIdentity();

	// for each row
	for (int j=0; j < n; ++j)
	{
		XMatrix<n,n> Li, LiInv;
		Li.SetIdentity();
		LiInv.SetIdentity();

		T pivot = U(j, j);

		if (pivot == 0.0)
			return U;

		assert(pivot != 0.0);

		// zero our all entries below pivot
		for (int i=j+1; i < n; ++i)
		{
			T l = -U(i, j)/pivot;
			
			Li(i,j) = l;	

			// create inverse of L1..Ln as we go (this is L)
			L(i,j) = -l;
		}

		U = Multiply(Li, U);
	}

	return U;
}

template <int m, typename T>
XMatrix<m, 1, T> Solve(const XMatrix<m, m, T>& L, const XMatrix<m, m, T>& U, const XMatrix<m, 1, T>& b)
{
	XMatrix<m, 1> y;
	XMatrix<m, 1> x;

	// Ly = b (forward substitution)
	for (int i=0; i < m; ++i)
	{
		T sum = 0.0;

		for (int j=0; j < i; ++j)
		{
			sum += y(j, 0)*L(i, j);	
		}	

		assert(L(i,i) != 0.0);

		y(i, 0) = (b(i,0) - sum) / L(i, i);
	}	
	
	// Ux = y (back substitution)
	for (int i=m-1; i >= 0; --i)
	{
		T sum = 0.0;
		
		for (int j=i+1; j < m; ++j)
		{
			sum += x(j, 0)*U(i, j);
		}	

		assert(U(i,i) != 0.0);

		x(i, 0) = (y(i, 0) - sum) / U(i,i);
	}

	return x;
}

template <int n, typename T>
T Determinant(const XMatrix<n, n, T>& A, XMatrix<n, n, T>& L, XMatrix<n, n, T>& U)
{
	U = LU(A, L); 

	// determinant is the product of diagonal entries of U (assume L has 1s on diagonal)
	T det = 1.0;	
	for (int i=0; i < n; ++i)
		det *= U(i, i);

	return det;	
}

template <int n, typename T>
XMatrix<n, n, T> Inverse(const XMatrix<n, n, T>& A, T& det)
{
	XMatrix<n,n> L, U;
	det = Determinant(A, L, U);
	
	XMatrix<n,n> Inv;

	if (det != 0.0f)
	{
		for (int i=0; i < n; ++i)
		{
			// solve for each column of the identity matrix
			XMatrix<n, 1> I;
			I(i, 0) = 1.0;

			XMatrix<n, 1> x = Solve(L, U, I);

			Inv.SetCol(i, x);
		}
	}

	return Inv;
}

template <int m, int n, typename T>
T FrobeniusNorm(const XMatrix<m, n, T>& A)
{
	T sum = 0.0;
	for (int i=0; i < m; ++i)
		for (int j=0; j < n; ++j)
			sum += A(i,j)*A(i,j);

	return sqrt(sum);
}
