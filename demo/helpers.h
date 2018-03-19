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
// Copyright (c) 2013-2017 NVIDIA Corporation. All rights reserved.

#pragma once

#include <stdarg.h>

// disable some warnings
#if _WIN32
#pragma warning(disable: 4267)  // conversion from 'size_t' to 'int', possible loss of data
#endif

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
	int x0 = max(x-1, 0);
	int x1 = min(x+1, dim-1);

	int y0 = max(y-1, 0);
	int y1 = min(y+1, dim-1);

	int z0 = max(z-1, 0);
	int z1 = min(z+1, dim-1);

	float dx = (SampleSDF(sdf, dim, x1, y, z) - SampleSDF(sdf, dim, x0, y, z))*(dim*0.5f);
	float dy = (SampleSDF(sdf, dim, x, y1, z) - SampleSDF(sdf, dim, x, y0, z))*(dim*0.5f);
	float dz = (SampleSDF(sdf, dim, x, y, z1) - SampleSDF(sdf, dim, x, y, z0))*(dim*0.5f);

	return Vec3(dx, dy, dz);
}

void GetParticleBounds(Vec3& lower, Vec3& upper)
{
	lower = Vec3(FLT_MAX);
	upper = Vec3(-FLT_MAX);

	for (int i=0; i < g_buffers->positions.size(); ++i)
	{
		lower = Min(Vec3(g_buffers->positions[i]), lower);
		upper = Max(Vec3(g_buffers->positions[i]), upper);
	}
}


void CreateParticleGrid(Vec3 lower, int dimx, int dimy, int dimz, float radius, Vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, float jitter=0.005f)
{
	if (rigid && g_buffers->rigidIndices.empty())
		g_buffers->rigidOffsets.push_back(0);

	for (int x = 0; x < dimx; ++x)
	{
		for (int y = 0; y < dimy; ++y)
		{
			for (int z=0; z < dimz; ++z)
			{
				if (rigid)
					g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));

				Vec3 position = lower + Vec3(float(x), float(y), float(z))*radius + RandomUnitVector()*jitter;

				g_buffers->positions.push_back(Vec4(position.x, position.y, position.z, invMass));
				g_buffers->velocities.push_back(velocity);
				g_buffers->phases.push_back(phase);
			}
		}
	}

	if (rigid)
	{
		g_buffers->rigidCoefficients.push_back(rigidStiffness);
		g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));
	}
}

void CreateParticleSphere(Vec3 center, int dim, float radius, Vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, float jitter=0.005f)
{
	if (rigid && g_buffers->rigidIndices.empty())
			g_buffers->rigidOffsets.push_back(0);

	for (int x=0; x <= dim; ++x)
	{
		for (int y=0; y <= dim; ++y)
		{
			for (int z=0; z <= dim; ++z)
			{
				float sx = x - dim*0.5f;
				float sy = y - dim*0.5f;
				float sz = z - dim*0.5f;

				if (sx*sx + sy*sy + sz*sz <= float(dim*dim)*0.25f)
				{
					if (rigid)
						g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));

					Vec3 position = center + radius*Vec3(sx, sy, sz) + RandomUnitVector()*jitter;

					g_buffers->positions.push_back(Vec4(position.x, position.y, position.z, invMass));
					g_buffers->velocities.push_back(velocity);
					g_buffers->phases.push_back(phase);
				}
			}
		}
	}

	if (rigid)
	{
		g_buffers->rigidCoefficients.push_back(rigidStiffness);
		g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));
	}
}

void CreateSpring(int i, int j, float stiffness, float give=0.0f)
{
	g_buffers->springIndices.push_back(i);
	g_buffers->springIndices.push_back(j);
	g_buffers->springLengths.push_back((1.0f+give)*Length(Vec3(g_buffers->positions[i])-Vec3(g_buffers->positions[j])));
	g_buffers->springStiffness.push_back(stiffness);	
}


