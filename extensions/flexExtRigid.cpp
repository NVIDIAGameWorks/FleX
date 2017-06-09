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
// Copyright (c) 20132017 NVIDIA Corporation. All rights reserved.

#include "../include/NvFlexExt.h"

#include "../core/maths.h"
#include "../core/voxelize.h"
#include "../core/sdf.h"

#include <vector>

namespace
{

float SampleSDF(const float* sdf, int dim, int x, int y, int z)
{
	assert(x < dim && x >= 0);
	assert(y < dim && y >= 0);
	assert(z < dim && z >= 0);

	return sdf[z*dim*dim + y*dim + x];
}

// return normal of signed distance field
Vec3 SampleSDFGrad(const float* sdf, int dim, int x, int y, int z)
{
	int x0 = std::max(x-1, 0);
	int x1 = std::min(x+1, dim-1);

	int y0 = std::max(y-1, 0);
	int y1 = std::min(y+1, dim-1);

	int z0 = std::max(z-1, 0);
	int z1 = std::min(z+1, dim-1);

	float dx = (SampleSDF(sdf, dim, x1, y, z) - SampleSDF(sdf, dim, x0, y, z))*(dim*0.5f);
	float dy = (SampleSDF(sdf, dim, x, y1, z) - SampleSDF(sdf, dim, x, y0, z))*(dim*0.5f);
	float dz = (SampleSDF(sdf, dim, x, y, z1) - SampleSDF(sdf, dim, x, y, z0))*(dim*0.5f);

	return Vec3(dx, dy, dz);
}

} // anonymous namespace

NvFlexExtAsset* NvFlexExtCreateRigidFromMesh(const float* vertices, int numVertices, const int* indices, int numTriangleIndices, float spacing, float expand)
{
	// Switch to relative coordinates by computing the mean position of the vertices and subtracting the result from every vertex position
	// The increased precision will prevent ghost forces caused by inaccurate center of mass computations
	Vec3 meshOffset(0.0f);
	for (int i = 0; i < numVertices; i++)
	{
		meshOffset += ((Vec3*)vertices)[i];
	}
	meshOffset /= float(numVertices);

	Vec3* relativeVertices = new Vec3[numVertices];
	for (int i = 0; i < numVertices; i++)
	{
		relativeVertices[i] += ((Vec3*)vertices)[i] - meshOffset;
	}

	std::vector<Vec4> particles;
	std::vector<Vec4> normals;
	std::vector<int> phases;

	const Vec3* positions = relativeVertices;

	Vec3 meshLower(FLT_MAX), meshUpper(-FLT_MAX);
	for (int i=0; i < numVertices; ++i)
	{
		meshLower = Min(meshLower, positions[i]);
		meshUpper = Max(meshUpper, positions[i]);
	}

	Vec3 edges = meshUpper-meshLower;
	float maxEdge = std::max(std::max(edges.x, edges.y), edges.z);

	// tweak spacing to avoid edge cases for particles laying on the boundary
	// just covers the case where an edge is a whole multiple of the spacing.
	float spacingEps = spacing*(1.0f - 1e-4f);

	// make sure to have at least one particle in each dimension
	int dx, dy, dz;
	dx = spacing > edges.x ? 1 : int(edges.x/spacingEps);
	dy = spacing > edges.y ? 1 : int(edges.y/spacingEps);
	dz = spacing > edges.z ? 1 : int(edges.z/spacingEps);

	int maxDim = std::max(std::max(dx, dy), dz);

	// expand border by two voxels to ensure adequate sampling at edges
	meshLower -= 2.0f*Vec3(spacing);
	meshUpper += 2.0f*Vec3(spacing);
	maxDim += 4;

	// we shift the voxelization bounds so that the voxel centers
	// lie symmetrically to the center of the object. this reduces the 
	// chance of missing features, and also better aligns the particles
	// with the mesh
	Vec3 meshShift;
	meshShift.x = 0.5f * (spacing - (edges.x - (dx-1)*spacing));
	meshShift.y = 0.5f * (spacing - (edges.y - (dy-1)*spacing));
	meshShift.z = 0.5f * (spacing - (edges.z - (dz-1)*spacing));
	meshLower -= meshShift;

	// don't allow samplings with > 64 per-side	
	if (maxDim > 64)
		return NULL;

	std::vector<uint32_t> voxels(maxDim*maxDim*maxDim);

	Voxelize(relativeVertices, numVertices, indices, numTriangleIndices, maxDim, maxDim, maxDim, &voxels[0], meshLower, meshLower + Vec3(maxDim*spacing));

	delete[] relativeVertices;

	std::vector<float> sdf(maxDim*maxDim*maxDim);
	MakeSDF(&voxels[0], maxDim, maxDim, maxDim, &sdf[0]);

	Vec3 center;

	for (int x=0; x < maxDim; ++x)
	{
		for (int y=0; y < maxDim; ++y)
		{
			for (int z=0; z < maxDim; ++z)
			{
				const int index = z*maxDim*maxDim + y*maxDim + x;

				// if voxel is marked as occupied the add a particle
				if (voxels[index])
				{
					Vec3 position = meshLower + spacing*Vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f);

					// normalize the sdf value and transform to world scale
					Vec3 n = SafeNormalize(SampleSDFGrad(&sdf[0], maxDim, x, y, z));
					float d = sdf[index]*maxEdge;

					// move particles inside or outside shape
					position += n*expand;

					normals.push_back(Vec4(n, d));
					particles.push_back(Vec4(position.x, position.y, position.z, 1.0f));						
					phases.push_back(0);

					center += position;
				}
			}
		}
	}
	const int numParticles = int(particles.size());

	// Switch back to absolute coordinates by adding meshOffset to the center of mass and to each particle positions
	center /= float(numParticles);
	center += meshOffset;

	for (int i = 0; i < numParticles; i++) {
		particles[i] += Vec4(meshOffset, 0.0f);
	}

	NvFlexExtAsset* asset = new NvFlexExtAsset();
	memset(asset, 0, sizeof(*asset));

	if (numParticles)
	{
		asset->numParticles = numParticles;
		asset->maxParticles = numParticles;

		asset->particles = new float[numParticles*4];
		memcpy(asset->particles, &particles[0], sizeof(Vec4)*numParticles);

		asset->numShapes = 1;
		asset->numShapeIndices = numParticles;
		
		// for rigids we just reference all particles in the shape
		asset->shapeIndices = new int[numParticles];

		for (int i = 0; i < numParticles; ++i)
			asset->shapeIndices[i] = i;

		// store center of mass
		asset->shapeCenters = new float[4];
		asset->shapeCenters[0] = center.x;
		asset->shapeCenters[1] = center.y;
		asset->shapeCenters[2] = center.z;

		asset->shapePlasticThresholds = NULL;
		asset->shapePlasticCreeps = NULL;

		asset->shapeCoefficients = new float[1];
		asset->shapeCoefficients[0] = 1.0f;

		asset->shapeOffsets = new int[1];
		asset->shapeOffsets[0] = numParticles;
	}

	return asset;
}
