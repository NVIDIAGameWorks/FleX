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

#include "perlin.h"

#include <cmath>

namespace Perlin
{

	//	types.
typedef int int32_t;
typedef float real64;

//
//	Fast math trickery....
//
#define _doublemagicroundeps	      (.5-1.4e-11)				//	almost .5f = .5f - 1e^(number of exp bit)
#define _doublemagic			double (6755399441055744.0)		//	2^52 * 1.5,  uses limited precision to floor

//! TODO: not sure if this is endian safe

//	fast floor float->int conversion routine.
inline int32_t Floor2Int(real64 val) 
{
#if 0
	val -= _doublemagicroundeps;
	val += _doublemagic;
	return ((int32_t*)&val)[0];
#else
	return (int32_t)floorf(val);
#endif
}



inline real64 Lerp(real64 t, real64 v1, real64 v2) 
{
	return v1 + t*(v2 - v1);
}


//	like cubic fade but has both first-order and second-order derivatives of 0.0 at [0.0,1.0]
inline real64 PerlinFade( real64 val ) {			
	real64 val3 = val*val*val;
	real64 val4 = val3*val;
	return 6.0f*val4*val - 15.0f*val4 + 10.0f*val3;
}



//
//	Perlin Noise Stuff
//
//	Implementation of Ken Perlins 2002 "Improved Perlin Noise".
//	Mostly taken from pbrt implementation in "Physically Based Rendering" by Matt Pharr and Greg Humpreys
//	which was inturn taken from Ken Perlins example Java code from [http://mrl.nyu.edu/~perlin/noise/]
//

//
//	Perlin Noise Permutation Data
//
#define NOISE_PERM_SIZE 256
static unsigned char NoisePerm[2 * NOISE_PERM_SIZE] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96,
		53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
		// Rest of noise permutation table
		8, 99, 37, 240, 21, 10, 23,
		190,  6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168,  68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54,  65, 25, 63, 161,  1, 216, 80, 73, 209, 76, 132, 187, 208,  89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186,  3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152,  2, 44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172, 9,
		129, 22, 39, 253,  19, 98, 108, 110, 79, 113, 224, 232, 178, 185,  112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,  81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214,  31, 181, 199, 106, 157, 184,  84, 204, 176, 115, 121, 50, 45, 127,  4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
		151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190,  6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168,  68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54,  65, 25, 63, 161,  1, 216, 80, 73, 209, 76, 132, 187, 208,  89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186,  3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152,  2, 44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172, 9,
		129, 22, 39, 253,  19, 98, 108, 110, 79, 113, 224, 232, 178, 185,  112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,  81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214,  31, 181, 199, 106, 157, 184,  84, 204, 176, 115, 121, 50, 45, 127,  4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

//	1d gradient function.   
static inline real64 Grad1d(int32_t x, real64 dx) 
{
	int32_t h = NoisePerm[x];
	h &= 3;
	return ((h&1) ? -dx : dx);
}

//	2d gradient function.   
static inline real64 Grad2d(int32_t x, int32_t y, real64 dx, real64 dy) 
{
	int32_t h = NoisePerm[NoisePerm[x]+y];
	h &= 3;
	return ((h&1) ? -dx : dx) + ((h&2) ? -dy : dy);
}
//	3d gradient function.
static inline real64 Grad3d(int32_t x, int32_t y, int32_t z, real64 dx, real64 dy, real64 dz) 
{
	int32_t h = NoisePerm[NoisePerm[NoisePerm[x]+y]+z];
	h &= 15;

	//	Ken Perlins original impl
	real64 u = h<8 ? dx : dy;
	real64 v = h<4 ? dy : (h==12 || h==14) ? dx : dz;
	return ((h&1) ? -u : u) + ((h&2) ? -v : v);

	//	(will this run faster without having all those branches?)
	/*
	//	or how about just this....
	static const real64 GRADS[16][3] = {	{1.0, 1.0, 0.0}, {-1.0, 1.0, 0.0}, {1.0, -1.0, 0.0}, {-1.0, -1.0, 0.0},		// the 12 grads
	{1.0, 0.0, 1.0}, {-1.0, 0.0, 1.0}, {1.0, 0.0, -1.0}, {-1.0, 0.0, -1.0},
	{0.0, 1.0, 1.0}, {0.0, -1.0, 1.0}, {0.0, 1.0, -1.0}, {0.0, -1.0, -1.0},
	{-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, -1.0, 1.0}, {0.0, -1.0, -1.0} };	// 8 more "padding" items to make this array of size 16.
	return GRADS[h][0]*dx + GRADS[h][1]*dy + GRADS[h][2]*dz;
	*/
}

//	4d version from java
/*
static double grad(int hash, double x, double y, double z, double w) {
int h = hash & 31; // CONVERT LO 5 BITS OF HASH TO 32 GRAD DIRECTIONS.
double a=y,b=z,c=w;            // X,Y,Z
switch (h >> 3) {          // OR, DEPENDING ON HIGH ORDER 2 BITS:
case 1: a=w;b=x;c=y;break;     // W,X,Y
case 2: a=z;b=w;c=x;break;     // Z,W,X
case 3: a=y;b=z;c=w;break;     // Y,Z,W
}
return ((h&4)==0 ? -a:a) + ((h&2)==0 ? -b:b) + ((h&1)==0 ? -c:c);
}
*/

static real64 PerlinNoise3DFunctionPeriodic(real64 x, real64 y, real64 z, int px, int py, int pz) 
{
	// Compute noise cell coordinates and offsets
	int32_t ix = Floor2Int(x);
	int32_t iy = Floor2Int(y);
	int32_t iz = Floor2Int(z);
	real64 dx = x - ix, dy = y - iy, dz = z - iz;

	int32_t ix0 = (ix%px) & (NOISE_PERM_SIZE-1);
	int32_t iy0 = (iy%py) & (NOISE_PERM_SIZE-1);
	int32_t iz0 = (iz%pz) & (NOISE_PERM_SIZE-1);

	int32_t ix1 = ((ix+1)%px) & (NOISE_PERM_SIZE-1);
	int32_t iy1 = ((iy+1)%py) & (NOISE_PERM_SIZE-1);
	int32_t iz1 = ((iz+1)%pz) & (NOISE_PERM_SIZE-1);

	real64 w000 = Grad3d(ix0, iy0, iz0,   dx,   dy,   dz);
	real64 w100 = Grad3d(ix1, iy0, iz0,   dx-1, dy,   dz);
	real64 w010 = Grad3d(ix0, iy1, iz0,   dx,   dy-1, dz);
	real64 w110 = Grad3d(ix1, iy1, iz0,   dx-1, dy-1, dz);
	real64 w001 = Grad3d(ix0, iy0, iz1, dx,   dy,   dz-1);
	real64 w101 = Grad3d(ix1, iy0, iz1, dx-1, dy,   dz-1);
	real64 w011 = Grad3d(ix0, iy1, iz1, dx,   dy-1, dz-1);
	real64 w111 = Grad3d(ix1, iy1, iz1, dx-1, dy-1, dz-1);
	// Compute trilinear interpolation of weights
	real64 wx = PerlinFade(dx);
	real64 wy = PerlinFade(dy);
	real64 wz = PerlinFade(dz);
	real64 x00 = Lerp(wx, w000, w100);
	real64 x10 = Lerp(wx, w010, w110);
	real64 x01 = Lerp(wx, w001, w101);
	real64 x11 = Lerp(wx, w011, w111);
	real64 y0 = Lerp(wy, x00, x10);
	real64 y1 = Lerp(wy, x01, x11);
	return Lerp(wz, y0, y1);
}

static real64 PerlinNoise3DFunction(real64 x, real64 y, real64 z) 
{
	// Compute noise cell coordinates and offsets
	int32_t ix = Floor2Int(x);
	int32_t iy = Floor2Int(y);
	int32_t iz = Floor2Int(z);
	real64 dx = x - ix, dy = y - iy, dz = z - iz;
	// Compute gradient weights
	ix &= (NOISE_PERM_SIZE-1);
	iy &= (NOISE_PERM_SIZE-1);
	iz &= (NOISE_PERM_SIZE-1);
	real64 w000 = Grad3d(ix,   iy,   iz,   dx,   dy,   dz);
	real64 w100 = Grad3d(ix+1, iy,   iz,   dx-1, dy,   dz);
	real64 w010 = Grad3d(ix,   iy+1, iz,   dx,   dy-1, dz);
	real64 w110 = Grad3d(ix+1, iy+1, iz,   dx-1, dy-1, dz);
	real64 w001 = Grad3d(ix,   iy,   iz+1, dx,   dy,   dz-1);
	real64 w101 = Grad3d(ix+1, iy,   iz+1, dx-1, dy,   dz-1);
	real64 w011 = Grad3d(ix,   iy+1, iz+1, dx,   dy-1, dz-1);
	real64 w111 = Grad3d(ix+1, iy+1, iz+1, dx-1, dy-1, dz-1);
	// Compute trilinear interpolation of weights
	real64 wx = PerlinFade(dx);
	real64 wy = PerlinFade(dy);
	real64 wz = PerlinFade(dz);
	real64 x00 = Lerp(wx, w000, w100);
	real64 x10 = Lerp(wx, w010, w110);
	real64 x01 = Lerp(wx, w001, w101);
	real64 x11 = Lerp(wx, w011, w111);
	real64 y0 = Lerp(wy, x00, x10);
	real64 y1 = Lerp(wy, x01, x11);
	return Lerp(wz, y0, y1);
}


static real64 PerlinNoise2DFunction(real64 x, real64 y) 
{
	// Compute noise cell coordinates and offsets
	int32_t ix = Floor2Int(x);
	int32_t iy = Floor2Int(y);
	real64 dx = x - ix, dy = y - iy;
	// Compute gradient weights
	ix &= (NOISE_PERM_SIZE-1);
	iy &= (NOISE_PERM_SIZE-1);
	real64 w00 = Grad2d(ix,   iy,   dx,		 dy);
	real64 w10 = Grad2d(ix+1, iy,   dx-1.0f, dy);
	real64 w01 = Grad2d(ix,   iy+1, dx,		 dy-1.0f);
	real64 w11 = Grad2d(ix+1, iy+1, dx-1.0f, dy-1.0f);
	// Compute bilinear interpolation of weights
	real64 wx = PerlinFade(dx);
	real64 wy = PerlinFade(dy);
	real64 x0 = Lerp(wx, w00, w10);
	real64 x1 = Lerp(wx, w01, w11);
	return Lerp(wy, x0, x1);
}


static real64 PerlinNoise1DFunction(real64 x) 
{
	// Compute noise cell coordinates and offsets
	int32_t ix = Floor2Int(x);
	real64 dx = x - ix;
	// Compute gradient weights
	ix &= (NOISE_PERM_SIZE-1);
	real64 w00 = Grad1d(ix,   dx);
	real64 w10 = Grad1d(ix+1, dx-1.0f);
	// Compute bilinear interpolation of weights
	real64 wx = PerlinFade(dx);
	real64 x0 = Lerp(wx, w00, w10);
	
	return x0;
}
}