void CreateParticleShape(const Mesh* srcMesh, Vec3 lower, Vec3 scale, float rotation, float spacing, Vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, bool skin, float jitter=0.005f, Vec3 skinOffset=0.0f, float skinExpand=0.0f, Vec4 color=Vec4(0.0f), float springStiffness=0.0f)
{
	if (rigid && g_buffers->rigidIndices.empty())
		g_buffers->rigidOffsets.push_back(0);

	if (!srcMesh)
		return;

	// duplicate mesh
	Mesh mesh;
	mesh.AddMesh(*srcMesh);

	int startIndex = int(g_buffers->positions.size());

	{
		mesh.Transform(RotationMatrix(rotation, Vec3(0.0f, 1.0f, 0.0f)));

		Vec3 meshLower, meshUpper;
		mesh.GetBounds(meshLower, meshUpper);

		Vec3 edges = meshUpper-meshLower;
		float maxEdge = max(max(edges.x, edges.y), edges.z);

		// put mesh at the origin and scale to specified size
		Matrix44 xform = ScaleMatrix(scale/maxEdge)*TranslationMatrix(Point3(-meshLower));

		mesh.Transform(xform);
		mesh.GetBounds(meshLower, meshUpper);

		// recompute expanded edges
		edges = meshUpper-meshLower;
		maxEdge = max(max(edges.x, edges.y), edges.z);

		// tweak spacing to avoid edge cases for particles laying on the boundary
		// just covers the case where an edge is a whole multiple of the spacing.
		float spacingEps = spacing*(1.0f - 1e-4f);

		// make sure to have at least one particle in each dimension
		int dx, dy, dz;
		dx = spacing > edges.x ? 1 : int(edges.x/spacingEps);
		dy = spacing > edges.y ? 1 : int(edges.y/spacingEps);
		dz = spacing > edges.z ? 1 : int(edges.z/spacingEps);

		int maxDim = max(max(dx, dy), dz);

		// expand border by two voxels to ensure adequate sampling at edges
		meshLower -= 2.0f*Vec3(spacing);
		meshUpper += 2.0f*Vec3(spacing);
		maxDim += 4;

		vector<uint32_t> voxels(maxDim*maxDim*maxDim);

		// we shift the voxelization bounds so that the voxel centers
		// lie symmetrically to the center of the object. this reduces the 
		// chance of missing features, and also better aligns the particles
		// with the mesh
		Vec3 meshOffset;
		meshOffset.x = 0.5f * (spacing - (edges.x - (dx-1)*spacing));
		meshOffset.y = 0.5f * (spacing - (edges.y - (dy-1)*spacing));
		meshOffset.z = 0.5f * (spacing - (edges.z - (dz-1)*spacing));
		meshLower -= meshOffset;

		//Voxelize(*mesh, dx, dy, dz, &voxels[0], meshLower - Vec3(spacing*0.05f) , meshLower + Vec3(maxDim*spacing) + Vec3(spacing*0.05f));
		Voxelize((const Vec3*)&mesh.m_positions[0], mesh.m_positions.size(), (const int*)&mesh.m_indices[0], mesh.m_indices.size(), maxDim, maxDim, maxDim, &voxels[0], meshLower, meshLower + Vec3(maxDim*spacing));

		vector<int> indices(maxDim*maxDim*maxDim);
		vector<float> sdf(maxDim*maxDim*maxDim);
		MakeSDF(&voxels[0], maxDim, maxDim, maxDim, &sdf[0]);

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
						if (rigid)
							g_buffers->rigidIndices.push_back(int(g_buffers->positions.size()));

						Vec3 position = lower + meshLower + spacing*Vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f) + RandomUnitVector()*jitter;

						 // normalize the sdf value and transform to world scale
						Vec3 n = SafeNormalize(SampleSDFGrad(&sdf[0], maxDim, x, y, z));
						float d = sdf[index]*maxEdge;

						if (rigid)
							g_buffers->rigidLocalNormals.push_back(Vec4(n, d));

						// track which particles are in which cells
						indices[index] = g_buffers->positions.size();

						g_buffers->positions.push_back(Vec4(position.x, position.y, position.z, invMass));						
						g_buffers->velocities.push_back(velocity);
						g_buffers->phases.push_back(phase);
					}
				}
			}
		}
		mesh.Transform(ScaleMatrix(1.0f + skinExpand)*TranslationMatrix(Point3(-0.5f*(meshUpper+meshLower))));
		mesh.Transform(TranslationMatrix(Point3(lower + 0.5f*(meshUpper+meshLower))));	
	
	
		if (springStiffness > 0.0f)
		{
			// construct cross link springs to occupied cells
			for (int x=0; x < maxDim; ++x)
			{
				for (int y=0; y < maxDim; ++y)
				{
					for (int z=0; z < maxDim; ++z)
					{
						const int centerCell = z*maxDim*maxDim + y*maxDim + x;

						// if voxel is marked as occupied the add a particle
						if (voxels[centerCell])
						{
							const int width = 1;

							// create springs to all the neighbors within the width
							for (int i=x-width; i <= x+width; ++i)
							{
								for (int j=y-width; j <= y+width; ++j)
								{
									for (int k=z-width; k <= z+width; ++k)
									{
										const int neighborCell = k*maxDim*maxDim + j*maxDim + i;

										if (neighborCell > 0 && neighborCell < int(voxels.size()) && voxels[neighborCell] && neighborCell != centerCell)
										{
											CreateSpring(indices[neighborCell], indices[centerCell], springStiffness);
										}
									}
								}
							}
						}
					}
				}
			}
		}

	}
	

	if (skin)
	{
		g_buffers->rigidMeshSize.push_back(mesh.GetNumVertices());

		int startVertex = 0;

		if (!g_mesh)
			g_mesh = new Mesh();

		// append to mesh
		startVertex = g_mesh->GetNumVertices();

		g_mesh->Transform(TranslationMatrix(Point3(skinOffset)));
		g_mesh->AddMesh(mesh);

		const Colour colors[7] = 
		{
			Colour(0.0f, 0.5f, 1.0f),
			Colour(0.797f, 0.354f, 0.000f),			
			Colour(0.000f, 0.349f, 0.173f),
			Colour(0.875f, 0.782f, 0.051f),
			Colour(0.01f, 0.170f, 0.453f),
			Colour(0.673f, 0.111f, 0.000f),
			Colour(0.612f, 0.194f, 0.394f) 
		};

		for (uint32_t i=startVertex; i < g_mesh->GetNumVertices(); ++i)
		{
			int indices[g_numSkinWeights] = { -1, -1, -1, -1 };
			float distances[g_numSkinWeights] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
			
			if (LengthSq(color) == 0.0f)
				g_mesh->m_colours[i] = 1.25f*colors[((unsigned int)(phase))%7];
			else
				g_mesh->m_colours[i] = Colour(color);

			// find closest n particles
			for (int j=startIndex; j < g_buffers->positions.size(); ++j)
			{
				float dSq = LengthSq(Vec3(g_mesh->m_positions[i])-Vec3(g_buffers->positions[j]));

				// insertion sort
				int w=0;
				for (; w < 4; ++w)
					if (dSq < distances[w])
						break;
				
				if (w < 4)
				{
					// shuffle down
					for (int s=3; s > w; --s)
					{
						indices[s] = indices[s-1];
						distances[s] = distances[s-1];
					}

					distances[w] = dSq;
					indices[w] = int(j);				
				}
			}

			// weight particles according to distance
			float wSum = 0.0f;

			for (int w=0; w < 4; ++w)
			{				
				// convert to inverse distance
				distances[w] = 1.0f/(0.1f + powf(distances[w], .125f));

				wSum += distances[w];

			}

			float weights[4];
			for (int w=0; w < 4; ++w)
				weights[w] = distances[w]/wSum;

			for (int j=0; j < 4; ++j)
			{
				g_meshSkinIndices.push_back(indices[j]);
				g_meshSkinWeights.push_back(weights[j]);
			}
		}
	}

	if (rigid)
	{
		g_buffers->rigidCoefficients.push_back(rigidStiffness);
		g_buffers->rigidOffsets.push_back(int(g_buffers->rigidIndices.size()));
	}
}

// wrapper to create shape from a filename
void CreateParticleShape(const char* filename, Vec3 lower, Vec3 scale, float rotation, float spacing, Vec3 velocity, float invMass, bool rigid, float rigidStiffness, int phase, bool skin, float jitter=0.005f, Vec3 skinOffset=0.0f, float skinExpand=0.0f, Vec4 color=Vec4(0.0f), float springStiffness=0.0f)
{
	Mesh* mesh = ImportMesh(filename);
	if (mesh)
		CreateParticleShape(mesh, lower, scale, rotation, spacing, velocity, invMass, rigid, rigidStiffness, phase, skin, jitter, skinOffset, skinExpand, color, springStiffness);
	
	delete mesh;
}

void SkinMesh()
{
	if (g_mesh)
	{
		int startVertex = 0;

		for (int r=0; r < g_buffers->rigidRotations.size(); ++r)
		{
			const Matrix33 rotation = g_buffers->rigidRotations[r];
			const int numVertices = g_buffers->rigidMeshSize[r];

			for (int i=startVertex; i < numVertices+startVertex; ++i)
			{
				Vec3 skinPos;

				for (int w=0; w < 4; ++w)
				{
					// small shapes can have < 4 particles
					if (g_meshSkinIndices[i*4+w] > -1)
					{
						assert(g_meshSkinWeights[i*4+w] < FLT_MAX);

						int index = g_meshSkinIndices[i*4+w];
						float weight = g_meshSkinWeights[i*4+w];

						skinPos += (rotation*(g_meshRestPositions[i]-Point3(g_buffers->restPositions[index])) + Vec3(g_buffers->positions[index]))*weight;
					}
				}

				g_mesh->m_positions[i] = Point3(skinPos);
			}

			startVertex += numVertices;
		}

		g_mesh->CalculateNormals();
	}
}

