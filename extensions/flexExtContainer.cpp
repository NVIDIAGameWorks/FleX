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

#include <vector>
#include <limits>
#include <algorithm>

#include "../core/core.h"
#include "../core/maths.h"

#include "../include/NvFlex.h"
#include "../include/NvFlexExt.h"

class Bitmap
{
public:

	typedef unsigned int Word;

	static const int kWordSize = sizeof(Word)*8;

	Bitmap(int numBits) : mBits((numBits+kWordSize-1)/kWordSize)
	{
	}

	inline void Set(int bit)
	{
		const int wordIndex = bit/kWordSize;
		const int bitIndex = bit&(kWordSize-1);

		const Word word = mBits[wordIndex];

		mBits[wordIndex] = word|(1<<bitIndex);
	}

	inline void Reset(int bit)
	{
		const int wordIndex = bit/kWordSize;
		const int bitIndex = bit&(kWordSize-1);

		const Word word = mBits[wordIndex];

		mBits[wordIndex] = word&~(1<<bitIndex);
	}

	inline bool IsSet(int bit)
	{
		const int wordIndex = bit/kWordSize;
		const int bitIndex = bit&(kWordSize-1);

		const Word word = mBits[wordIndex];

		return (word & (1<<bitIndex)) != 0;
	}

private:

	std::vector<Word> mBits;
};


struct NvFlexExtContainer
{
	int mMaxParticles;
	
	NvFlexSolver* mSolver;
	NvFlexLibrary* mFlexLib;

	// first n indices 
	NvFlexVector<int> mActiveList;
	
	std::vector<int> mFreeList;
	std::vector<NvFlexExtInstance*> mInstances;

	std::vector<NvFlexExtSoftJoint*> mSoftJoints;

	// particles
	NvFlexVector<Vec4> mParticles;
	NvFlexVector<Vec4> mParticlesRest;
	NvFlexVector<Vec3> mVelocities;
	NvFlexVector<int> mPhases;
	NvFlexVector<Vec4> mNormals;	

	// shapes
	NvFlexVector<int> mShapeOffsets;
	NvFlexVector<int> mShapeIndices;
	NvFlexVector<float> mShapeCoefficients;
	NvFlexVector<float> mShapePlasticThresholds;
	NvFlexVector<float> mShapePlasticCreeps;
	NvFlexVector<Quat> mShapeRotations;
	NvFlexVector<Vec3> mShapeTranslations;
	NvFlexVector<Vec3> mShapeRestPositions;

	// springs
	NvFlexVector<int> mSpringIndices;
	NvFlexVector<float> mSpringLengths;
	NvFlexVector<float> mSpringCoefficients;

	// cloth
	NvFlexVector<int> mTriangleIndices;
	NvFlexVector<Vec3> mTriangleNormals;	

	NvFlexVector<int> mInflatableStarts;
	NvFlexVector<int> mInflatableCounts;	
	NvFlexVector<float> mInflatableRestVolumes;
	NvFlexVector<float> mInflatableCoefficients;
	NvFlexVector<float> mInflatableOverPressures;

	// bounds (CPU), stored in a vector to ensure transfers can happen asynchronously
	NvFlexVector<Vec3> mBoundsLower;
	NvFlexVector<Vec3> mBoundsUpper;

	// needs compact
	bool mNeedsCompact;
	// needs to update active list
	bool mNeedsActiveListRebuild;

	NvFlexExtContainer(NvFlexLibrary* l) :
		mMaxParticles(0), mSolver(NULL), mFlexLib(l),
		mActiveList(l),mParticles(l),mParticlesRest(l),mVelocities(l),
		mPhases(l),mNormals(l),mShapeOffsets(l),mShapeIndices(l),
		mShapeCoefficients(l),mShapePlasticThresholds(l),
		mShapePlasticCreeps(l),mShapeRotations(l),mShapeTranslations(l),
		mShapeRestPositions(l),mSpringIndices(l),mSpringLengths(l),
		mSpringCoefficients(l),mTriangleIndices(l),mTriangleNormals(l),
		mInflatableStarts(l),mInflatableCounts(l),mInflatableRestVolumes(l),
		mInflatableCoefficients(l),mInflatableOverPressures(l), mBoundsLower(l), mBoundsUpper(l),
		mNeedsCompact(false), mNeedsActiveListRebuild(false)
	{}
};



