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

#include "maths.h"
#include "platform.h"

#include <algorithm>
#include <iostream>

using namespace std;

#if _WIN32
_declspec (thread) uint32_t AABBTree::s_traceDepth;
#endif

AABBTree::AABBTree(const Vec3* vertices, uint32_t numVerts, const uint32_t* indices, uint32_t numFaces) 
    : m_vertices(vertices)
    , m_numVerts(numVerts)
    , m_indices(indices)
    , m_numFaces(numFaces)
{
    // build stats
    m_treeDepth = 0;
    m_innerNodes = 0;
    m_leafNodes = 0;

    Build();
}

namespace
{

	struct FaceSorter
	{
		FaceSorter(const Vec3* positions, const uint32_t* indices, uint32_t n, uint32_t axis) 
			: m_vertices(positions)
			, m_indices(indices)
			, m_numIndices(n)
			, m_axis(axis)
		{        
		}

		inline bool operator()(uint32_t lhs, uint32_t rhs) const
		{
			float a = GetCentroid(lhs);
			float b = GetCentroid(rhs);

			if (a == b)
				return lhs < rhs;
			else
				return a < b;
		}

		inline float GetCentroid(uint32_t face) const
		{
			const Vec3& a = m_vertices[m_indices[face*3+0]];
			const Vec3& b = m_vertices[m_indices[face*3+1]];
			const Vec3& c = m_vertices[m_indices[face*3+2]];

			return (a[m_axis] + b[m_axis] + c[m_axis])/3.0f;
		}

		const Vec3* m_vertices;
		const uint32_t* m_indices;
		uint32_t m_numIndices;
		uint32_t m_axis;
	};
	
	inline uint32_t LongestAxis(const Vector3& v)
	{    
		if (v.x > v.y && v.x > v.z)
			return 0;
		else
			return (v.y > v.z) ? 1 : 2;
	}

} // anonymous namespace

void AABBTree::CalculateFaceBounds(uint32_t* faces, uint32_t numFaces, Vector3& outMinExtents, Vector3& outMaxExtents)
{
    Vector3 minExtents(FLT_MAX);
    Vector3 maxExtents(-FLT_MAX);

    // calculate face bounds
    for (uint32_t i=0; i < numFaces; ++i)
    {
        Vector3 a = Vector3(m_vertices[m_indices[faces[i]*3+0]]);
        Vector3 b = Vector3(m_vertices[m_indices[faces[i]*3+1]]);
        Vector3 c = Vector3(m_vertices[m_indices[faces[i]*3+2]]);

        minExtents = Min(a, minExtents);
        maxExtents = Max(a, maxExtents);

        minExtents = Min(b, minExtents);
        maxExtents = Max(b, maxExtents);

        minExtents = Min(c, minExtents);
        maxExtents = Max(c, maxExtents);
    }

    outMinExtents = minExtents;
    outMaxExtents = maxExtents;
}

// track current tree depth
static uint32_t s_depth = 0;

void AABBTree::Build()
{
    assert(m_numFaces*3);

    //const double startTime = GetSeconds();

    const uint32_t numFaces = m_numFaces;

    // build initial list of faces
    m_faces.reserve(numFaces);
	/*	
    for (uint32_t i=0; i < numFaces; ++i)
    {
        m_faces[i] = i;
    }
	*/

    // calculate bounds of each face and store
    m_faceBounds.reserve(numFaces);   
    
	std::vector<Bounds> stack;
	for (uint32_t i=0; i < numFaces; ++i)
    {
		Bounds top;
        CalculateFaceBounds(&i, 1, top.m_min, top.m_max);
		
		m_faces.push_back(i);
		m_faceBounds.push_back(top);
		/*
		stack.push_back(top);

		while (!stack.empty())
		{
			Bounds b = stack.back();
			stack.pop_back();

			const float kAreaThreshold = 200.0f;

			if (b.GetSurfaceArea() < kAreaThreshold)
			{
				// node is good, append to our face list
				m_faces.push_back(i);
				m_faceBounds.push_back(b);
			}
			else
			{
				// split along longest axis
				uint32_t a = LongestAxis(b.m_max-b.m_min);

				float splitPos = (b.m_min[a] + b.m_max[a])*0.5f;
				Bounds left(b);
				left.m_max[a] = splitPos;
				
				assert(left.GetSurfaceArea() < b.GetSurfaceArea());


				Bounds right(b);
				right.m_min[a] = splitPos;

				assert(right.GetSurfaceArea() < b.GetSurfaceArea());

				stack.push_back(left);				
				stack.push_back(right);
			}
		}
		*/
    }

	m_nodes.reserve(uint32_t(numFaces*1.5f));

    // allocate space for all the nodes
	m_freeNode = 1;

    // start building
    BuildRecursive(0, &m_faces[0], numFaces);

    assert(s_depth == 0);


	/*
    const double buildTime = (GetSeconds()-startTime);
    cout << "AAABTree Build Stats:" << endl;
    cout << "Node size: " << sizeof(Node) << endl;
    cout << "Build time: " << buildTime << "s" << endl;
    cout << "Inner nodes: " << m_innerNodes << endl;
    cout << "Leaf nodes: " << m_leafNodes << endl;
    cout << "Alloc nodes: " << m_nodes.size() << endl;
    cout << "Avg. tris/leaf: " << m_faces.size() / float(m_leafNodes) << endl;
    cout << "Max depth: " << m_treeDepth << endl;
	*/
    // free some memory
    FaceBoundsArray f;
    m_faceBounds.swap(f);
}

