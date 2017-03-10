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

#include "../include/NvFlexExt.h"

#include "../core/cloth.h"

namespace
{
	struct Key
	{
		Key(int i, float d) : index(i), depth(d) {}

		int index;
		float depth;
		
		bool operator < (const Key& rhs) const { return depth < rhs.depth; }
	};
}

int NvFlexExtCreateWeldedMeshIndices(const float* vertices, int numVertices, int* uniqueIndices, int* originalToUniqueMap, float threshold)
{
	memset(originalToUniqueMap, -1, numVertices*sizeof(int));

	const Vec3* positions = (const Vec3*)vertices;

	// use a sweep and prune style search to accelerate neighbor finding
	std::vector<Key> keys;
	for (int i=0; i < numVertices; i++)
		keys.push_back(Key(i, positions[i].z));

	std::sort(keys.begin(), keys.end());

	int uniqueCount = 0;

	// sweep keys to find matching verts
	for (int i=0; i < numVertices; ++i)
	{
		// we are a duplicate, skip
		if (originalToUniqueMap[keys[i].index] != -1)
			continue;

		// scan forward until no vertex can be closer than threshold
		for (int j=i+1; j < numVertices && (keys[j].depth-keys[i].depth) <= threshold; ++j)
		{
			float distance = Length(Vector3(positions[keys[i].index])-Vector3(positions[keys[j].index]));

			if (distance <= threshold)
				originalToUniqueMap[keys[j].index] = uniqueCount;
		}

		originalToUniqueMap[keys[i].index] = uniqueCount;

		uniqueIndices[uniqueCount++] = keys[i].index;
	}

	return uniqueCount;
}

NvFlexExtAsset* NvFlexExtCreateClothFromMesh(const float* particles, int numVertices, const int* indices, int numTriangles, float stretchStiffness, float bendStiffness, float tetherStiffness, float tetherGive, float pressure)
{
	NvFlexExtAsset* asset = new NvFlexExtAsset();
	memset(asset, 0, sizeof(*asset));

	asset->particles = new float[numVertices*4];
	memcpy(asset->particles, particles, numVertices*sizeof(float)*4);	

	asset->triangleIndices = new int[numTriangles*3];
	memcpy(asset->triangleIndices, indices, numTriangles*3*sizeof(int));

	asset->numParticles = numVertices;
	asset->maxParticles = numVertices;

	asset->numTriangles = numTriangles;

	// create cloth mesh
	ClothMesh cloth((Vec4*)particles, numVertices, indices, numTriangles*3, stretchStiffness, bendStiffness, true);

	if (cloth.mValid)
	{
		// create tethers
		if (tetherStiffness > 0.0f)
		{
			std::vector<int> anchors;
			anchors.reserve(numVertices);

			// find anchors
			for (int i=0; i < numVertices; ++i)
			{
				Vec4& particle = ((Vec4*)particles)[i];

				if (particle.w == 0.0f)
					anchors.push_back(i);
			}

			if (anchors.size())
			{
				// create tethers
				for (int i=0; i < numVertices; ++i)
				{
					Vec4& particle = ((Vec4*)particles)[i];
					if (particle.w == 0.0f)
						continue;

					float minSqrDist = FLT_MAX;
					int minIndex = -1;

					// find the closest attachment point
					for (int a=0; a < int(anchors.size()); ++a)
					{
						Vec4& attachment = ((Vec4*)particles)[anchors[a]];

						float distSqr = LengthSq(Vec3(particle)-Vec3(attachment));
						if (distSqr < minSqrDist)
						{
							minSqrDist = distSqr;
							minIndex = anchors[a];
						}
					}

					// add a tether
					if (minIndex != -1)
					{						
						cloth.mConstraintIndices.push_back(i);
						cloth.mConstraintIndices.push_back(minIndex);
						cloth.mConstraintRestLengths.push_back(sqrtf(minSqrDist)*(1.0f + tetherGive));						
						
						// negative stiffness indicates tether (unilateral constraint)
						cloth.mConstraintCoefficients.push_back(-tetherStiffness);
					}
				}
			}
		}

		const int numSprings = int(cloth.mConstraintCoefficients.size());

		asset->springIndices = new int[numSprings*2];
		asset->springCoefficients = new float[numSprings];
		asset->springRestLengths = new float[numSprings];
		asset->numSprings = numSprings;

		for (int i=0; i < numSprings; ++i)
		{
			asset->springIndices[i*2+0] = cloth.mConstraintIndices[i*2+0];
			asset->springIndices[i*2+1] = cloth.mConstraintIndices[i*2+1];			
			asset->springRestLengths[i] = cloth.mConstraintRestLengths[i];
			asset->springCoefficients[i] = cloth.mConstraintCoefficients[i];
		}

		if (pressure > 0.0f)
		{
			asset->inflatable = true;
			asset->inflatableVolume = cloth.mRestVolume;
			asset->inflatableStiffness = cloth.mConstraintScale;
			asset->inflatablePressure = pressure;
		}
	}
	else
	{
		NvFlexExtDestroyAsset(asset);
		return NULL;
	}

	return asset;
}

struct FlexExtTearingClothAsset : public NvFlexExtAsset
{
	ClothMesh* mMesh;
};