namespace
{

// compacts all constraints into linear arrays
void CompactObjects(NvFlexExtContainer* c)
{
	int totalNumSprings = 0;
	int totalNumTris = 0;
	int totalNumShapes = 0;
	int totalNumShapeIndices = 0;

	bool plasticDeformation = false;

	// pre-calculate array sizes
	for (size_t i = 0; i < c->mInstances.size(); ++i)
	{
		NvFlexExtInstance* inst = c->mInstances[i];

		const NvFlexExtAsset* asset = inst->asset;

		// index into the triangle array for this instance
		inst->triangleIndex = totalNumTris;

		totalNumSprings += asset->numSprings;
		totalNumTris += asset->numTriangles;

		totalNumShapeIndices += asset->numShapeIndices;
		totalNumShapes += asset->numShapes;

		if (asset->shapePlasticThresholds && asset->shapePlasticCreeps)
		{ 
			plasticDeformation = true;
		}
	}

	// each joint corresponds to one shape matching constraint
	for (size_t i = 0; i < c->mSoftJoints.size(); ++i)
	{
		const NvFlexExtSoftJoint* joint = c->mSoftJoints[i];

		totalNumShapeIndices += joint->numParticles;
		++totalNumShapes;
	}

	//----------------------
	// map buffers

	// springs
	c->mSpringIndices.map();
	c->mSpringLengths.map();
	c->mSpringCoefficients.map();

	// cloth
	c->mTriangleIndices.map();
	c->mTriangleNormals.map();

	// inflatables
	c->mInflatableStarts.map();
	c->mInflatableCounts.map();
	c->mInflatableRestVolumes.map();
	c->mInflatableCoefficients.map();
	c->mInflatableOverPressures.map();

	// shapes
	c->mShapeIndices.map();
	c->mShapeRestPositions.map();
	c->mShapeOffsets.map();
	c->mShapeCoefficients.map();

	c->mShapePlasticThresholds.map();
	c->mShapePlasticCreeps.map();

	c->mShapeTranslations.map();
	c->mShapeRotations.map();

	//----------------------
	// resize buffers

	// springs
	c->mSpringIndices.resize(totalNumSprings * 2);
	c->mSpringLengths.resize(totalNumSprings);
	c->mSpringCoefficients.resize(totalNumSprings);

	// cloth
	c->mTriangleIndices.resize(totalNumTris * 3);
	c->mTriangleNormals.resize(totalNumTris);

	// inflatables
	c->mInflatableStarts.resize(0);
	c->mInflatableCounts.resize(0);
	c->mInflatableRestVolumes.resize(0);
	c->mInflatableCoefficients.resize(0);
	c->mInflatableOverPressures.resize(0);

	// shapes
	c->mShapeIndices.resize(totalNumShapeIndices);
	c->mShapeRestPositions.resize(totalNumShapeIndices);
	c->mShapeOffsets.resize(1 + totalNumShapes);
	c->mShapeCoefficients.resize(totalNumShapes);

	if (plasticDeformation) 
	{
		c->mShapePlasticThresholds.resize(totalNumShapes);
		c->mShapePlasticCreeps.resize(totalNumShapes);
	}
	else
	{
		c->mShapePlasticThresholds.resize(0);
		c->mShapePlasticCreeps.resize(0);
	}

	c->mShapeTranslations.resize(totalNumShapes);
	c->mShapeRotations.resize(totalNumShapes);

	int* __restrict dstSpringIndices = (totalNumSprings) ? &c->mSpringIndices[0] : NULL;
	float* __restrict dstSpringLengths = (totalNumSprings) ? &c->mSpringLengths[0] : NULL;
	float* __restrict dstSpringCoefficients = (totalNumSprings) ? &c->mSpringCoefficients[0] : NULL;

	int* __restrict dstTriangleIndices = (totalNumTris) ? &c->mTriangleIndices[0] : NULL;

	int* __restrict dstShapeIndices = (totalNumShapeIndices) ? &c->mShapeIndices[0] : NULL;
	Vec3* __restrict dstShapeRestPositions = (totalNumShapeIndices) ? &c->mShapeRestPositions[0] : NULL;
	int* __restrict dstShapeOffsets = (totalNumShapes) ? &c->mShapeOffsets[0] : NULL;
	float* __restrict dstShapeCoefficients = (totalNumShapes) ? &c->mShapeCoefficients[0] : NULL;
	float* __restrict dstShapePlasticThresholds = NULL;
	float* __restrict dstShapePlasticCreeps = NULL;
	if (plasticDeformation)
	{
		dstShapePlasticThresholds = (totalNumShapes) ? &c->mShapePlasticThresholds[0] : NULL;
		dstShapePlasticCreeps = (totalNumShapes) ? &c->mShapePlasticCreeps[0] : NULL;
	}
	Vec3* __restrict dstShapeTranslations = (totalNumShapes) ? &c->mShapeTranslations[0] : NULL;
	Quat* __restrict dstShapeRotations = (totalNumShapes) ? &c->mShapeRotations[0] : NULL;

	// push leading zero if necessary
	if (totalNumShapes != 0)
	{
		*dstShapeOffsets = 0;
		++dstShapeOffsets;
	}

	int shapeIndexOffset = 0;
	int shapeIndex = 0;

	// go through each instance and update springs, shapes, etc
	for (size_t i = 0; i < c->mInstances.size(); ++i)
	{
		NvFlexExtInstance* inst = c->mInstances[i];

		const NvFlexExtAsset* asset = inst->asset;

		// map indices from the asset to the instance
		const int* __restrict remap = &inst->particleIndices[0];

		// flatten spring data
		int numSprings = asset->numSprings;
		const int numSpringIndices = asset->numSprings * 2;
		const int* __restrict srcSpringIndices = asset->springIndices;

		for (int i = 0; i < numSpringIndices; ++i)
		{
			*dstSpringIndices = remap[*srcSpringIndices];

			++dstSpringIndices;
			++srcSpringIndices;
		}

		memcpy(dstSpringLengths, asset->springRestLengths, numSprings*sizeof(float));
		memcpy(dstSpringCoefficients, asset->springCoefficients, numSprings*sizeof(float));

		dstSpringLengths += numSprings;
		dstSpringCoefficients += numSprings;

		// shapes
		if (asset->numShapes)
		{
			const int indexOffset = shapeIndexOffset;

			// store start index into shape array
			inst->shapeIndex = shapeIndex;

			int shapeStart = 0;

			for (int s=0; s < asset->numShapes; ++s)
			{
				dstShapeOffsets[shapeIndex] = asset->shapeOffsets[s] + indexOffset;
				dstShapeCoefficients[shapeIndex]  = asset->shapeCoefficients[s];
				if (plasticDeformation)
				{
					if (asset->shapePlasticThresholds)
						dstShapePlasticThresholds[shapeIndex] = asset->shapePlasticThresholds[s];
					else
						dstShapePlasticThresholds[shapeIndex] = 0.0f;

					if (asset->shapePlasticCreeps)
						dstShapePlasticCreeps[shapeIndex]  = asset->shapePlasticCreeps[s];
					else
						dstShapePlasticCreeps[shapeIndex] = 0.0f;
				}
				dstShapeTranslations[shapeIndex]  = Vec3(&inst->shapeTranslations[s*3]);
				dstShapeRotations[shapeIndex]  = Quat(&inst->shapeRotations[s*4]);

				++shapeIndex;

				const int shapeEnd = asset->shapeOffsets[s];

				for (int i=shapeStart; i < shapeEnd; ++i)
				{
					const int currentParticle = asset->shapeIndices[i];

					// remap indices and create local space positions for each shape
					dstShapeRestPositions[shapeIndexOffset] = Vec3(&asset->particles[currentParticle*4]) - Vec3(&asset->shapeCenters[s*3]);
					dstShapeIndices[shapeIndexOffset] = remap[asset->shapeIndices[i]];

					++shapeIndexOffset;
				}

				shapeStart = shapeEnd;
			}		
		}

		if (asset->numTriangles)
		{
			// triangles
			const int numTriIndices = asset->numTriangles * 3;
			const int* __restrict srcTriIndices = asset->triangleIndices;

			for (int i = 0; i < numTriIndices; ++i)
			{
				*dstTriangleIndices = remap[*srcTriIndices];

				++dstTriangleIndices;
				++srcTriIndices;
			}

			if (asset->inflatable)
			{
				c->mInflatableStarts.push_back(inst->triangleIndex);
				c->mInflatableCounts.push_back(asset->numTriangles);
				c->mInflatableRestVolumes.push_back(asset->inflatableVolume);
				c->mInflatableCoefficients.push_back(asset->inflatableStiffness);
				c->mInflatableOverPressures.push_back(asset->inflatablePressure);
			}
		}
	}


	// go through each joint and add shape matching constraint to the solver
	for (size_t i = 0; i < c->mSoftJoints.size(); ++i)
	{
		NvFlexExtSoftJoint* joint = c->mSoftJoints[i];
		const int numJointParticles = joint->numParticles;

		// store start index into shape array
		joint->shapeIndex = shapeIndex;

		const int offset = dstShapeOffsets[shapeIndex - 1];
		dstShapeOffsets[shapeIndex] = offset + numJointParticles;

		for (int i = 0; i < numJointParticles; ++i)
		{
			dstShapeIndices[shapeIndexOffset] = joint->particleIndices[i];
			dstShapeRestPositions[shapeIndexOffset] = Vec3(joint->particleLocalPositions[3 * i + 0], joint->particleLocalPositions[3 * i + 1], joint->particleLocalPositions[3 * i + 2]);

			++shapeIndexOffset;
		}

		dstShapeTranslations[shapeIndex] = Vec3(joint->shapeTranslations);
		dstShapeRotations[shapeIndex] = Quat(joint->shapeRotations);
		dstShapeCoefficients[shapeIndex] = joint->stiffness;

		++shapeIndex;
	}

	//----------------------
	// unmap buffers

	// springs
	c->mSpringIndices.unmap();
	c->mSpringLengths.unmap();
	c->mSpringCoefficients.unmap();

	// cloth
	c->mTriangleIndices.unmap();
	c->mTriangleNormals.unmap();

	// inflatables
	c->mInflatableStarts.unmap();
	c->mInflatableCounts.unmap();
	c->mInflatableRestVolumes.unmap();
	c->mInflatableCoefficients.unmap();
	c->mInflatableOverPressures.unmap();

	// shapes
	c->mShapeIndices.unmap();
	c->mShapeRestPositions.unmap();
	c->mShapeOffsets.unmap();
	c->mShapeCoefficients.unmap();

	c->mShapePlasticThresholds.unmap();
	c->mShapePlasticCreeps.unmap();

	c->mShapeTranslations.unmap();
	c->mShapeRotations.unmap();

	// ----------------------
	// Flex update

	// springs
	if (c->mSpringLengths.size())
		NvFlexSetSprings(c->mSolver, c->mSpringIndices.buffer, c->mSpringLengths.buffer, c->mSpringCoefficients.buffer, int(c->mSpringLengths.size()));
	else
		NvFlexSetSprings(c->mSolver, NULL, NULL, NULL, 0);

	// shapes
	if (c->mShapeCoefficients.size())
	{
		NvFlexSetRigids(c->mSolver, c->mShapeOffsets.buffer, c->mShapeIndices.buffer, c->mShapeRestPositions.buffer, NULL, c->mShapeCoefficients.buffer, c->mShapePlasticThresholds.buffer, c->mShapePlasticCreeps.buffer, c->mShapeRotations.buffer, c->mShapeTranslations.buffer, int(c->mShapeCoefficients.size()), c->mShapeIndices.size());
	}
	else
	{
		c->mShapeRotations.resize(0);
		c->mShapeTranslations.resize(0);

		NvFlexSetRigids(c->mSolver, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0);		
	}

	// triangles
	if (c->mTriangleIndices.size())
		NvFlexSetDynamicTriangles(c->mSolver, c->mTriangleIndices.buffer, NULL, int(c->mTriangleIndices.size()/3));
	else
		NvFlexSetDynamicTriangles(c->mSolver, NULL, NULL, 0);

	// inflatables
	if (c->mInflatableCounts.size())
		NvFlexSetInflatables(c->mSolver, c->mInflatableStarts.buffer, c->mInflatableCounts.buffer, c->mInflatableRestVolumes.buffer, c->mInflatableOverPressures.buffer, c->mInflatableCoefficients.buffer, int(c->mInflatableCounts.size()));
	else
		NvFlexSetInflatables(c->mSolver, NULL, NULL, NULL, NULL, NULL, 0);

	c->mNeedsCompact = false;

}

} // anonymous namespace


