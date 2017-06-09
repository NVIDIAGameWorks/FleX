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

#include "core/core.h"
#include "core/maths.h"

#include "include/NvFlex.h"
#include "include/NvFlexExt.h"

#include "src/dx/context/context.h"
#include "src/dx/context/device.h"

#include "flexExt_dx_common.h"

#include "shaders\flexExt.UpdateForceFields.h"


struct NvFlexExtForceFieldCallback
{
	NvFlexExtForceFieldCallback(NvFlexSolver* solver) : mSolver(solver)
	{
		// force fields
		mMaxForceFields = 0;
		mNumForceFields = 0;
		
		mForceFieldsGpu = NULL;

		mDevice = NULL;
		mContext = NULL;

		NvFlexLibrary* lib = NvFlexGetSolverLibrary(solver);
		NvFlexGetDeviceAndContext(lib, (void**)&mDevice, (void**)&mContext);

		{
			// force field shader
			NvFlex::ComputeShaderDesc desc{};
			desc.cs = (void*)g_flexExt_UpdateForceFields;
			desc.cs_length = sizeof(g_flexExt_UpdateForceFields);
			desc.label = L"NvFlexExtForceFieldCallback";
			desc.NvAPI_Slot = 0;

			mShaderUpdateForceFields = mContext->createComputeShader(&desc);
		}

		{
			// constant buffer
			NvFlex::ConstantBufferDesc desc;
			desc.stride = sizeof(int);
			desc.dim = 4;
			desc.uploadAccess = true;
			
			mConstantBuffer = mContext->createConstantBuffer(&desc);
		}
	}

	~NvFlexExtForceFieldCallback()
	{
		// force fields
		delete mForceFieldsGpu;
		delete mConstantBuffer;
		delete mShaderUpdateForceFields;
	}
	
	NvFlex::Buffer* mForceFieldsGpu;

	// DX Specific
	NvFlex::ComputeShader* mShaderUpdateForceFields;
	NvFlex::ConstantBuffer* mConstantBuffer;

	int mMaxForceFields;
	int mNumForceFields;

	// D3D device and context wrappers for the solver library
	NvFlex::Device* mDevice;
	NvFlex::Context* mContext;

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

void ApplyForceFieldsCallback(NvFlexSolverCallbackParams params)
{
	// callbacks always have the correct CUDA device set so we can safely launch kernels without acquiring

	NvFlexExtForceFieldCallback* c = (NvFlexExtForceFieldCallback*)params.userData;

	if (params.numActive && c->mNumForceFields)
	{
		const unsigned int numThreadsPerBlock = 256;
		const unsigned int kNumBlocks = (params.numActive + numThreadsPerBlock - 1) / numThreadsPerBlock;

		NvFlex::Buffer* particles = (NvFlex::Buffer*)params.particles;
		NvFlex::Buffer* velocities = (NvFlex::Buffer*)params.velocities;

		// Init constant buffer
		{
			FlexExtConstParams constBuffer;

			constBuffer.kNumParticles = params.numActive;
			constBuffer.kNumForceFields = c->mNumForceFields;
			constBuffer.kDt = params.dt;

			memcpy(c->mContext->map(c->mConstantBuffer), &constBuffer, sizeof(FlexExtConstParams));
			c->mContext->unmap(c->mConstantBuffer);
		}

		{
			NvFlex::DispatchParams params = {};
			params.shader = c->mShaderUpdateForceFields;
			params.readWrite[0] = velocities->getResourceRW();
			params.readOnly[0] = particles->getResource();
			params.readOnly[1] = c->mForceFieldsGpu->getResource();
			params.gridDim = { kNumBlocks , 1, 1 };
			params.rootConstantBuffer = c->mConstantBuffer;

			c->mContext->dispatch(&params);
		}
	}
}

void NvFlexExtSetForceFields(NvFlexExtForceFieldCallback* c, const NvFlexExtForceField* forceFields, int numForceFields)
{
	// re-alloc if necessary
	if (numForceFields > c->mMaxForceFields)
	{
		delete c->mForceFieldsGpu;

		NvFlex::BufferDesc desc {};
		desc.dim = numForceFields;
		desc.stride = sizeof(NvFlexExtForceField);
		desc.bufferType = NvFlex::eBuffer | NvFlex::eUAV_SRV | NvFlex::eStructured | NvFlex::eStage; 
		desc.format = NvFlexFormat::eNvFlexFormat_unknown;
		desc.data = NULL;
	
		c->mForceFieldsGpu = c->mContext->createBuffer(&desc);

		c->mMaxForceFields = numForceFields;
	}
	c->mNumForceFields = numForceFields;

	if (numForceFields > 0)
	{
		// update staging buffer
		void* dstPtr = c->mContext->map(c->mForceFieldsGpu, NvFlex::eMapWrite);
		memcpy(dstPtr, forceFields, numForceFields*sizeof(NvFlexExtForceField));
		c->mContext->unmap(c->mForceFieldsGpu);

		// upload to device buffer
		c->mContext->upload(c->mForceFieldsGpu, 0, numForceFields*sizeof(NvFlexExtForceField));

	}

	NvFlexSolverCallback callback;
	callback.function = ApplyForceFieldsCallback;
	callback.userData = c;

	// register a callback to calculate the forces at the end of the time-step
	NvFlexRegisterSolverCallback(c->mSolver, callback, eNvFlexStageUpdateEnd);
}