// partion faces around the median face
uint32_t AABBTree::PartitionMedian(Node& n, uint32_t* faces, uint32_t numFaces)
{
	FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces*3, LongestAxis(n.m_maxExtents-n.m_minExtents));
    std::nth_element(faces, faces+numFaces/2, faces+numFaces, predicate);

	return numFaces/2;
}

// partion faces based on the surface area heuristic
uint32_t AABBTree::PartitionSAH(Node& n, uint32_t* faces, uint32_t numFaces)
{
	
	/*
    Vector3 mean(0.0f);
    Vector3 variance(0.0f);

    // calculate best axis based on variance
    for (uint32_t i=0; i < numFaces; ++i)
    {
        mean += 0.5f*(m_faceBounds[faces[i]].m_min + m_faceBounds[faces[i]].m_max);
    }
    
    mean /= float(numFaces);

    for (uint32_t i=0; i < numFaces; ++i)
    {
        Vector3 v = 0.5f*(m_faceBounds[faces[i]].m_min + m_faceBounds[faces[i]].m_max) - mean;
        v *= v;
        variance += v;
    }

    uint32_t bestAxis = LongestAxis(variance);
	*/

	uint32_t bestAxis = 0;
	uint32_t bestIndex = 0;
	float bestCost = FLT_MAX;

	for (uint32_t a=0; a < 3; ++a)	
	//uint32_t a = bestAxis;
	{
		// sort faces by centroids
		FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces*3, a);
		std::sort(faces, faces+numFaces, predicate);

		// two passes over data to calculate upper and lower bounds
		vector<float> cumulativeLower(numFaces);
		vector<float> cumulativeUpper(numFaces);

		Bounds lower;
		Bounds upper;

		for (uint32_t i=0; i < numFaces; ++i)
		{
			lower.Union(m_faceBounds[faces[i]]);
			upper.Union(m_faceBounds[faces[numFaces-i-1]]);

			cumulativeLower[i] = lower.GetSurfaceArea();        
			cumulativeUpper[numFaces-i-1] = upper.GetSurfaceArea();
		}

		float invTotalSA = 1.0f / cumulativeUpper[0];

		// test all split positions
		for (uint32_t i=0; i < numFaces-1; ++i)
		{
			float pBelow = cumulativeLower[i] * invTotalSA;
			float pAbove = cumulativeUpper[i] * invTotalSA;

			float cost = 0.125f + (pBelow*i + pAbove*(numFaces-i));
			if (cost <= bestCost)
			{
				bestCost = cost;
				bestIndex = i;
				bestAxis = a;
			}
		}
	}

	// re-sort by best axis
	FaceSorter predicate(&m_vertices[0], &m_indices[0], m_numFaces*3, bestAxis);
	std::sort(faces, faces+numFaces, predicate);

	return bestIndex+1;
}