NvFlexExtContainer* NvFlexExtCreateContainer(NvFlexLibrary* flexLib, NvFlexSolver* solver, int maxParticles)
{
	NvFlexExtContainer* c = new NvFlexExtContainer(flexLib);

	c->mSolver = solver;
	c->mFlexLib = flexLib;
	c->mMaxParticles = maxParticles;

	// initialize free list
	c->mFreeList.resize(maxParticles);
	for (int i=0; i < maxParticles; ++i)
		c->mFreeList[i] = i;

	c->mActiveList.init(maxParticles);
	c->mParticles.init(maxParticles);
	c->mParticlesRest.init(maxParticles);
	c->mVelocities.init(maxParticles);
	c->mPhases.init(maxParticles);
	c->mNormals.init(maxParticles);


	// ensure we have the corerct CUDA context set
	NvFlexAcquireContext(flexLib);

	c->mBoundsLower.init(1);
	c->mBoundsUpper.init(1);

	NvFlexRestoreContext(flexLib);

	c->mNeedsCompact = false;

	return c;
}

void NvFlexExtDestroyContainer(NvFlexExtContainer* c)
{
	// ensure we have the corerct CUDA context set
	NvFlexLibrary* lib = c->mFlexLib;

	NvFlexAcquireContext(lib);

	delete c;

	NvFlexRestoreContext(lib);

}