void AddBox(Vec3 halfEdge = Vec3(2.0f), Vec3 center=Vec3(0.0f), Quat quat=Quat(), bool dynamic=false, int channels=eNvFlexPhaseShapeChannelMask)
{
	// transform
	g_buffers->shapePositions.push_back(Vec4(center.x, center.y, center.z, 0.0f));
	g_buffers->shapeRotations.push_back(quat);

	g_buffers->shapePrevPositions.push_back(g_buffers->shapePositions.back());
	g_buffers->shapePrevRotations.push_back(g_buffers->shapeRotations.back());

	NvFlexCollisionGeometry geo;
	geo.box.halfExtents[0] = halfEdge.x;
	geo.box.halfExtents[1] = halfEdge.y;
	geo.box.halfExtents[2] = halfEdge.z;

	g_buffers->shapeGeometry.push_back(geo);
	g_buffers->shapeFlags.push_back(NvFlexMakeShapeFlagsWithChannels(eNvFlexShapeBox, dynamic, channels));
}

// helper that creates a plinth whose center matches the particle bounds
void AddPlinth()
{
	Vec3 lower, upper;
	GetParticleBounds(lower, upper);

	Vec3 center = (lower+upper)*0.5f;
	center.y = 0.5f;

	AddBox(Vec3(2.0f, 0.5f, 2.0f), center);
}

void AddSphere(float radius, Vec3 position, Quat rotation)
{
	NvFlexCollisionGeometry geo;
	geo.sphere.radius = radius;
	g_buffers->shapeGeometry.push_back(geo);

	g_buffers->shapePositions.push_back(Vec4(position, 0.0f));
	g_buffers->shapeRotations.push_back(rotation);

	g_buffers->shapePrevPositions.push_back(g_buffers->shapePositions.back());
	g_buffers->shapePrevRotations.push_back(g_buffers->shapeRotations.back());

	int flags = NvFlexMakeShapeFlags(eNvFlexShapeSphere, false);
	g_buffers->shapeFlags.push_back(flags);
}

// creates a capsule aligned to the local x-axis with a given radius
void AddCapsule(float radius, float halfHeight, Vec3 position, Quat rotation)
{
	NvFlexCollisionGeometry geo;
	geo.capsule.radius = radius;
	geo.capsule.halfHeight = halfHeight;

	g_buffers->shapeGeometry.push_back(geo);

	g_buffers->shapePositions.push_back(Vec4(position, 0.0f));
	g_buffers->shapeRotations.push_back(rotation);

	g_buffers->shapePrevPositions.push_back(g_buffers->shapePositions.back());
	g_buffers->shapePrevRotations.push_back(g_buffers->shapeRotations.back());

	int flags = NvFlexMakeShapeFlags(eNvFlexShapeCapsule, false);
	g_buffers->shapeFlags.push_back(flags);
}

void CreateSDF(const Mesh* mesh, uint32_t dim, Vec3 lower, Vec3 upper, float* sdf)
{
	if (mesh)
	{
		printf("Begin mesh voxelization\n");

		double startVoxelize = GetSeconds();

		uint32_t* volume = new uint32_t[dim*dim*dim];
		Voxelize((const Vec3*)&mesh->m_positions[0], mesh->m_positions.size(), (const int*)&mesh->m_indices[0], mesh->m_indices.size(), dim, dim, dim, volume, lower, upper);

		printf("End mesh voxelization (%.2fs)\n", (GetSeconds()-startVoxelize));
	
		printf("Begin SDF gen (fast marching method)\n");

		double startSDF = GetSeconds();

		MakeSDF(volume, dim, dim, dim, sdf);

		printf("End SDF gen (%.2fs)\n", (GetSeconds()-startSDF));
	
		delete[] volume;
	}
}


void AddRandomConvex(int numPlanes, Vec3 position, float minDist, float maxDist, Vec3 axis, float angle)
{
	const int maxPlanes = 12;

	// 12-kdop
	const Vec3 directions[maxPlanes] = { 
		Vec3(1.0f, 0.0f, 0.0f),
		Vec3(0.0f, 1.0f, 0.0f),
		Vec3(0.0f, 0.0f, 1.0f),
		Vec3(-1.0f, 0.0f, 0.0f),
		Vec3(0.0f, -1.0f, 0.0f),
		Vec3(0.0f, 0.0f, -1.0f),
		Vec3(1.0f, 1.0f, 0.0f),
		Vec3(-1.0f, -1.0f, 0.0f),
		Vec3(1.0f, 0.0f, 1.0f),
		Vec3(-1.0f, 0.0f, -1.0f),
		Vec3(0.0f, 1.0f, 1.0f),
		Vec3(0.0f, -1.0f, -1.0f),
	 };

	numPlanes = Clamp(6, numPlanes, maxPlanes);

	int mesh = NvFlexCreateConvexMesh(g_flexLib);

	NvFlexVector<Vec4> planes(g_flexLib);
	planes.map();

	// create a box
	for (int i=0; i < numPlanes; ++i)
	{
		Vec4 plane = Vec4(Normalize(directions[i]), -Randf(minDist, maxDist));
		planes.push_back(plane);
	}

	g_buffers->shapePositions.push_back(Vec4(position.x, position.y, position.z, 0.0f));
	g_buffers->shapeRotations.push_back(QuatFromAxisAngle(axis, angle));

	g_buffers->shapePrevPositions.push_back(g_buffers->shapePositions.back());
	g_buffers->shapePrevRotations.push_back(g_buffers->shapeRotations.back());

	// set aabbs
	ConvexMeshBuilder builder(&planes[0]);
	builder(numPlanes);

	Vec3 lower(FLT_MAX), upper(-FLT_MAX);
	for (size_t v=0; v < builder.mVertices.size(); ++v)
	{
		const Vec3 p =  builder.mVertices[v];

		lower = Min(lower, p);
		upper = Max(upper, p);
	}

	planes.unmap();

	
	NvFlexUpdateConvexMesh(g_flexLib, mesh, planes.buffer, planes.size(), lower, upper);

	NvFlexCollisionGeometry geo;
	geo.convexMesh.mesh = mesh;
	geo.convexMesh.scale[0] = 1.0f;
	geo.convexMesh.scale[1] = 1.0f;
	geo.convexMesh.scale[2] = 1.0f;

	g_buffers->shapeGeometry.push_back(geo);

	int flags = NvFlexMakeShapeFlags(eNvFlexShapeConvexMesh, false);
	g_buffers->shapeFlags.push_back(flags);


	// create render mesh for convex
	Mesh renderMesh;

	for (uint32_t j = 0; j < builder.mIndices.size(); j += 3)
	{
		uint32_t a = builder.mIndices[j + 0];
		uint32_t b = builder.mIndices[j + 1];
		uint32_t c = builder.mIndices[j + 2];

		Vec3 n = Normalize(Cross(builder.mVertices[b] - builder.mVertices[a], builder.mVertices[c] - builder.mVertices[a]));
		
		int startIndex = renderMesh.m_positions.size();

		renderMesh.m_positions.push_back(Point3(builder.mVertices[a]));
		renderMesh.m_normals.push_back(n);

		renderMesh.m_positions.push_back(Point3(builder.mVertices[b]));
		renderMesh.m_normals.push_back(n);

		renderMesh.m_positions.push_back(Point3(builder.mVertices[c]));
		renderMesh.m_normals.push_back(n);

		renderMesh.m_indices.push_back(startIndex+0);
		renderMesh.m_indices.push_back(startIndex+1);
		renderMesh.m_indices.push_back(startIndex+2);
	}

	// insert into the global mesh list
	GpuMesh* gpuMesh = CreateGpuMesh(&renderMesh);
	g_convexes[mesh] = gpuMesh;
}