void AABBTree::BuildRecursive(uint32_t nodeIndex, uint32_t* faces, uint32_t numFaces)
{
    const uint32_t kMaxFacesPerLeaf = 6;
    
    // if we've run out of nodes allocate some more
    if (nodeIndex >= m_nodes.size())
    {
		uint32_t s = std::max(uint32_t(1.5f*m_nodes.size()), 512U);

		//cout << "Resizing tree, current size: " << m_nodes.size()*sizeof(Node) << " new size: " << s*sizeof(Node) << endl;

        m_nodes.resize(s);
    }

    // a reference to the current node, need to be careful here as this reference may become invalid if array is resized
	Node& n = m_nodes[nodeIndex];

	// track max tree depth
    ++s_depth;
    m_treeDepth = max(m_treeDepth, s_depth);

	CalculateFaceBounds(faces, numFaces, n.m_minExtents, n.m_maxExtents);

	// calculate bounds of faces and add node  
    if (numFaces <= kMaxFacesPerLeaf)
    {
        n.m_faces = faces;
        n.m_numFaces = numFaces;		

        ++m_leafNodes;
    }
    else
    {
        ++m_innerNodes;        

        // face counts for each branch
        //const uint32_t leftCount = PartitionMedian(n, faces, numFaces);
        const uint32_t leftCount = PartitionSAH(n, faces, numFaces);
        const uint32_t rightCount = numFaces-leftCount;

		// alloc 2 nodes
		m_nodes[nodeIndex].m_children = m_freeNode;

		// allocate two nodes
		m_freeNode += 2;
  
        // split faces in half and build each side recursively
        BuildRecursive(m_nodes[nodeIndex].m_children+0, faces, leftCount);
        BuildRecursive(m_nodes[nodeIndex].m_children+1, faces+leftCount, rightCount);
    }

    --s_depth;
}

struct StackEntry
{
    uint32_t m_node;   
    float m_dist;
};


#define TRACE_STATS 0

/*
bool AABBTree::TraceRay(const Vec3& start, const Vector3& dir, float& outT, float& outU, float& outV, float& outW, float& outSign, uint32_t& outIndex) const
{
#if _WIN32
    // reset stats
    s_traceDepth = 0;
#endif
	
    const Vector3 rcp_dir(1.0f/dir.x, 1.0f/dir.y, 1.0f/dir.z);

	// some temp variables
	Vector3 normal;
    float t, u, v, w, s;

    float minT, minU, minV, minW, minSign;
	minU = minV = minW = minSign = minT = FLT_MAX;
    
	uint32_t minIndex = 0;
    Vector3 minNormal;

    const uint32_t kStackDepth = 50;
    
    StackEntry stack[kStackDepth];
    stack[0].m_node = 0;
    stack[0].m_dist = 0.0f;

    uint32_t stackCount = 1;

    while (stackCount)
    {
        // pop node from back
        StackEntry& e = stack[--stackCount];
        
        // ignore if another node has already come closer
        if (e.m_dist >= minT)
        {
            continue;
        }

        const Node* node = &m_nodes[e.m_node];

filth:

        if (node->m_faces == NULL)
        {

#if TRACE_STATS
            extern uint32_t g_nodesChecked;
            ++g_nodesChecked;
#endif

#if _WIN32
			++s_traceDepth;
#endif
            // find closest node
            const Node& leftChild = m_nodes[node->m_children+0];
            const Node& rightChild = m_nodes[node->m_children+1];

            float dist[2] = {FLT_MAX, FLT_MAX};

            IntersectRayAABBOmpf(start, rcp_dir, leftChild.m_minExtents, leftChild.m_maxExtents, dist[0]);
            IntersectRayAABBOmpf(start, rcp_dir, rightChild.m_minExtents, rightChild.m_maxExtents, dist[1]);

            const uint32_t closest = dist[1] < dist[0]; // 0 or 1
            const uint32_t furthest = closest ^ 1;

            if (dist[furthest] < minT)
            {
                StackEntry& e = stack[stackCount++];
                e.m_node = node->m_children+furthest;
                e.m_dist = dist[furthest];
            }

            // lifo
            if (dist[closest] < minT)
            {
                node = &m_nodes[node->m_children+closest];
                goto filth;
            }
        }
        else
        {
            for (uint32_t i=0; i < node->m_numFaces; ++i)
            {
                const uint32_t faceIndex = node->m_faces[i];
                const uint32_t indexStart = faceIndex*3;

                const Vec3& a = m_vertices[m_indices[indexStart+0]];
                const Vec3& b = m_vertices[m_indices[indexStart+1]];
                const Vec3& c = m_vertices[m_indices[indexStart+2]];
#if TRACE_STATS
                extern uint32_t g_trisChecked;
                ++g_trisChecked;
#endif

                if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s))
                {
                    if (t < minT && t > 0.01f)
                    {
                        minT = t;
                        minU = u;
                        minV = v;
                        minW = w;
						minSign = s;
                        minIndex = faceIndex;
                    }
                }
            }
        }
    }

    // copy to outputs
    outT = minT;
    outU = minU;
    outV = minV;
    outW = minW;
	outSign = minSign;
    outIndex = minIndex;

    return (outT != FLT_MAX);
}
*/