int NvFlexExtAllocParticles(NvFlexExtContainer* c, int n, int* indices)
{
	const int numToAlloc = Min(int(c->mFreeList.size()), n);
	const int start = int(c->mFreeList.size())-numToAlloc;

	if (numToAlloc)
	{
		memcpy(indices, &c->mFreeList[start], numToAlloc*sizeof(int));
		c->mFreeList.resize(start);
	}

	c->mNeedsActiveListRebuild = true;

	return numToAlloc;
}

void NvFlexExtFreeParticles(NvFlexExtContainer* c, int n, const int* indices)
{
#if _DEBUG
	for (int i=0; i < n; ++i)
	{
		// check valid values
		assert(indices[i] >= 0 && indices[i] < int(c->mFreeList.capacity()));

		// check for double delete
		assert(std::find(c->mFreeList.begin(), c->mFreeList.end(), indices[i]) == c->mFreeList.end());
	}
#endif

	c->mFreeList.insert(c->mFreeList.end(), indices, indices+n);

	c->mNeedsActiveListRebuild = true;
}

int NvFlexExtGetActiveList(NvFlexExtContainer* c, int* indices)
{
	int count = 0;

	Bitmap inactive(c->mMaxParticles);

	// create bitmap
	for (size_t i=0; i < c->mFreeList.size(); ++i)
	{
		// if this fires then somehow a duplicate has ended up in the free list (double delete)
		assert(!inactive.IsSet(c->mFreeList[i]));

		inactive.Set(c->mFreeList[i]);
	}

	// iterate bitmap to find active elements
	for (int i=0; i < c->mMaxParticles; ++i)
		if (inactive.IsSet(i) == false)
			indices[count++] = i;

	return count;
}