void CreateRandomBody(int numPlanes, Vec3 position, float minDist, float maxDist, Vec3 axis, float angle, float invMass, int phase, float stiffness)
{
	// 12-kdop
	const Vec3 directions[] = { 
		Vec3(1.0f, 0.0f, 0.0f),
		Vec3(0.0f, 1.0f, 0.0f),
		Vec3(0.0f, 0.0f, 1.0f),
		Vec3(-1.0f, 0.0f, 0.0f),
		Vec3(0.0f, -1.0f, 0.0f),
		Vec3(0.0f, 0.0f, -1.0f),
		Vec3(1.0f, 1.0f, 0.0f),
		Vec3(-1.0f, -1.0f, 0.0f),
		Vec3(1.0f, 0.0f, 1.0f),
		Vec3(-1.0f, 0.0f, -1.0f),
		Vec3(0.0f, 1.0f, 1.0f),
		Vec3(0.0f, -1.0f, -1.0f),
	 };

	numPlanes = max(4, numPlanes);

	vector<Vec4> planes;

	// create a box
	for (int i=0; i < numPlanes; ++i)
	{
		// pick random dir and distance
		Vec3 dir = Normalize(directions[i]);//RandomUnitVector();
		float dist = Randf(minDist, maxDist);

		planes.push_back(Vec4(dir.x, dir.y, dir.z, -dist));
	}

	// set aabbs
	ConvexMeshBuilder builder(&planes[0]);
	builder(numPlanes);
			
	int startIndex = int(g_buffers->positions.size());

	for (size_t v=0; v < builder.mVertices.size(); ++v)
	{
		Quat q = QuatFromAxisAngle(axis, angle);
		Vec3 p =  rotate(Vec3(q), q.w, builder.mVertices[v]) + position;

		g_buffers->positions.push_back(Vec4(p.x, p.y, p.z, invMass));
		g_buffers->velocities.push_back(0.0f);
		g_buffers->phases.push_back(phase);

		// add spring to all verts with higher index
		for (size_t i=v+1; i < builder.mVertices.size(); ++i)
		{
			int a = startIndex + int(v);
			int b = startIndex + int(i);

			g_buffers->springIndices.push_back(a);
			g_buffers->springIndices.push_back(b);
			g_buffers->springLengths.push_back(Length(builder.mVertices[v]-builder.mVertices[i]));
			g_buffers->springStiffness.push_back(stiffness);

		}
	}	

	for (size_t t=0; t < builder.mIndices.size(); ++t)
		g_buffers->triangles.push_back(startIndex + builder.mIndices[t]);		

	// lazy
	g_buffers->triangleNormals.resize(g_buffers->triangleNormals.size() + builder.mIndices.size()/3, Vec3(0.0f));
}

NvFlexTriangleMeshId CreateTriangleMesh(Mesh* m)
{
	if (!m)
		return 0;

	Vec3 lower, upper;
	m->GetBounds(lower, upper);

	NvFlexVector<Vec4> positions(g_flexLib, m->m_positions.size());
	positions.map();
	NvFlexVector<int> indices(g_flexLib);

	for (int i = 0; i < int(m->m_positions.size()); ++i)
	{
		Vec3 vertex = Vec3(m->m_positions[i]);
		positions[i] = Vec4(vertex, 0.0f);
	}
	indices.assign((int*)&m->m_indices[0], m->m_indices.size());

	positions.unmap();
	indices.unmap();

	NvFlexTriangleMeshId flexMesh = NvFlexCreateTriangleMesh(g_flexLib);
	NvFlexUpdateTriangleMesh(g_flexLib, flexMesh, positions.buffer, indices.buffer, m->GetNumVertices(), m->GetNumFaces(), (float*)&lower, (float*)&upper);

	// entry in the collision->render map
	g_meshes[flexMesh] = CreateGpuMesh(m);
	
	return flexMesh;
}

void AddTriangleMesh(NvFlexTriangleMeshId mesh, Vec3 translation, Quat rotation, Vec3 scale)
{
	Vec3 lower, upper;
	NvFlexGetTriangleMeshBounds(g_flexLib, mesh, lower, upper);

	NvFlexCollisionGeometry geo;
	geo.triMesh.mesh = mesh;
	geo.triMesh.scale[0] = scale.x;
	geo.triMesh.scale[1] = scale.y;
	geo.triMesh.scale[2] = scale.z;

	g_buffers->shapePositions.push_back(Vec4(translation, 0.0f));
	g_buffers->shapeRotations.push_back(Quat(rotation));
	g_buffers->shapePrevPositions.push_back(Vec4(translation, 0.0f));
	g_buffers->shapePrevRotations.push_back(Quat(rotation));
	g_buffers->shapeGeometry.push_back((NvFlexCollisionGeometry&)geo);
	g_buffers->shapeFlags.push_back(NvFlexMakeShapeFlags(eNvFlexShapeTriangleMesh, false));
}

NvFlexDistanceFieldId CreateSDF(const char* meshFile, int dim, float margin = 0.1f, float expand = 0.0f)
{
	Mesh* mesh = ImportMesh(meshFile);

	// include small margin to ensure valid gradients near the boundary
	mesh->Normalize(1.0f - margin);
	mesh->Transform(TranslationMatrix(Point3(margin, margin, margin)*0.5f));

	Vec3 lower(0.0f);
	Vec3 upper(1.0f);

	// try and load the sdf from disc if it exists
	// Begin Add Android Support
#ifdef ANDROID
	string sdfFile = string(meshFile, strlen(meshFile) - strlen(strrchr(meshFile, '.'))) + ".pfm";
#else
	string sdfFile = string(meshFile, strrchr(meshFile, '.')) + ".pfm";
#endif
	// End Add Android Support

	PfmImage pfm;
	if (!PfmLoad(sdfFile.c_str(), pfm))
	{
		pfm.m_width = dim;
		pfm.m_height = dim;
		pfm.m_depth = dim;
		pfm.m_data = new float[dim*dim*dim];

		printf("Cooking SDF: %s - dim: %d^3\n", sdfFile.c_str(), dim);

		CreateSDF(mesh, dim, lower, upper, pfm.m_data);

		PfmSave(sdfFile.c_str(), pfm);
	}

	//printf("Loaded SDF, %d\n", pfm.m_width);

	assert(pfm.m_width == pfm.m_height && pfm.m_width == pfm.m_depth);

	// cheap collision offset
	int numVoxels = int(pfm.m_width*pfm.m_height*pfm.m_depth);
	for (int i = 0; i < numVoxels; ++i)
		pfm.m_data[i] += expand;

	NvFlexVector<float> field(g_flexLib);
	field.assign(pfm.m_data, pfm.m_width*pfm.m_height*pfm.m_depth);
	field.unmap();

	// set up flex collision shape
	NvFlexDistanceFieldId sdf = NvFlexCreateDistanceField(g_flexLib);
	NvFlexUpdateDistanceField(g_flexLib, sdf, dim, dim, dim, field.buffer);

	// entry in the collision->render map
	g_fields[sdf] = CreateGpuMesh(mesh);

	delete mesh;
	delete[] pfm.m_data;

	return sdf;
}

