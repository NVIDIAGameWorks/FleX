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

#include "maths.h"

#include <vector>

void Extrude(const Vec3* points, int numPoints, std::vector<Vec3>& vertices, std::vector<Vec3>& normals, std::vector<int>& triangles, float radius, int resolution, int smoothing)
{
	if (numPoints < 2)
		return;

	Vec3 u, v;
	Vec3 w = SafeNormalize(Vec3(points[1]) - Vec3(points[0]), Vec3(0.0f, 1.0f, 0.0f));

	BasisFromVector(w, &u, &v);

	Matrix44 frame;
	frame.SetCol(0, Vec4(u.x, u.y, u.z, 0.0f));
	frame.SetCol(1, Vec4(v.x, v.y, v.z, 0.0f));
	frame.SetCol(2, Vec4(w.x, w.y, w.z, 0.0f));
	frame.SetCol(3, Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (int i = 0; i < numPoints - 1; ++i)
	{
		Vec3 next;

		if (i < numPoints - 1)
			next = Normalize(Vec3(points[i + 1]) - Vec3(points[i - 1]));
		else
			next = Normalize(Vec3(points[i]) - Vec3(points[i - 1]));

		int a = Max(i - 1, 0);
		int b = i;
		int c = Min(i + 1, numPoints - 1);
		int d = Min(i + 2, numPoints - 1);

		Vec3 p1 = Vec3(points[b]);
		Vec3 p2 = Vec3(points[c]);
		Vec3 m1 = 0.5f*(Vec3(points[c]) - Vec3(points[a]));
		Vec3 m2 = 0.5f*(Vec3(points[d]) - Vec3(points[b]));

		// ensure last segment handled correctly
		int segments = (i < numPoints - 2) ? smoothing : smoothing + 1;

		for (int s = 0; s < segments; ++s)
		{
			Vec3 pos = HermiteInterpolate(p1, p2, m1, m2, s / float(smoothing));
			Vec3 dir = Normalize(HermiteTangent(p1, p2, m1, m2, s / float(smoothing)));

			Vec3 cur = frame.GetAxis(2);
			const float angle = acosf(Dot(cur, dir));

			// if parallel then don't need to do anything
			if (fabsf(angle) > 0.001f)
				frame = RotationMatrix(angle, SafeNormalize(Cross(cur, dir)))*frame;

			size_t startIndex = vertices.size();

			for (int c = 0; c < resolution; ++c)
			{
				float angle = k2Pi / resolution;

				// transform position and normal to world space
				Vec4 v = frame*Vec4(cosf(angle*c), sinf(angle*c), 0.0f, 0.0f);

				vertices.push_back(Vec3(v)*radius + pos);
				normals.push_back(Vec3(v));
			}

			// output triangles
			if (startIndex != 0)
			{
				for (int i = 0; i < resolution; ++i)
				{
					int curIndex = static_cast<int>(startIndex + i);
					int nextIndex = static_cast<int>(startIndex + (i + 1) % resolution);

					triangles.push_back(curIndex);
					triangles.push_back(curIndex - resolution);
					triangles.push_back(nextIndex - resolution);

					triangles.push_back(nextIndex - resolution);
					triangles.push_back(nextIndex);
					triangles.push_back(curIndex);
				}
			}
		}
	}
}