NvFlexExtParticleData NvFlexExtMapParticleData(NvFlexExtContainer* c)
{
	NvFlexExtParticleData data;

	c->mParticles.map();
	c->mParticlesRest.map();
	c->mVelocities.map();
	c->mPhases.map();
	c->mNormals.map();

	c->mBoundsLower.map();
	c->mBoundsUpper.map();

	if (c->mParticles.size())
		data.particles = (float*)&c->mParticles[0];

	if (c->mParticlesRest.size())
		data.restParticles = (float*)&c->mParticlesRest[0];

	if (c->mVelocities.size())
		data.velocities = (float*)&c->mVelocities[0];

	if (c->mPhases.size())
		data.phases = (int*)&c->mPhases[0];

	if (c->mNormals.size())
		data.normals = (float*)&c->mNormals[0];

	data.lower = c->mBoundsLower[0];
	data.upper = c->mBoundsUpper[0];

	return data;
}

void NvFlexExtUnmapParticleData(NvFlexExtContainer*c)
{
	c->mParticles.unmap();
	c->mParticlesRest.unmap();
	c->mVelocities.unmap();
	c->mPhases.unmap();
	c->mNormals.unmap();

	c->mBoundsLower.unmap();
	c->mBoundsUpper.unmap();
}

NvFlexExtTriangleData NvFlexExtMapTriangleData(NvFlexExtContainer* c)
{
	NvFlexExtTriangleData data;

	c->mTriangleIndices.map();
	c->mTriangleNormals.map();
	
	if (c->mTriangleIndices.size())
		data.indices = &c->mTriangleIndices[0];
	
	if (c->mTriangleNormals.size())
		data.normals = (float*)&c->mTriangleNormals[0];

	return data;
}

void NvFlexExtUnmapTriangleData(NvFlexExtContainer* c)
{	
	c->mTriangleIndices.unmap();
	c->mTriangleNormals.unmap();
}