void AddSDF(NvFlexDistanceFieldId sdf, Vec3 translation, Quat rotation, float width)
{
	NvFlexCollisionGeometry geo;
	geo.sdf.field = sdf;
	geo.sdf.scale = width;

	g_buffers->shapePositions.push_back(Vec4(translation, 0.0f));
	g_buffers->shapeRotations.push_back(Quat(rotation));
	g_buffers->shapePrevPositions.push_back(Vec4(translation, 0.0f));
	g_buffers->shapePrevRotations.push_back(Quat(rotation));
	g_buffers->shapeGeometry.push_back((NvFlexCollisionGeometry&)geo);
	g_buffers->shapeFlags.push_back(NvFlexMakeShapeFlags(eNvFlexShapeSDF, false));
}

inline int GridIndex(int x, int y, int dx) { return y*dx + x; }

void CreateSpringGrid(Vec3 lower, int dx, int dy, int dz, float radius, int phase, float stretchStiffness, float bendStiffness, float shearStiffness, Vec3 velocity, float invMass)
{
	int baseIndex = int(g_buffers->positions.size());

	for (int z=0; z < dz; ++z)
	{
		for (int y=0; y < dy; ++y)
		{
			for (int x=0; x < dx; ++x)
			{
				Vec3 position = lower + radius*Vec3(float(x), float(z), float(y));

				g_buffers->positions.push_back(Vec4(position.x, position.y, position.z, invMass));
				g_buffers->velocities.push_back(velocity);
				g_buffers->phases.push_back(phase);

				if (x > 0 && y > 0)
				{
					g_buffers->triangles.push_back(baseIndex + GridIndex(x-1, y-1, dx));
					g_buffers->triangles.push_back(baseIndex + GridIndex(x, y-1, dx));
					g_buffers->triangles.push_back(baseIndex + GridIndex(x, y, dx));
					
					g_buffers->triangles.push_back(baseIndex + GridIndex(x-1, y-1, dx));
					g_buffers->triangles.push_back(baseIndex + GridIndex(x, y, dx));
					g_buffers->triangles.push_back(baseIndex + GridIndex(x-1, y, dx));

					g_buffers->triangleNormals.push_back(Vec3(0.0f, 1.0f, 0.0f));
					g_buffers->triangleNormals.push_back(Vec3(0.0f, 1.0f, 0.0f));
				}
			}
		}
	}	

	// horizontal
	for (int y=0; y < dy; ++y)
	{
		for (int x=0; x < dx; ++x)
		{
			int index0 = y*dx + x;

			if (x > 0)
			{
				int index1 = y*dx + x - 1;
				CreateSpring(baseIndex + index0, baseIndex + index1, stretchStiffness);
			}

			if (x > 1)
			{
				int index2 = y*dx + x - 2;
				CreateSpring(baseIndex + index0, baseIndex + index2, bendStiffness);
			}

			if (y > 0 && x < dx-1)
			{
				int indexDiag = (y-1)*dx + x + 1;
				CreateSpring(baseIndex + index0, baseIndex + indexDiag, shearStiffness);
			}

			if (y > 0 && x > 0)
			{
				int indexDiag = (y-1)*dx + x - 1;
				CreateSpring(baseIndex + index0, baseIndex + indexDiag, shearStiffness);
			}
		}
	}

	// vertical
	for (int x=0; x < dx; ++x)
	{
		for (int y=0; y < dy; ++y)
		{
			int index0 = y*dx + x;

			if (y > 0)
			{
				int index1 = (y-1)*dx + x;
				CreateSpring(baseIndex + index0, baseIndex + index1, stretchStiffness);
			}

			if (y > 1)
			{
				int index2 = (y-2)*dx + x;
				CreateSpring(baseIndex + index0, baseIndex + index2, bendStiffness);
			}
		}
	}	
}

void CreateRope(Rope& rope, Vec3 start, Vec3 dir, float stiffness, int segments, float length, int phase, float spiralAngle=0.0f, float invmass=1.0f, float give=0.075f)
{
	rope.mIndices.push_back(int(g_buffers->positions.size()));

	g_buffers->positions.push_back(Vec4(start.x, start.y, start.z, invmass));
	g_buffers->velocities.push_back(0.0f);
	g_buffers->phases.push_back(phase);//int(g_buffers->positions.size()));
	
	Vec3 left, right;
	BasisFromVector(dir, &left, &right);

	float segmentLength = length/segments;
	Vec3 spiralAxis = dir;
	float spiralHeight = spiralAngle/(2.0f*kPi)*(length/segments);

	if (spiralAngle > 0.0f)
		dir = left;

	Vec3 p = start;

	for (int i=0; i < segments; ++i)
	{
		int prev = int(g_buffers->positions.size())-1;

		p += dir*segmentLength;

		// rotate 
		if (spiralAngle > 0.0f)
		{
			p += spiralAxis*spiralHeight;

			dir = RotationMatrix(spiralAngle, spiralAxis)*dir;
		}

		rope.mIndices.push_back(int(g_buffers->positions.size()));

		g_buffers->positions.push_back(Vec4(p.x, p.y, p.z, 1.0f));
		g_buffers->velocities.push_back(0.0f);
		g_buffers->phases.push_back(phase);//int(g_buffers->positions.size()));

		// stretch
		CreateSpring(prev, prev+1, stiffness, give);

		// tether
		//if (i > 0 && i%4 == 0)
			//CreateSpring(prev-3, prev+1, -0.25f);
		
		// bending spring
		if (i > 0)
			CreateSpring(prev-1, prev+1, stiffness*0.5f, give);
	}
}

namespace
{
	struct Tri
	{
		int a;
		int b;
		int c;

		Tri(int a, int b, int c) : a(a), b(b), c(c) {}

		bool operator < (const Tri& rhs)
		{
			if (a != rhs.a)
				return a < rhs.a;
			else if (b != rhs.b)
				return b < rhs.b;
			else
				return c < rhs.c;
		}
	};
}


namespace
{
	struct TriKey
	{
		int orig[3];
		int indices[3];

		TriKey(int a, int b, int c)		
		{
			orig[0] = a;
			orig[1] = b;
			orig[2] = c;

			indices[0] = a;
			indices[1] = b;
			indices[2] = c;

			std::sort(indices, indices+3);
		}			

		bool operator < (const TriKey& rhs) const
		{
			if (indices[0] != rhs.indices[0])
				return indices[0] < rhs.indices[0];
			else if (indices[1] != rhs.indices[1])
				return indices[1] < rhs.indices[1];
			else
				return indices[2] < rhs.indices[2];
		}
	};
}

