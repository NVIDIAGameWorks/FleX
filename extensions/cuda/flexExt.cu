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

#include <cuda.h>
#include <cuda_runtime_api.h>

#include <vector>
#include <limits>
#include <algorithm>

#include "../../core/core.h"
#include "../../core/maths.h"

#include "../../include/NvFlex.h"
#include "../../include/NvFlexExt.h"

#define CudaCheck(x) { cudaError_t err = x; if (err != cudaSuccess) { printf("Cuda error: %d in %s at %s:%d\n", err, #x, __FILE__, __LINE__); assert(0); } }

static const int kNumThreadsPerBlock = 256;

struct NvFlexExtForceFieldCallback
{
	NvFlexExtForceFieldCallback(NvFlexSolver* solver) : mSolver(solver)
	{
		// force fields
		mForceFieldsCpu = NULL;
		mForceFieldsGpu = NULL;
		mMaxForceFields = 0;
		mNumForceFields = 0;

	}

	~NvFlexExtForceFieldCallback()
	{
		// force fields
		CudaCheck(cudaFreeHost(mForceFieldsCpu));
		CudaCheck(cudaFree(mForceFieldsGpu));
	}
	
	NvFlexExtForceField* mForceFieldsCpu;	// pinned host copy for async transfer
	NvFlexExtForceField* mForceFieldsGpu; // device copy

	int mMaxForceFields;
	int mNumForceFields;

	NvFlexSolver* mSolver;
};

NvFlexExtForceFieldCallback* NvFlexExtCreateForceFieldCallback(NvFlexSolver* solver)
{
	return new NvFlexExtForceFieldCallback(solver);	
}

void NvFlexExtDestroyForceFieldCallback(NvFlexExtForceFieldCallback* callback)
{
	delete callback;
}


__global__ void UpdateForceFields(int numParticles, const Vec4* __restrict__ positions, Vec4* __restrict__ velocities, const NvFlexExtForceField* __restrict__ forceFields, int numForceFields, float dt)
{
	const int i = blockIdx.x*blockDim.x + threadIdx.x;
	
	for (int f = 0; f < numForceFields; f++)
	{
		const NvFlexExtForceField& forceField = forceFields[f];

		if (i < numParticles)
		{
			const int index = i;

			Vec4 p = positions[index];
			Vec3 v = Vec3(velocities[index]);

			Vec3 localPos = Vec3(p.x, p.y, p.z) - Vec3(forceField.mPosition[0], forceField.mPosition[1], forceField.mPosition[2]);

			float length = Length(localPos);
			if (length >= forceField.mRadius)
			{
				continue;
			}
			
			Vec3 fieldDir;
			if (length > 0.0f)
			{
				fieldDir = localPos / length;
			}
			else
			{
				fieldDir = localPos;
			}

			// If using linear falloff, scale with distance.
			float fieldStrength = forceField.mStrength;
			if (forceField.mLinearFalloff)
			{
				fieldStrength *= (1.0f - (length / forceField.mRadius));
			}

			// Apply force
			Vec3 force = localPos * fieldStrength;

			float unitMultiplier;
			if (forceField.mMode == eNvFlexExtModeForce)
			{
				unitMultiplier = dt * p.w; // time/mass
			} 
			else if (forceField.mMode == eNvFlexExtModeImpulse)
			{
				unitMultiplier = p.w; // 1/mass
			}
			else if (forceField.mMode == eNvFlexExtModeVelocityChange)
			{
				unitMultiplier = 1.0f;
			}

			Vec3 deltaVelocity = fieldDir * fieldStrength * unitMultiplier;
			velocities[index] = Vec4(v + deltaVelocity, 0.0f);
		}
	}
}

void ApplyForceFieldsCallback(NvFlexSolverCallbackParams params)
{
	// callbacks always have the correct CUDA device set so we can safely launch kernels without acquiring

	NvFlexExtForceFieldCallback* c = (NvFlexExtForceFieldCallback*)params.userData;

	if (params.numActive && c->mNumForceFields)
	{
		const int kNumBlocks = (params.numActive+kNumThreadsPerBlock-1)/kNumThreadsPerBlock;

		UpdateForceFields<<<kNumBlocks, kNumThreadsPerBlock>>>(
			params.numActive,
	   		(Vec4*)params.particles,
	   		(Vec4*)params.velocities,
			c->mForceFieldsGpu,
			c->mNumForceFields,
			params.dt);
	}
}

void NvFlexExtSetForceFields(NvFlexExtForceFieldCallback* c, const NvFlexExtForceField* forceFields, int numForceFields)
{
	// re-alloc if necessary
	if (numForceFields > c->mMaxForceFields)
	{
		CudaCheck(cudaFreeHost(c->mForceFieldsCpu));
		CudaCheck(cudaMallocHost(&c->mForceFieldsCpu, sizeof(NvFlexExtForceField)*numForceFields));

		CudaCheck(cudaFree(c->mForceFieldsGpu));
		CudaCheck(cudaMalloc(&c->mForceFieldsGpu, sizeof(NvFlexExtForceField)*numForceFields));


		c->mMaxForceFields = numForceFields;
	}
	c->mNumForceFields = numForceFields;

	if (numForceFields > 0)
	{
		// copy to pinned host memory
		memcpy(c->mForceFieldsCpu, forceFields, numForceFields*sizeof(NvFlexExtForceField));

		cudaMemcpyKind kind = cudaMemcpyHostToDevice;
		CudaCheck(cudaMemcpyAsync(c->mForceFieldsGpu, &c->mForceFieldsCpu[0], numForceFields*sizeof(NvFlexExtForceField), kind, 0));
	}

	NvFlexSolverCallback callback;
	callback.function = ApplyForceFieldsCallback;
	callback.userData = c;
	
	// register a callback to calculate the forces at the end of the time-step
	NvFlexRegisterSolverCallback(c->mSolver, callback, eNvFlexStageUpdateEnd);
}