NvFlexExtShapeData NvFlexExtMapShapeData(NvFlexExtContainer* c)
{
	NvFlexExtShapeData data;

	c->mShapeRotations.map();
	c->mShapeTranslations.map();

	if (c->mShapeRotations.size())
		data.rotations = (float*)&c->mShapeRotations[0];

	if (c->mShapeTranslations.size())
		data.positions = (float*)&c->mShapeTranslations[0];

	return data;
}

void NvFlexExtUnmapShapeData(NvFlexExtContainer* c)
{
	c->mShapeRotations.unmap();
	c->mShapeTranslations.unmap();
}


NvFlexExtInstance* NvFlexExtCreateInstance(NvFlexExtContainer* c, NvFlexExtParticleData* particleData, const NvFlexExtAsset* asset, const float* transform, float vx, float vy, float vz, int phase, float invMassScale)
{	
	const int numParticles = asset->numParticles;

	// check if asset will fit
	if (int(c->mFreeList.size()) < numParticles)
		return NULL;

	NvFlexExtInstance* inst = new NvFlexExtInstance();

	inst->asset = asset;
	inst->triangleIndex = -1;
	inst->shapeIndex = -1;
	inst->inflatableIndex = -1;
	inst->userData = NULL;
	inst->numParticles = numParticles;
	
	assert(inst->numParticles <= asset->maxParticles);

	// allocate particles for instance
	inst->particleIndices = new int[asset->maxParticles];
	int n = NvFlexExtAllocParticles(c, numParticles, &inst->particleIndices[0]);
	assert(n == numParticles);
	(void)n;

	c->mInstances.push_back(inst);

	const Matrix44 xform(transform);

	for (int i=0; i < numParticles; ++i)
	{
		const int index = inst->particleIndices[i];

		// add transformed particles to the locked particle data
		((Vec4*)(particleData->particles))[index] = xform*Vec4(Vec3(&asset->particles[i*4]), 1.0f);
		((Vec4*)(particleData->particles))[index].w = asset->particles[i*4+3]*invMassScale;
		((Vec4*)(particleData->restParticles))[index] = Vec4(&asset->particles[i*4]);

		((Vec3*)(particleData->velocities))[index] = Vec3(vx, vy, vz);
		((int*)(particleData->phases))[index] = phase;
		((Vec4*)(particleData->normals))[index] = Vec4(0.0f);
	}

	const int numShapes = asset->numShapes;

	// allocate memory for shape transforms
	Vec3* shapeTranslations = new Vec3[numShapes];
	Quat* shapeRotations = new Quat[numShapes];

	Quat rotation = Quat(Matrix33(xform.GetAxis(0), xform.GetAxis(1), xform.GetAxis(2)));

	for (int i=0; i < numShapes; ++i)
	{
		shapeTranslations[i] = Vec3(xform*Vec4(asset->shapeCenters[i*3+0], asset->shapeCenters[i*3+1], asset->shapeCenters[i*3+2], 1.0));
		shapeRotations[i] = rotation;
	}

	inst->shapeTranslations = (float*)shapeTranslations;
	inst->shapeRotations = (float*)shapeRotations;

	// mark container as dirty
	c->mNeedsCompact = true;
	c->mNeedsActiveListRebuild = true;

	return inst;
}

void NvFlexExtDestroyInstance(NvFlexExtContainer* c, const NvFlexExtInstance* inst)
{
	NvFlexExtFreeParticles(c, inst->numParticles, &inst->particleIndices[0]);
	delete[] inst->particleIndices;

	delete[] inst->shapeRotations;
	delete[] inst->shapeTranslations;

	// TODO: O(N) remove
	std::vector<NvFlexExtInstance*>::iterator iter = std::find(c->mInstances.begin(), c->mInstances.end(), inst);
	assert(iter != c->mInstances.end());
	c->mInstances.erase(iter);

	c->mNeedsCompact = true;
	c->mNeedsActiveListRebuild = true;

	delete inst;
}

void NvFlexExtTickContainer(NvFlexExtContainer* c, float dt, int substeps, bool enableTiming)
{
	// update the device
	NvFlexExtPushToDevice(c);

	// update solver
	NvFlexUpdateSolver(c->mSolver, dt, substeps, enableTiming);

	// update host
	NvFlexExtPullFromDevice(c);
}

void NvFlexExtNotifyAssetChanged(NvFlexExtContainer* c, const NvFlexExtAsset* asset)
{
	c->mNeedsCompact = true;
}