void CreateTetMesh(const char* filename, Vec3 lower, float scale, float stiffness, int phase)
{
	FILE* f = fopen(filename, "r");

	char line[2048];

	if (f)
	{
		typedef std::map<TriKey, int> TriMap;
		TriMap triCount;

		const int vertOffset = g_buffers->positions.size();

		Vec3 meshLower(FLT_MAX);
		Vec3 meshUpper(-FLT_MAX);

		bool firstTet = true;

		while (!feof(f))
		{
			if (fgets(line, 2048, f))
			{
				switch(line[0])
				{
				case '#':
					break;
				case 'v':
					{
						Vec3 pos;
						sscanf(line, "v %f %f %f", &pos.x, &pos.y, &pos.z);

						g_buffers->positions.push_back(Vec4(pos.x, pos.y, pos.z, 1.0f));
						g_buffers->velocities.push_back(0.0f);
						g_buffers->phases.push_back(phase);

						meshLower = Min(pos, meshLower);
						meshUpper = Max(pos, meshUpper);
						break;
					}
				case 't':
					{
						if (firstTet)
						{
							Vec3 edges = meshUpper-meshLower;
							float maxEdge = max(edges.x, max(edges.y, edges.z));

							// normalize positions
							for (int i=vertOffset; i < int(g_buffers->positions.size()); ++i)
							{
								Vec3 p = lower + (Vec3(g_buffers->positions[i])-meshLower)*scale/maxEdge;
								g_buffers->positions[i] = Vec4(p, g_buffers->positions[i].w);
							}

							firstTet = false;
						}

						int indices[4];
						sscanf(line, "t %d %d %d %d", &indices[0], &indices[1], &indices[2], &indices[3]);

						indices[0] += vertOffset;
						indices[1] += vertOffset;
						indices[2] += vertOffset;
						indices[3] += vertOffset;

						CreateSpring(indices[0], indices[1], stiffness);
						CreateSpring(indices[0], indices[2], stiffness);
						CreateSpring(indices[0], indices[3], stiffness);
				
						CreateSpring(indices[1], indices[2], stiffness);
						CreateSpring(indices[1], indices[3], stiffness);
						CreateSpring(indices[2], indices[3], stiffness);

						TriKey k1(indices[0], indices[2], indices[1]);
						triCount[k1] += 1;

						TriKey k2(indices[1], indices[2], indices[3]);
						triCount[k2] += 1;

						TriKey k3(indices[0], indices[1], indices[3]);
						triCount[k3] += 1;

						TriKey k4(indices[0], indices[3], indices[2]);
						triCount[k4] += 1;

						break;
					}
				}
			}
		}

		for (TriMap::iterator iter=triCount.begin(); iter != triCount.end(); ++iter)
		{
			TriKey key = iter->first;

			// only output faces that are referenced by one tet (open faces)
			if (iter->second == 1)
			{
				g_buffers->triangles.push_back(key.orig[0]);
				g_buffers->triangles.push_back(key.orig[1]);
				g_buffers->triangles.push_back(key.orig[2]);
				g_buffers->triangleNormals.push_back(0.0f);
			}
		}


		fclose(f);
	}
}


// finds the closest particle to a view ray
int PickParticle(Vec3 origin, Vec3 dir, Vec4* particles, int* phases, int n, float radius, float &outT)
{
	float maxDistSq = radius*radius;
	float minT = FLT_MAX;
	int minIndex = -1;

	for (int i=0; i < n; ++i)
	{
		if (phases[i] & eNvFlexPhaseFluid)
			continue;

		Vec3 delta = Vec3(particles[i])-origin;
		float t = Dot(delta, dir);

		if (t > 0.0f)
		{
			Vec3 perp = delta - t*dir;

			float dSq = LengthSq(perp);

			if (dSq < maxDistSq && t < minT)
			{
				minT = t;
				minIndex = i;
			}
		}
	}

	outT = minT;

	return minIndex;
}

// calculates the center of mass of every rigid given a set of particle positions and rigid indices
void CalculateRigidCentersOfMass(const Vec4* restPositions, int numRestPositions, const int* offsets, Vec3* translations, const int* indices, int numRigids)
{
	// To improve the accuracy of the result, first transform the restPositions to relative coordinates (by finding the mean and subtracting that from all positions)
	// Note: If this is not done, one might see ghost forces if the mean of the restPositions is far from the origin.
	Vec3 shapeOffset(0.0f);

	for (int i = 0; i < numRestPositions; i++)
	{
		shapeOffset += Vec3(restPositions[i]);
	}

	shapeOffset /= float(numRestPositions);

	for (int i=0; i < numRigids; ++i)
	{
		const int startIndex = offsets[i];
		const int endIndex = offsets[i+1];

		const int n = endIndex-startIndex;

		assert(n);

		Vec3 com;
	
		for (int j=startIndex; j < endIndex; ++j)
		{
			const int r = indices[j];

			// By subtracting shapeOffset the calculation is done in relative coordinates
			com += Vec3(restPositions[r]) - shapeOffset;
		}

		com /= float(n);

		// Add the shapeOffset to switch back to absolute coordinates
		com += shapeOffset;

		translations[i] = com;

	}
}

// calculates local space positions given a set of particle positions, rigid indices and centers of mass of the rigids
void CalculateRigidLocalPositions(const Vec4* restPositions, const int* offsets, const Vec3* translations, const int* indices, int numRigids, Vec3* localPositions)
{
	int count = 0;

	for (int i=0; i < numRigids; ++i)
	{
		const int startIndex = offsets[i];
		const int endIndex = offsets[i+1];

		assert(endIndex-startIndex);

		for (int j=startIndex; j < endIndex; ++j)
		{
			const int r = indices[j];

			localPositions[count++] = Vec3(restPositions[r]) - translations[i];
		}
	}
}

void DrawImguiString(int x, int y, Vec3 color, int align, const char* s, ...)
{
	char buf[2048];

	va_list args;

	va_start(args, s);
	vsnprintf(buf, 2048, s, args);
	va_end(args);

	imguiDrawText(x, y, align, buf, imguiRGBA((unsigned char)(color.x*255), (unsigned char)(color.y*255), (unsigned char)(color.z*255)));
}

enum 
{
	HELPERS_SHADOW_OFFSET = 1,
};

void DrawShadowedText(int x, int y, Vec3 color, int align, const char* s, ...)
{
	char buf[2048];

	va_list args;

	va_start(args, s);
	vsnprintf(buf, 2048, s, args);
	va_end(args);


	imguiDrawText(x + HELPERS_SHADOW_OFFSET, y - HELPERS_SHADOW_OFFSET, align, buf, imguiRGBA(0, 0, 0));
	imguiDrawText(x, y, align, buf, imguiRGBA((unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255)));
}

void DrawRect(float x, float y, float w, float h, Vec3 color)
{
	imguiDrawRect(x, y, w, h, imguiRGBA((unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255)));
}

void DrawShadowedRect(float x, float y, float w, float h, Vec3 color)
{
	imguiDrawRect(x + HELPERS_SHADOW_OFFSET, y - HELPERS_SHADOW_OFFSET, w, h, imguiRGBA(0, 0, 0));
	imguiDrawRect(x, y, w, h, imguiRGBA((unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255)));
}

void DrawLine(float x0, float y0, float x1, float y1, float r, Vec3 color)
{
	imguiDrawLine(x0, y0, x1, y1, r, imguiRGBA((unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255)));
}

void DrawShadowedLine(float x0, float y0, float x1, float y1, float r, Vec3 color)
{
	imguiDrawLine(x0 + HELPERS_SHADOW_OFFSET, y0 - HELPERS_SHADOW_OFFSET, x1 + HELPERS_SHADOW_OFFSET, y1 - HELPERS_SHADOW_OFFSET, r, imguiRGBA(0, 0, 0));
	imguiDrawLine(x0, y0, x1, y1, r, imguiRGBA((unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255)));
}

