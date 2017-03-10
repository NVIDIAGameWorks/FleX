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

#include "flexExt_dx_common.h"

#define kNumThreadsPerBlock 256

cbuffer consts : register(b0) { FlexExtConstParams gParams; };

namespace UpdateForceFields
{
	StructuredBuffer<float4> positions : register(t0);
	StructuredBuffer<FlexExtForceFieldD3D> forceFields : register(t1);

	RWStructuredBuffer<float4> velocities : register(u0);

	[numthreads(kNumThreadsPerBlock, 1, 1)] void execute(uint3 globalIdx : SV_DispatchThreadID)
	{
		const int i = globalIdx.x;
		const int numParticles = gParams.kNumParticles;
		const int numForceFields = gParams.kNumForceFields;
		const float dt = gParams.kDt;
		
		for (int f = 0; f < numForceFields; f++)
		{
			const FlexExtForceFieldD3D forceField = forceFields[f];
			
			if (i < numParticles)
			{
				const int index = i;
				
				float4 p = positions[index];
				float3 v = velocities[index].xyz;
				
				float3 localPos = float3(p.x, p.y, p.z) - float3(forceField.mPosition[0], forceField.mPosition[1], forceField.mPosition[2]);

				float dist = length(localPos);
				if (dist >= forceField.mRadius)
				{
					continue;
				}

				float3 fieldDir;
				if (dist > 0.0f)
				{
					fieldDir = localPos / dist;
				}
				else
				{
					fieldDir = localPos;
				}

				// If using linear falloff, scale with distance.
				float fieldStrength = forceField.mStrength;
				if (forceField.mLinearFalloff)
				{
					fieldStrength *= (1.0f - (dist / forceField.mRadius));
				}

				// Apply force
				float3 force = localPos * fieldStrength;

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

				float3 deltaVelocity = fieldDir * fieldStrength * unitMultiplier;
				velocities[index] = float4(v + deltaVelocity, 0.0f);
			}
		}
	}
}