void NvFlexExtPushToDevice(NvFlexExtContainer* c)
{
	if (c->mNeedsActiveListRebuild)
	{
		// update active list
		c->mActiveList.map();
		int n = NvFlexExtGetActiveList(c, &c->mActiveList[0]);		
		c->mActiveList.unmap();

		NvFlexSetActive(c->mSolver, c->mActiveList.buffer, NULL);
		NvFlexSetActiveCount(c->mSolver, n);

		c->mNeedsActiveListRebuild = false;
	}

	// push any changes to solver
	NvFlexSetParticles(c->mSolver, c->mParticles.buffer, NULL);
	NvFlexSetRestParticles(c->mSolver, c->mParticlesRest.buffer, NULL);

	NvFlexSetVelocities(c->mSolver, c->mVelocities.buffer, NULL);
	NvFlexSetPhases(c->mSolver, c->mPhases.buffer, NULL);
	NvFlexSetNormals(c->mSolver, c->mNormals.buffer, NULL);
	
	if (c->mNeedsCompact)
		CompactObjects(c);
}

void NvFlexExtPullFromDevice(NvFlexExtContainer* c)
{
	// read back particle data
	NvFlexGetParticles(c->mSolver, c->mParticles.buffer, NULL);
	NvFlexGetVelocities(c->mSolver, c->mVelocities.buffer, NULL);
	NvFlexGetPhases(c->mSolver, c->mPhases.buffer, NULL);
	NvFlexGetNormals(c->mSolver, c->mNormals.buffer, NULL);
	NvFlexGetBounds(c->mSolver, c->mBoundsLower.buffer, c->mBoundsUpper.buffer);

	// read back shape transforms
	if (c->mShapeCoefficients.size())
		NvFlexGetRigids(c->mSolver, NULL, NULL, NULL, NULL, NULL, NULL, NULL, c->mShapeRotations.buffer, c->mShapeTranslations.buffer);
}

void NvFlexExtUpdateInstances(NvFlexExtContainer* c)
{
	c->mShapeTranslations.map();
	c->mShapeRotations.map();

	for (int i=0; i < int(c->mInstances.size()); ++i)
	{
		NvFlexExtInstance* inst = c->mInstances[i];

		// copy data back to per-instance memory from the container's memory
		const int numShapes = inst->asset->numShapes;
		const int shapeStart = inst->shapeIndex;

		if (shapeStart == -1)
			continue;

		for (int s=0; s < numShapes; ++s)
		{
			((Vec3*)inst->shapeTranslations)[s] = c->mShapeTranslations[shapeStart + s];
			((Quat*)inst->shapeRotations)[s] = c->mShapeRotations[shapeStart + s];
		}		
	}

	for (int i = 0; i < int(c->mSoftJoints.size()); ++i)
	{
		NvFlexExtSoftJoint* joint = c->mSoftJoints[i];

		const int shapeStart = joint->shapeIndex;

		// Here we compute the COM only once instead of in NvFlexExtCreateSoftJoint() to avoid buffer mapping issue
		if (!joint->initialized)
		{
			// Calculate the center of mass of the new shape matching constraint given a set of joint particles and its indices
			// To improve the accuracy of the result, first transform the particlePosition to relative coordinates (by finding the mean and subtracting that from all positions)
			// Note: If this is not done, one might see ghost forces if the mean of the particlePosition is far from the origin.
			Vec3 shapeOffset(0.0f);
			for (int i = 0; i < joint->numParticles; ++i)
			{
				const Vec4 particlePosition = c->mParticles[joint->particleIndices[i]];
				shapeOffset += Vec3(particlePosition);
			}
			shapeOffset /= float(joint->numParticles);

			Vec3 com;
			for (int i = 0; i < joint->numParticles; ++i)
			{
				const Vec4 particlePosition = c->mParticles[joint->particleIndices[i]];

				// By subtracting shapeOffset the calculation is done in relative coordinates
				com += Vec3(particlePosition) - shapeOffset;
			}
			com /= float(joint->numParticles);

			// Add the shapeOffset to switch back to absolute coordinates
			com += shapeOffset;

			// update per-joint shapeTranslations and copy to the container's memory
			joint->shapeTranslations[0] = com.x;
			joint->shapeTranslations[1] = com.y;
			joint->shapeTranslations[2] = com.z;

			joint->initialized = true;		// Complete joint initilization process 
		}
		else
		{
			joint->shapeTranslations[0] = c->mShapeTranslations[shapeStart].x;
			joint->shapeTranslations[1] = c->mShapeTranslations[shapeStart].y;
			joint->shapeTranslations[2] = c->mShapeTranslations[shapeStart].z;
		}

		// copy data back to per-joint memory from the container's memory
		joint->shapeRotations[0] = c->mShapeRotations[shapeStart].x;
		joint->shapeRotations[1] = c->mShapeRotations[shapeStart].y;
		joint->shapeRotations[2] = c->mShapeRotations[shapeStart].z;
		joint->shapeRotations[3] = c->mShapeRotations[shapeStart].w;
	}

	c->mShapeTranslations.unmap();
	c->mShapeRotations.unmap();
}

