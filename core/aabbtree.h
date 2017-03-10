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

#include "core.h"
#include "maths.h"

#include <vector>

class AABBTree
{
	AABBTree(const AABBTree&);
	AABBTree& operator=(const AABBTree&);

public:

    AABBTree(const Vec3* vertices, uint32_t numVerts, const uint32_t* indices, uint32_t numFaces);

	bool TraceRaySlow(const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const;
    bool TraceRay(const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const;

    void DebugDraw();
    
    Vector3 GetCenter() const { return (m_nodes[0].m_minExtents+m_nodes[0].m_maxExtents)*0.5f; }
    Vector3 GetMinExtents() const { return m_nodes[0].m_minExtents; }
    Vector3 GetMaxExtents() const { return m_nodes[0].m_maxExtents; }
	
#if _WIN32
    // stats (reset each trace)
    static uint32_t GetTraceDepth() { return s_traceDepth; }
#endif
	
private:

    void DebugDrawRecursive(uint32_t nodeIndex, uint32_t depth);

    struct Node
    {
        Node() 	
            : m_numFaces(0)
            , m_faces(NULL)
            , m_minExtents(0.0f)
            , m_maxExtents(0.0f)
        {
        }

		union
		{
			uint32_t m_children;
			uint32_t m_numFaces;			
		};

		uint32_t* m_faces;        
        Vector3 m_minExtents;
        Vector3 m_maxExtents;
    };


    struct Bounds
    {
        Bounds() : m_min(0.0f), m_max(0.0f)
        {
        }

        Bounds(const Vector3& min, const Vector3& max) : m_min(min), m_max(max)
        {
        }

        inline float GetVolume() const
        {
            Vector3 e = m_max-m_min;
            return (e.x*e.y*e.z);
        }

        inline float GetSurfaceArea() const
        {
            Vector3 e = m_max-m_min;
            return 2.0f*(e.x*e.y + e.x*e.z + e.y*e.z);
        }

        inline void Union(const Bounds& b)
        {
            m_min = Min(m_min, b.m_min);
            m_max = Max(m_max, b.m_max);
        }

        Vector3 m_min;
        Vector3 m_max;
    };

    typedef std::vector<uint32_t> IndexArray;
    typedef std::vector<Vec3> PositionArray;
    typedef std::vector<Node> NodeArray;
    typedef std::vector<uint32_t> FaceArray;
    typedef std::vector<Bounds> FaceBoundsArray;

	// partition the objects and return the number of objects in the lower partition
	uint32_t PartitionMedian(Node& n, uint32_t* faces, uint32_t numFaces);
	uint32_t PartitionSAH(Node& n, uint32_t* faces, uint32_t numFaces);

    void Build();
    void BuildRecursive(uint32_t nodeIndex, uint32_t* faces, uint32_t numFaces);
    void TraceRecursive(uint32_t nodeIndex, const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const;
 
    void CalculateFaceBounds(uint32_t* faces, uint32_t numFaces, Vector3& outMinExtents, Vector3& outMaxExtents);
    uint32_t GetNumFaces() const { return m_numFaces; }
	uint32_t GetNumNodes() const { return uint32_t(m_nodes.size()); }

	// track the next free node
	uint32_t m_freeNode;

    const Vec3* m_vertices;
    const uint32_t m_numVerts;

    const uint32_t* m_indices;
    const uint32_t m_numFaces;

    FaceArray m_faces;
    NodeArray m_nodes;
    FaceBoundsArray m_faceBounds;    

    // stats
    uint32_t m_treeDepth;
    uint32_t m_innerNodes;
    uint32_t m_leafNodes; 
	
#if _WIN32
   _declspec (thread) static uint32_t s_traceDepth;
#endif
};