// Soft body support functions

Vec3 CalculateMean(const Vec3* particles, const int* indices, int numIndices)
{
	Vec3 sum;

	for (int i = 0; i < numIndices; ++i)
		sum += Vec3(particles[indices[i]]);

	if (numIndices)
		return sum / float(numIndices);
	else
		return sum;
}

float CalculateRadius(const Vec3* particles, Vec3 center, const int* indices, int numIndices)
{
	float radiusSq = 0.0f;

	for (int i = 0; i < numIndices; ++i)
	{
		float dSq = LengthSq(Vec3(particles[indices[i]]) - center);
		if (dSq > radiusSq)
			radiusSq = dSq;
	}

	return sqrtf(radiusSq);
}

struct Cluster
{
	Vec3 mean;
	float radius;

	// indices of particles belonging to this cluster
	std::vector<int> indices;
};

struct Seed
{
	int index;
	float priority;

	bool operator < (const Seed& rhs) const
	{
		return priority < rhs.priority;
	}
};

int CreateClusters(Vec3* particles, const float* priority, int numParticles, std::vector<int>& outClusterOffsets, std::vector<int>& outClusterIndices, std::vector<Vec3>& outClusterPositions, float radius, float smoothing = 0.0f)
{
	std::vector<Seed> seeds;
	std::vector<Cluster> clusters;

	// flags a particle as belonging to at least one cluster
	std::vector<bool> used(numParticles, false);

	// initialize seeds
	for (int i = 0; i < numParticles; ++i)
	{
		Seed s;
		s.index = i;
		s.priority = priority[i];

		seeds.push_back(s);
	}

	std::stable_sort(seeds.begin(), seeds.end());

	while (seeds.size())
	{
		// pick highest unused particle from the seeds list
		Seed seed = seeds.back();
		seeds.pop_back();

		if (!used[seed.index])
		{
			Cluster c;

			const float radiusSq = sqr(radius);

			// push all neighbors within radius
			for (int p = 0; p < numParticles; ++p)
			{
				float dSq = LengthSq(Vec3(particles[seed.index]) - Vec3(particles[p]));
				if (dSq <= radiusSq)
				{
					c.indices.push_back(p);

					used[p] = true;
				}
			}

			c.mean = CalculateMean(particles, &c.indices[0], c.indices.size());

			clusters.push_back(c);
		}
	}

	if (smoothing > 0.0f)
	{
		// expand clusters by smoothing radius
		float radiusSmoothSq = sqr(smoothing);

		for (int i = 0; i < int(clusters.size()); ++i)
		{
			Cluster& c = clusters[i];

			// clear cluster indices
			c.indices.resize(0);

			// push all neighbors within radius
			for (int p = 0; p < numParticles; ++p)
			{
				float dSq = LengthSq(c.mean - Vec3(particles[p]));
				if (dSq <= radiusSmoothSq)
					c.indices.push_back(p);
			}

			c.mean = CalculateMean(particles, &c.indices[0], c.indices.size());
		}
	}

	// write out cluster indices
	int count = 0;

	//outClusterOffsets.push_back(0);

	for (int c = 0; c < int(clusters.size()); ++c)
	{
		const Cluster& cluster = clusters[c];

		const int clusterSize = int(cluster.indices.size());

		// skip empty clusters
		if (clusterSize)
		{
			// write cluster indices
			for (int i = 0; i < clusterSize; ++i)
				outClusterIndices.push_back(cluster.indices[i]);

			// write cluster offset
			outClusterOffsets.push_back(outClusterIndices.size());

			// write center
			outClusterPositions.push_back(cluster.mean);

			++count;
		}
	}

	return count;
}

// creates distance constraints between particles within some distance
int CreateLinks(const Vec3* particles, int numParticles, std::vector<int>& outSpringIndices, std::vector<float>& outSpringLengths, std::vector<float>& outSpringStiffness, float radius, float stiffness = 1.0f)
{
	const float radiusSq = sqr(radius);

	int count = 0;

	for (int i = 0; i < numParticles; ++i)
	{
		for (int j = i + 1; j < numParticles; ++j)
		{
			float dSq = LengthSq(Vec3(particles[i]) - Vec3(particles[j]));

			if (dSq < radiusSq)
			{
				outSpringIndices.push_back(i);
				outSpringIndices.push_back(j);
				outSpringLengths.push_back(sqrtf(dSq));
				outSpringStiffness.push_back(stiffness);

				++count;
			}
		}
	}

	return count;
}

void CreateSkinning(const Vec3* vertices, int numVertices, const Vec3* clusters, int numClusters, float* outWeights, int* outIndices, float falloff, float maxdist)
{
	const int maxBones = 4;

	// for each vertex, find the closest n clusters
	for (int i = 0; i < numVertices; ++i)
	{
		int indices[4] = { -1, -1, -1, -1 };
		float distances[4] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
		float weights[maxBones];

		for (int c = 0; c < numClusters; ++c)
		{
			float dSq = LengthSq(vertices[i] - clusters[c]);

			// insertion sort
			int w = 0;
			for (; w < maxBones; ++w)
				if (dSq < distances[w])
					break;

			if (w < maxBones)
			{
				// shuffle down
				for (int s = maxBones - 1; s > w; --s)
				{
					indices[s] = indices[s - 1];
					distances[s] = distances[s - 1];
				}

				distances[w] = dSq;
				indices[w] = c;
			}
		}

		// weight particles according to distance
		float wSum = 0.0f;

		for (int w = 0; w < maxBones; ++w)
		{
			if (distances[w] > sqr(maxdist))
			{
				// clamp bones over a given distance to zero
				weights[w] = 0.0f;
			}
			else
			{
				// weight falls off inversely with distance
				weights[w] = 1.0f / (powf(distances[w], falloff) + 0.0001f);
			}

			wSum += weights[w];
		}

		if (wSum == 0.0f)
		{
			// if all weights are zero then just 
			// rigidly skin to the closest bone
			weights[0] = 1.0f;
		}
		else
		{
			// normalize weights
			for (int w = 0; w < maxBones; ++w)
			{
				weights[w] = weights[w] / wSum;
			}
		}

		// output
		for (int j = 0; j < maxBones; ++j)
		{
			outWeights[i*maxBones + j] = weights[j];
			outIndices[i*maxBones + j] = indices[j];
		}
	}
}


