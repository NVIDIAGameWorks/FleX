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

#pragma once

#include <vector>

#include "core.h"
#include "maths.h"

struct Mesh
{
    void AddMesh(const Mesh& m);

    uint32_t GetNumVertices() const { return uint32_t(m_positions.size()); }
    uint32_t GetNumFaces() const { return uint32_t(m_indices.size()) / 3; }

	void DuplicateVertex(uint32_t i);

    void CalculateNormals();
    void Transform(const Matrix44& m);
	void Normalize(float s=1.0f);	// scale so bounds in any dimension equals s and lower bound = (0,0,0)

    void GetBounds(Vector3& minExtents, Vector3& maxExtents) const;

    std::vector<Point3> m_positions;
    std::vector<Vector3> m_normals;
    std::vector<Vector2> m_texcoords[2];
    std::vector<Colour> m_colours;

    std::vector<uint32_t> m_indices;    
};

// create mesh from file
Mesh* ImportMeshFromObj(const char* path);
Mesh* ImportMeshFromPly(const char* path);
Mesh* ImportMeshFromBin(const char* path);

// just switches on filename
Mesh* ImportMesh(const char* path);

// save a mesh in a flat binary format
void ExportMeshToBin(const char* path, const Mesh* m);

// create procedural primitives
Mesh* CreateTriMesh(float size, float y=0.0f);
Mesh* CreateCubeMesh();
Mesh* CreateQuadMesh(float size, float y=0.0f);
Mesh* CreateDiscMesh(float radius, uint32_t segments);
Mesh* CreateTetrahedron(float ground=0.0f, float height=1.0f); //fixed but not used
Mesh* CreateSphere(int slices, int segments, float radius = 1.0f);
Mesh* CreateCapsule(int slices, int segments, float radius = 1.0f, float halfHeight = 1.0f);