NvFlexExtAsset* NvFlexExtCreateTearingClothFromMesh(const float* particles, int numParticles, int maxParticles, const int* indices, int numTriangles, float stretchStiffness, float bendStiffness, float pressure)
{
	FlexExtTearingClothAsset* asset = new FlexExtTearingClothAsset();
	memset(asset, 0, sizeof(*asset));

	asset->particles = new float[maxParticles*4];
	memcpy(asset->particles, particles, numParticles*sizeof(float)*4);	

	asset->triangleIndices = new int[numTriangles*3];
	memcpy(asset->triangleIndices, indices, numTriangles*3*sizeof(int));

	asset->numParticles = numParticles;
	asset->maxParticles = maxParticles;

	asset->numTriangles = numTriangles;
	
	// create and store cloth mesh
	asset->mMesh = new ClothMesh((Vec4*)particles, numParticles, indices, numTriangles*3, stretchStiffness, bendStiffness, true);
	
	ClothMesh& cloth = *asset->mMesh;

	if (cloth.mValid)
	{
		const int numSprings = int(cloth.mConstraintCoefficients.size());

		// asset references cloth mesh memory directly
		asset->springIndices = &cloth.mConstraintIndices[0];
		asset->springCoefficients = &cloth.mConstraintCoefficients[0];
		asset->springRestLengths = &cloth.mConstraintRestLengths[0];
		asset->numSprings = numSprings;

		if (pressure > 0.0f)
		{
			asset->inflatable = true;
			asset->inflatableVolume = cloth.mRestVolume;
			asset->inflatableStiffness = cloth.mConstraintScale;
			asset->inflatablePressure = pressure;
		}
	}
	else
	{
		NvFlexExtDestroyAsset(asset);
		return NULL;
	}

	return asset;

}

void NvFlexExtDestroyTearingCloth(NvFlexExtAsset* asset)
{
	FlexExtTearingClothAsset* tearable = (FlexExtTearingClothAsset*)asset;

	delete[] asset->particles;
	delete[] asset->triangleIndices;

	delete tearable->mMesh;
	delete tearable;
}

void NvFlexExtTearClothMesh(NvFlexExtAsset* asset, float maxStrain, int maxSplits, NvFlexExtTearingParticleClone* particleCopies, int* numParticleCopies,  int maxCopies, NvFlexExtTearingMeshEdit* triangleEdits, int* numTriangleEdits, int maxEdits) 
{
	FlexExtTearingClothAsset* tearable = (FlexExtTearingClothAsset*)asset;
	
	std::vector<ClothMesh::TriangleUpdate> edits;
	std::vector<ClothMesh::VertexCopy> copies;

	int splits = 0;

	maxCopies = Min(maxCopies, tearable->maxParticles-tearable->numParticles);

	// iterate over all edges and tear if beyond maximum strain
	for (int i=0; i < tearable->numSprings && int(copies.size()) < maxCopies && splits < maxSplits; ++i)
	{
		int a = tearable->springIndices[i*2+0];
		int b = tearable->springIndices[i*2+1];

		Vec3 p = Vec3(&tearable->particles[a*4]);
		Vec3 q = Vec3(&tearable->particles[b*4]);

		// check strain and break if greater than max threshold
		if (Length(p-q) > tearable->springRestLengths[i]*maxStrain)
		{
			// skip fixed particles
			if (Vec4(&tearable->particles[a*4]).w == 0.0f)
				continue;

			if (Vec4(&tearable->particles[b*4]).w == 0.0f)
				continue;

			// choose vertex of edge to split
			const int splitIndex = Randf() > 0.5f ? a : b;
			const Vec3 splitPlane = Normalize(p-q);	// todo: use plane perpendicular to normal and edge..

			std::vector<int> adjacentTriangles;
			std::vector<int> adjacentVertices;

			const int newIndex = tearable->mMesh->SplitVertex((Vec4*)tearable->particles, splitIndex, splitPlane, adjacentTriangles, adjacentVertices, edits, copies, maxCopies-int(copies.size()));

			if (newIndex != -1)
			{
				++splits;

				// separate each adjacent vertex if it is now singular
				for (int s=0; s < int(adjacentVertices.size()); ++s)
				{
					const int adjacentVertex = adjacentVertices[s];

					tearable->mMesh->SeparateVertex(adjacentVertex, edits, copies, maxCopies-int(copies.size()));
				}

				// also test the new vertex which can become singular
				tearable->mMesh->SeparateVertex(newIndex, edits, copies, maxCopies-int(copies.size()));
			}
		}
	}

	// update asset particle count
	tearable->numParticles = tearable->mMesh->mNumVertices;

	// output copies
	for (int c=0; c < int(copies.size()); ++c)
	{
		NvFlexExtTearingParticleClone clone;
		clone.srcIndex = copies[c].srcIndex;
		clone.destIndex = copies[c].destIndex;

		particleCopies[c] = clone;
	}

	// output mesh edits, note that some edits will not be reported if edit buffer is not big enough
	const int numEdits = Min(int(edits.size()), maxEdits);

	for (int u=0; u < numEdits; ++u)
	{
		NvFlexExtTearingMeshEdit update;
		update.triIndex = edits[u].triangle;
		update.newParticleIndex = edits[u].vertex;

		tearable->triangleIndices[update.triIndex] = update.newParticleIndex;

		triangleEdits[u] = update;
	}


	*numTriangleEdits = numEdits;
	*numParticleCopies = int(copies.size());
}