bool AABBTree::TraceRay(const Vec3& start, const Vector3& dir, float& outT, float& u, float& v, float& w, float& faceSign, uint32_t& faceIndex) const
{   
    //s_traceDepth = 0;

    Vector3 rcp_dir(1.0f/dir.x, 1.0f/dir.y, 1.0f/dir.z);

    outT = FLT_MAX;
    TraceRecursive(0, start, dir, outT, u, v, w, faceSign, faceIndex);

    return (outT != FLT_MAX);
}


void AABBTree::TraceRecursive(uint32_t nodeIndex, const Vec3& start, const Vector3& dir, float& outT, float& outU, float& outV, float& outW, float& faceSign, uint32_t& faceIndex) const
{
	const Node& node = m_nodes[nodeIndex];

    if (node.m_faces == NULL)
    {
#if _WIN32
        ++s_traceDepth;
#endif
		
#if TRACE_STATS
        extern uint32_t g_nodesChecked;
        ++g_nodesChecked;
#endif

        // find closest node
        const Node& leftChild = m_nodes[node.m_children+0];
        const Node& rightChild = m_nodes[node.m_children+1];

        float dist[2] = {FLT_MAX, FLT_MAX};

        IntersectRayAABB(start, dir, leftChild.m_minExtents, leftChild.m_maxExtents, dist[0], NULL);
        IntersectRayAABB(start, dir, rightChild.m_minExtents, rightChild.m_maxExtents, dist[1], NULL);
        
        uint32_t closest = 0;
        uint32_t furthest = 1;
		
        if (dist[1] < dist[0])
        {
            closest = 1;
            furthest = 0;
        }		

        if (dist[closest] < outT)
            TraceRecursive(node.m_children+closest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);

        if (dist[furthest] < outT)
            TraceRecursive(node.m_children+furthest, start, dir, outT, outU, outV, outW, faceSign, faceIndex);

    }
    else
    {
        Vector3 normal;
        float t, u, v, w, s;

        for (uint32_t i=0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i]*3;

            const Vec3& a = m_vertices[m_indices[indexStart+0]];
            const Vec3& b = m_vertices[m_indices[indexStart+1]];
            const Vec3& c = m_vertices[m_indices[indexStart+2]];
#if TRACE_STATS
            extern uint32_t g_trisChecked;
            ++g_trisChecked;
#endif

            if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s))
            {
                if (t < outT)
                {
                    outT = t;
					outU = u;
					outV = v;
					outW = w;
					faceSign = s;
					faceIndex = node.m_faces[i];
                }
            }
        }
    }
}

/*
bool AABBTree::TraceRay(const Vec3& start, const Vector3& dir, float& outT, Vector3* outNormal) const
{   
    outT = FLT_MAX;
    TraceRecursive(0, start, dir, outT, outNormal);

    return (outT != FLT_MAX);
}

void AABBTree::TraceRecursive(uint32_t n, const Vec3& start, const Vector3& dir, float& outT, Vector3* outNormal) const
{
    const Node& node = m_nodes[n];

    if (node.m_numFaces == 0)
    {
        extern _declspec(thread) uint32_t g_traceDepth;
        ++g_traceDepth;
#if _DEBUG
        extern uint32_t g_nodesChecked;
        ++g_nodesChecked;
#endif
        float t;
        if (IntersectRayAABB(start, dir, node.m_minExtents, node.m_maxExtents, t, NULL))
        {
            if (t <= outT)
            {
                TraceRecursive(n*2+1, start, dir, outT, outNormal);
                TraceRecursive(n*2+2, start, dir, outT, outNormal);              
            }
        }
    }
    else
    {
        Vector3 normal;
        float t, u, v, w;

        for (uint32_t i=0; i < node.m_numFaces; ++i)
        {
            uint32_t indexStart = node.m_faces[i]*3;

            const Vec3& a = m_vertices[m_indices[indexStart+0]];
            const Vec3& b = m_vertices[m_indices[indexStart+1]];
            const Vec3& c = m_vertices[m_indices[indexStart+2]];
#if _DEBUG
            extern uint32_t g_trisChecked;
            ++g_trisChecked;
#endif

            if (IntersectRayTri(start, dir, a, b, c, t, u, v, w, &normal))
            {
                if (t < outT)
                {
                    outT = t;

                    if (outNormal)
                        *outNormal = normal;
                }
            }
        }
    }
}
*/
bool AABBTree::TraceRaySlow(const Vec3& start, const Vector3& dir, float& outT, float& outU, float& outV, float& outW, float& faceSign, uint32_t& faceIndex) const
{    
    const uint32_t numFaces = GetNumFaces();

    float minT, minU, minV, minW, minS;
	minT = minU = minV = minW = minS = FLT_MAX;

    Vector3 minNormal(0.0f, 1.0f, 0.0f);

    Vector3 n(0.0f, 1.0f, 0.0f);
    float t, u, v, w, s;
    bool hit = false;
	uint32_t minIndex = 0;

    for (uint32_t i=0; i < numFaces; ++i)
    {
        const Vec3& a = m_vertices[m_indices[i*3+0]];
        const Vec3& b = m_vertices[m_indices[i*3+1]];
        const Vec3& c = m_vertices[m_indices[i*3+2]];

        if (IntersectRayTriTwoSided(start, dir, a, b, c, t, u, v, w, s))
        {
            if (t < minT)
            {
                minT = t;
				minU = u;
				minV = v;
				minW = w;
				minS = s;
                minNormal = n;
				minIndex = i;
                hit = true;
            }
        }
    }

    outT = minT;
	outU = minU;
	outV = minV;
	outW = minW;
	faceSign = minS;
	faceIndex = minIndex;

    return hit;
}