void NvFlexExtDestroyAsset(NvFlexExtAsset* asset)
{
	delete[] asset->particles;
	delete[] asset->springIndices;
	delete[] asset->springCoefficients;
	delete[] asset->springRestLengths;
	delete[] asset->triangleIndices;
	delete[] asset->shapeIndices;
	delete[] asset->shapeOffsets;
	delete[] asset->shapeCenters;
	delete[] asset->shapeCoefficients;	
	delete[] asset->shapePlasticThresholds;	
	delete[] asset->shapePlasticCreeps;	

	delete asset;
}

NvFlexExtSoftJoint* NvFlexExtCreateSoftJoint(NvFlexExtContainer* c, const int* particleIndices, const float* particleLocalPositions, const int numJointParticles, const float stiffness)
{
	NvFlexExtSoftJoint* joint = new NvFlexExtSoftJoint();

	joint->particleIndices = new int[numJointParticles];
	memcpy(joint->particleIndices, particleIndices, sizeof(int) * numJointParticles);

	joint->particleLocalPositions = new float[3 * numJointParticles];
	memcpy(joint->particleLocalPositions, particleLocalPositions, 3 * sizeof(float)*numJointParticles);

	// initialize with Quat()
	joint->shapeRotations[0] = Quat().x;
	joint->shapeRotations[1] = Quat().y;
	joint->shapeRotations[2] = Quat().z;
	joint->shapeRotations[3] = Quat().w;

	joint->numParticles = numJointParticles;
	joint->stiffness = stiffness;
	joint->initialized = false;		// Initialization will be fully completed in NvFlexExtUpdateInstances()

	c->mSoftJoints.push_back(joint);

	// mark container as dirty
	c->mNeedsCompact = true;

	return joint;
}

void NvFlexExtDestroySoftJoint(NvFlexExtContainer* c, NvFlexExtSoftJoint* joint)
{
	delete[] joint->particleIndices;
	delete[] joint->particleLocalPositions;

	// TODO: O(N) remove
	std::vector<NvFlexExtSoftJoint*>::iterator iter = std::find(c->mSoftJoints.begin(), c->mSoftJoints.end(), joint);
	assert(iter != c->mSoftJoints.end());
	c->mSoftJoints.erase(iter);

	c->mNeedsCompact = true;

	delete joint;
}

void NvFlexExtSoftJointSetTransform(NvFlexExtContainer* c, NvFlexExtSoftJoint* joint, const float* newPosition, const float* newRotation)
{
	// calculate transform from old position to new position
	Matrix44 LocalFromOld = AffineInverse(TranslationMatrix(Point3(joint->shapeTranslations))*RotationMatrix(joint->shapeRotations));
	Matrix44 NewFromLocal = TranslationMatrix(Point3(newPosition))*RotationMatrix(newRotation);
	Matrix44 transform = NewFromLocal*LocalFromOld;

	// transform soft joint particles to new location

	//----------------------
	// map buffers
	c->mParticles.map();

	for (int i = 0; i < joint->numParticles; ++i)
	{
		const Vec3 particlePosition = Vec3(c->mParticles[joint->particleIndices[i]]);
		Vec4 particleNewPostion = transform * Vec4(particlePosition, 1.0f);

		// update soft joint particles
		c->mParticles[joint->particleIndices[i]].x = particleNewPostion.x;
		c->mParticles[joint->particleIndices[i]].y = particleNewPostion.y;
		c->mParticles[joint->particleIndices[i]].z = particleNewPostion.z;
	}

	joint->shapeTranslations[0] = newPosition[0];
	joint->shapeTranslations[1] = newPosition[1];
	joint->shapeTranslations[2] = newPosition[2];

	joint->shapeRotations[0] = newRotation[0];
	joint->shapeRotations[1] = newRotation[1];
	joint->shapeRotations[2] = newRotation[2];
	joint->shapeRotations[3] = newRotation[3];

	//----------------------
	// unmap buffers
	c->mParticles.unmap();
}


