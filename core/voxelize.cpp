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

#include "aabbtree.h"
#include "mesh.h"

void Voxelize(const Vec3* vertices, int numVertices, const int* indices, int numTriangleIndices, uint32_t width, uint32_t height, uint32_t depth, uint32_t* volume, Vec3 minExtents, Vec3 maxExtents)
{
	memset(volume, 0, sizeof(uint32_t)*width*height*depth);

	// build an aabb tree of the mesh
	AABBTree tree(vertices, numVertices, (const uint32_t*)indices, numTriangleIndices/3); 

	// parity count method, single pass
	const Vec3 extents(maxExtents-minExtents);
	const Vec3 delta(extents.x/width, extents.y/height, extents.z/depth);
	const Vec3 offset(0.5f*delta.x, 0.5f*delta.y, 0.5f*delta.z);
	
	// this is the bias we apply to step 'off' a triangle we hit, not very robust
	const float eps = 0.00001f*extents.z;

	for (uint32_t x=0; x < width; ++x)
	{
		for (uint32_t y=0; y < height; ++y)
		{
			bool inside = false;

			Vec3 rayDir = Vec3(0.0f, 0.0f, 1.0f);
			Vec3 rayStart = minExtents + Vec3(x*delta.x + offset.x, y*delta.y + offset.y, 0.0f);

			uint32_t lastTri = uint32_t(-1);
			for (;;)
			{
				// calculate ray start
				float t, u, v, w, s;
				uint32_t tri;

				if (tree.TraceRay(rayStart, rayDir, t, u, v, w, s, tri))
				{
					// calculate cell in which intersection occurred
					const float zpos = rayStart.z + t*rayDir.z;
					const float zhit = (zpos-minExtents.z)/delta.z;
					
					uint32_t z = uint32_t(floorf((rayStart.z-minExtents.z)/delta.z + 0.5f));
					uint32_t zend = std::min(uint32_t(floorf(zhit + 0.5f)), depth-1);

					if (inside)
					{
						// march along column setting bits 
						for (uint32_t k=z; k < zend; ++k)
							volume[k*width*height + y*width + x] = uint32_t(-1);
					}
					
					inside = !inside;
					
					// we hit the tri we started from
					if (tri == lastTri)
						printf("Error self-intersect\n");
					lastTri = tri;

					rayStart += rayDir*(t+eps);

				}
				else
					break;
			}
		}
	}	
}