void AABBTree::DebugDraw()
{
	/*
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    DebugDrawRecursive(0, 0);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	*/

}

void AABBTree::DebugDrawRecursive(uint32_t nodeIndex, uint32_t depth)
{
    static uint32_t kMaxDepth = 3;

    if (depth > kMaxDepth)
        return;


    /*
    Node& n = m_nodes[nodeIndex];

	Vector3 minExtents = FLT_MAX;
    Vector3 maxExtents = -FLT_MAX;

    // calculate face bounds
    for (uint32_t i=0; i < m_vertices.size(); ++i)
    {
        Vector3 a = m_vertices[i];

        minExtents = Min(a, minExtents);
        maxExtents = Max(a, maxExtents);
    }

    
    glBegin(GL_QUADS);
    glVertex3f(minExtents.x, maxExtents.y, 0.0f);
    glVertex3f(maxExtents.x, maxExtents.y, 0.0f);
    glVertex3f(maxExtents.x, minExtents.y, 0.0f);
    glVertex3f(minExtents.x, minExtents.y, 0.0f);
    glEnd();
    

    n.m_center = Vec3(minExtents+maxExtents)/2;
    n.m_extents = (maxExtents-minExtents)/2;
    */
/*
    if (n.m_minEtextents != Vector3(0.0f))
    {
        Vec3 corners[8];
        corners[0] = n.m_center + Vector3(-n.m_extents.x, n.m_extents.y, n.m_extents.z);
        corners[1] = n.m_center + Vector3(n.m_extents.x, n.m_extents.y, n.m_extents.z);
        corners[2] = n.m_center + Vector3(n.m_extents.x, -n.m_extents.y, n.m_extents.z);
        corners[3] = n.m_center + Vector3(-n.m_extents.x, -n.m_extents.y, n.m_extents.z);

        corners[4] = n.m_center + Vector3(-n.m_extents.x, n.m_extents.y, -n.m_extents.z);
        corners[5] = n.m_center + Vector3(n.m_extents.x, n.m_extents.y, -n.m_extents.z);
        corners[6] = n.m_center + Vector3(n.m_extents.x, -n.m_extents.y, -n.m_extents.z);
        corners[7] = n.m_center + Vector3(-n.m_extents.x, -n.m_extents.y, -n.m_extents.z);
        
        glBegin(GL_QUADS);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3fv(corners[0]);
        glVertex3fv(corners[1]);
        glVertex3fv(corners[2]);
        glVertex3fv(corners[3]);

        glVertex3fv(corners[1]);
        glVertex3fv(corners[5]);
        glVertex3fv(corners[6]);
        glVertex3fv(corners[2]);

        glVertex3fv(corners[0]);
        glVertex3fv(corners[4]);
        glVertex3fv(corners[5]);
        glVertex3fv(corners[1]);

        glVertex3fv(corners[4]);
        glVertex3fv(corners[5]);
        glVertex3fv(corners[6]);
        glVertex3fv(corners[7]);

        glVertex3fv(corners[0]);
        glVertex3fv(corners[4]);
        glVertex3fv(corners[7]);
        glVertex3fv(corners[3]);

        glVertex3fv(corners[3]);
        glVertex3fv(corners[7]);
        glVertex3fv(corners[6]);
        glVertex3fv(corners[2]);

        glEnd();            

        DebugDrawRecursive(nodeIndex*2+1, depth+1);
        DebugDrawRecursive(nodeIndex*2+2, depth+1);
    }    
    */
}