void SampleMesh(Mesh* mesh, Vec3 lower, Vec3 scale, float rotation, float radius, float volumeSampling, float surfaceSampling, std::vector<Vec3>& outPositions)
{
	if (!mesh)
		return;

	mesh->Transform(RotationMatrix(rotation, Vec3(0.0f, 1.0f, 0.0f)));

	Vec3 meshLower, meshUpper;
	mesh->GetBounds(meshLower, meshUpper);

	Vec3 edges = meshUpper - meshLower;
	float maxEdge = max(max(edges.x, edges.y), edges.z);

	// put mesh at the origin and scale to specified size
	Matrix44 xform = ScaleMatrix(scale / maxEdge)*TranslationMatrix(Point3(-meshLower));

	mesh->Transform(xform);
	mesh->GetBounds(meshLower, meshUpper);

	std::vector<Vec3> samples;

	if (volumeSampling > 0.0f)
	{
		// recompute expanded edges
		edges = meshUpper - meshLower;
		maxEdge = max(max(edges.x, edges.y), edges.z);

		// use a higher resolution voxelization as a basis for the particle decomposition
		float spacing = radius / volumeSampling;

		// tweak spacing to avoid edge cases for particles laying on the boundary
		// just covers the case where an edge is a whole multiple of the spacing.
		float spacingEps = spacing*(1.0f - 1e-4f);

		// make sure to have at least one particle in each dimension
		int dx, dy, dz;
		dx = spacing > edges.x ? 1 : int(edges.x / spacingEps);
		dy = spacing > edges.y ? 1 : int(edges.y / spacingEps);
		dz = spacing > edges.z ? 1 : int(edges.z / spacingEps);

		int maxDim = max(max(dx, dy), dz);

		// expand border by two voxels to ensure adequate sampling at edges
		meshLower -= 2.0f*Vec3(spacing);
		meshUpper += 2.0f*Vec3(spacing);
		maxDim += 4;

		vector<uint32_t> voxels(maxDim*maxDim*maxDim);

		// we shift the voxelization bounds so that the voxel centers
		// lie symmetrically to the center of the object. this reduces the 
		// chance of missing features, and also better aligns the particles
		// with the mesh
		Vec3 meshOffset;
		meshOffset.x = 0.5f * (spacing - (edges.x - (dx - 1)*spacing));
		meshOffset.y = 0.5f * (spacing - (edges.y - (dy - 1)*spacing));
		meshOffset.z = 0.5f * (spacing - (edges.z - (dz - 1)*spacing));
		meshLower -= meshOffset;

		//Voxelize(*mesh, dx, dy, dz, &voxels[0], meshLower - Vec3(spacing*0.05f) , meshLower + Vec3(maxDim*spacing) + Vec3(spacing*0.05f));
		Voxelize((const Vec3*)&mesh->m_positions[0], mesh->m_positions.size(), (const int*)&mesh->m_indices[0], mesh->m_indices.size(), maxDim, maxDim, maxDim, &voxels[0], meshLower, meshLower + Vec3(maxDim*spacing));

		// sample interior
		for (int x = 0; x < maxDim; ++x)
		{
			for (int y = 0; y < maxDim; ++y)
			{
				for (int z = 0; z < maxDim; ++z)
				{
					const int index = z*maxDim*maxDim + y*maxDim + x;

					// if voxel is marked as occupied the add a particle
					if (voxels[index])
					{
						Vec3 position = lower + meshLower + spacing*Vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f);

						// normalize the sdf value and transform to world scale
						samples.push_back(position);
					}
				}
			}
		}
	}

	// move back
	mesh->Transform(ScaleMatrix(1.0f)*TranslationMatrix(Point3(-0.5f*(meshUpper + meshLower))));
	mesh->Transform(TranslationMatrix(Point3(lower + 0.5f*(meshUpper + meshLower))));

	if (surfaceSampling > 0.0f)
	{
		// sample vertices
		for (int i = 0; i < int(mesh->m_positions.size()); ++i)
			samples.push_back(Vec3(mesh->m_positions[i]));

		// random surface sampling
		if (1)
		{
			for (int i = 0; i < 50000; ++i)
			{
				int t = Rand() % mesh->GetNumFaces();
				float u = Randf();
				float v = Randf()*(1.0f - u);
				float w = 1.0f - u - v;

				int a = mesh->m_indices[t * 3 + 0];
				int b = mesh->m_indices[t * 3 + 1];
				int c = mesh->m_indices[t * 3 + 2];
				
				Point3 pt = mesh->m_positions[a] * u + mesh->m_positions[b] * v + mesh->m_positions[c] * w;
				Vec3 p(pt.x,pt.y,pt.z);

				samples.push_back(p);
			}
		}
	}

	std::vector<int> clusterIndices;
	std::vector<int> clusterOffsets;
	std::vector<Vec3> clusterPositions;
	std::vector<float> priority(samples.size());

	CreateClusters(&samples[0], &priority[0], samples.size(), clusterOffsets, clusterIndices, outPositions, radius);

}

void ClearShapes()
{
	g_buffers->shapeGeometry.resize(0);
	g_buffers->shapePositions.resize(0);
	g_buffers->shapeRotations.resize(0);
	g_buffers->shapePrevPositions.resize(0);
	g_buffers->shapePrevRotations.resize(0);
	g_buffers->shapeFlags.resize(0);
}

void UpdateShapes()
{	
	// mark shapes as dirty so they are sent to flex during the next update
	g_shapesChanged = true;
}

// calculates the union bounds of all the collision shapes in the scene
void GetShapeBounds(Vec3& totalLower, Vec3& totalUpper)
{
	Bounds totalBounds;

	for (int i=0; i < g_buffers->shapeFlags.size(); ++i)
	{
		NvFlexCollisionGeometry geo = g_buffers->shapeGeometry[i];

		int type = g_buffers->shapeFlags[i]&eNvFlexShapeFlagTypeMask;

		Vec3 localLower;
		Vec3 localUpper;

		switch(type)
		{
			case eNvFlexShapeBox:
			{
				localLower = -Vec3(geo.box.halfExtents);
				localUpper = Vec3(geo.box.halfExtents);
				break;
			}
			case eNvFlexShapeSphere:
			{
				localLower = -geo.sphere.radius;
				localUpper = geo.sphere.radius;
				break;
			}
			case eNvFlexShapeCapsule:
			{
				localLower = -Vec3(geo.capsule.halfHeight, 0.0f, 0.0f) - Vec3(geo.capsule.radius);
				localUpper = Vec3(geo.capsule.halfHeight, 0.0f, 0.0f) + Vec3(geo.capsule.radius);
				break;
			}
			case eNvFlexShapeConvexMesh:
			{
				NvFlexGetConvexMeshBounds(g_flexLib, geo.convexMesh.mesh, localLower, localUpper);

				// apply instance scaling
				localLower *= geo.convexMesh.scale;
				localUpper *= geo.convexMesh.scale;
				break;
			}
			case eNvFlexShapeTriangleMesh:
			{
				NvFlexGetTriangleMeshBounds(g_flexLib, geo.triMesh.mesh, localLower, localUpper);
				
				// apply instance scaling
				localLower *= Vec3(geo.triMesh.scale);
				localUpper *= Vec3(geo.triMesh.scale);
				break;
			}
			case eNvFlexShapeSDF:
			{
				localLower = 0.0f;
				localUpper = geo.sdf.scale;
				break;
			}
		};

		// transform local bounds to world space
		Vec3 worldLower, worldUpper;
		TransformBounds(localLower, localUpper, Vec3(g_buffers->shapePositions[i]), g_buffers->shapeRotations[i], 1.0f, worldLower, worldUpper);

		totalBounds = Union(totalBounds, Bounds(worldLower, worldUpper));
	}

	totalLower = totalBounds.lower;
	totalUpper = totalBounds.upper;
}