//------------------------------------------------
//! Public interfaces

float Perlin1D(float x, int octaves, float persistence)
{
	float r = 0.0f;
	float a = 1.0f;
	int freq = 1; 

	for (int i=0; i < octaves; i++)
	{
		float fx = x*freq;

		r += Perlin::PerlinNoise1DFunction(fx)*a;

		//! update amplitude
		a *= persistence;
		freq = 2 << i;
	}

	return r;
}


float Perlin2D(float x, float y, int octaves, float persistence)
{
	float r = 0.0f;
	float a = 1.0f;
	int freq = 1; 

	for (int i=0; i < octaves; i++)
	{
		float fx = x*freq;
		float fy = y*freq;

		r += Perlin::PerlinNoise2DFunction(fx, fy)*a;

		//! update amplitude
		a *= persistence;
		freq = 2 << i;
	}

	return r;
}


float Perlin3D(float x, float y, float z, int octaves, float persistence)
{
	float r = 0.0f;
	float a = 1.0f;
	int freq = 1; 

	for (int i=0; i < octaves; i++)
	{
		float fx = x*freq;
		float fy = y*freq;
		float fz = z*freq;

		r += Perlin::PerlinNoise3DFunction(fx, fy, fz)*a;

		//! update amplitude
		a *= persistence;
		freq = 2 << i;
	}

	return r;
}

float Perlin3DPeriodic(float x, float y, float z, int px, int py, int pz, int octaves, float persistence)
{
	float r = 0.0f;
	float a = 1.0f;
	int freq = 1; 

	for (int i=0; i < octaves; i++)
	{
		float fx = x*freq;
		float fy = y*freq;
		float fz = z*freq;

		r += Perlin::PerlinNoise3DFunctionPeriodic(fx, fy, fz, px, py, pz)*a;

		//! update amplitude
		a *= persistence;
		freq = 2 << i;
	}

	return r;
}
