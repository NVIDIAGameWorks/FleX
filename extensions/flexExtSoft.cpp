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

#include "../include/NvFlexExt.h"

#include "../core/core.h"
#include "../core/maths.h"
#include "../core/voxelize.h"

#include <vector>
#include <algorithm>

using namespace std;

// Soft body support functions

namespace
{

Vec3 CalculateMean(const Vec3* particles, const int* indices, int numIndices)
{
	Vec3 sum;

	for (int i = 0; i < numIndices; ++i)
		sum += Vec3(particles[indices[i]]);

	if (numIndices)
		return sum / float(numIndices);
	else
		return sum;
}

float CalculateRadius(const Vec3* particles, Vec3 center, const int* indices, int numIndices)
{
	float radiusSq = 0.0f;

	for (int i = 0; i < numIndices; ++i)
	{
		float dSq = LengthSq(Vec3(particles[indices[i]]) - center);
		if (dSq > radiusSq)
			radiusSq = dSq;
	}

	return sqrtf(radiusSq);
}

struct Cluster
{
	Vec3 mean;
	float radius;

	// indices of particles belonging to this cluster
	std::vector<int> indices;
};

struct Seed
{
	int index;
	float priority;

	bool operator < (const Seed& rhs) const
	{
		return priority < rhs.priority;
	}
};

// basic SAP based acceleration structure for point cloud queries
struct SweepAndPrune
{
	struct Entry
	{
		Entry(Vec3 p, int i) : point(p), index(i) {}

		Vec3 point;
		int index;
	};

	SweepAndPrune(const Vec3* points, int n)
	{
		entries.reserve(n);		
		for (int i=0; i < n; ++i)
			entries.push_back(Entry(points[i], i));

		struct SortOnAxis
		{
			int axis;

			SortOnAxis(int axis) : axis(axis) {}

			bool operator()(const Entry& lhs, const Entry& rhs) const			
			{
				return lhs.point[axis] < rhs.point[axis];
			}
		};

		// calculate particle bounds and longest axis
		Vec3 lower(FLT_MAX), upper(-FLT_MAX);
		for (int i=0; i < n; ++i)
		{
			lower = Min(points[i], lower);
			upper = Max(points[i], upper);
		}

		Vec3 edges = upper-lower;
		
		if (edges.x > edges.y && edges.x > edges.z)
			longestAxis = 0;
		else if (edges.y > edges.z)
			longestAxis = 1;
		else
			longestAxis = 2;
		
		std::sort(entries.begin(), entries.end(), SortOnAxis(longestAxis));
	}

	void QuerySphere(Vec3 center, float radius, std::vector<int>& indices)
	{
		// find start point in the array
		int low = 0;
		int high = int(entries.size());
		
		// the point we are trying to find
		float queryLower = center[longestAxis] - radius;
		float queryUpper = center[longestAxis] + radius;

		// binary search to find the start point in the sorted entries array
		while (low < high)
		{
			const int mid = (high+low)/2;

			if (queryLower > entries[mid].point[longestAxis])
				low = mid+1;
			else
				high = mid;
		}

		// scan forward over potential overlaps
		float radiusSq = radius*radius;

		for (int i=low; i < int(entries.size()); ++i)
		{
			Vec3 p = entries[i].point;

			if (LengthSq(p-center) < radiusSq)
			{
				indices.push_back(entries[i].index);
			}					
			else if (entries[i].point[longestAxis] > queryUpper)
			{
				// early out if ther are no more possible candidates
				break;
			}
		}
	}
	
	int longestAxis;	// [0,2] -> x,y,z

	std::vector<Entry> entries;
};

int CreateClusters(Vec3* particles, const float* priority, int numParticles, std::vector<int>& outClusterOffsets, std::vector<int>& outClusterIndices, std::vector<Vec3>& outClusterPositions, float radius, float smoothing = 0.0f)
{
	std::vector<Seed> seeds;
	std::vector<Cluster> clusters;

	// flags a particle as belonging to at least one cluster
	std::vector<bool> used(numParticles, false);

	// initialize seeds
	for (int i = 0; i < numParticles; ++i)
	{
		Seed s;
		s.index = i;
		s.priority = priority[i];

		seeds.push_back(s);
	}

	// sort seeds on priority
	std::stable_sort(seeds.begin(), seeds.end());

	SweepAndPrune sap(particles, numParticles);

	while (seeds.size())
	{
		// pick highest unused particle from the seeds list
		Seed seed = seeds.back();
		seeds.pop_back();

		if (!used[seed.index])
		{
			Cluster c;

			sap.QuerySphere(Vec3(particles[seed.index]), radius, c.indices);
			
			// mark overlapping particles as used so they are removed from the list of potential cluster seeds
			for (int i=0; i < int(c.indices.size()); ++i)
				used[c.indices[i]] = true;

			c.mean = CalculateMean(particles, &c.indices[0], int(c.indices.size()));

			clusters.push_back(c);
		}
	}

	if (smoothing > 0.0f)
	{
		for (int i = 0; i < int(clusters.size()); ++i)
		{
			Cluster& c = clusters[i];

			// clear cluster indices
			c.indices.resize(0);

			// calculate cluster particles using cluster mean and smoothing radius
			sap.QuerySphere(c.mean, smoothing, c.indices);

			c.mean = CalculateMean(particles, &c.indices[0], int(c.indices.size()));
		}
	}

	// write out cluster indices
	int count = 0;

	for (int c = 0; c < int(clusters.size()); ++c)
	{
		const Cluster& cluster = clusters[c];

		const int clusterSize = int(cluster.indices.size());

		// skip empty clusters
		if (clusterSize)
		{
			// write cluster indices
			for (int i = 0; i < clusterSize; ++i)
				outClusterIndices.push_back(cluster.indices[i]);

			// write cluster offset
			outClusterOffsets.push_back(int(outClusterIndices.size()));

			// write center
			outClusterPositions.push_back(cluster.mean);

			++count;
		}
	}

	return count;
}

// creates distance constraints between particles within some radius
int CreateLinks(const Vec3* particles, int numParticles, std::vector<int>& outSpringIndices, std::vector<float>& outSpringLengths, std::vector<float>& outSpringStiffness, float radius, float stiffness = 1.0f)
{
	int count = 0;

	std::vector<int> neighbors;
	SweepAndPrune sap(particles, numParticles);

	for (int i = 0; i < numParticles; ++i)
	{
		neighbors.resize(0);

		sap.QuerySphere(Vec3(particles[i]), radius, neighbors);

		for (int j = 0; j < int(neighbors.size()); ++j)
		{
			const int nj = neighbors[j];

			if (nj != i)
			{
				outSpringIndices.push_back(i);
				outSpringIndices.push_back(nj);
				outSpringLengths.push_back(Length(Vec3(particles[i]) - Vec3(particles[nj])));
				outSpringStiffness.push_back(stiffness);

				++count;
			}
		}
	}

	return count;
}

void CreateSkinning(const Vec3* vertices, int numVertices, const Vec3* clusters, int numClusters, float* outWeights, int* outIndices, float falloff, float maxdist)
{
	const int maxBones = 4;

	SweepAndPrune sap(clusters, numClusters);

	std::vector<int> influences;

	// for each vertex, find the closest n clusters
	for (int i = 0; i < numVertices; ++i)
	{
		int indices[4] = { -1, -1, -1, -1 };
		float distances[4] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
		float weights[maxBones];

		influences.resize(0);
		sap.QuerySphere(vertices[i], maxdist, influences);

		for (int c = 0; c < int(influences.size()); ++c)			
		{
			float dSq = LengthSq(vertices[i] - clusters[influences[c]]);

			// insertion sort
			int w = 0;
			for (; w < maxBones; ++w)
				if (dSq < distances[w])
					break;

			if (w < maxBones)
			{
				// shuffle down
				for (int s = maxBones - 1; s > w; --s)
				{
					indices[s] = indices[s - 1];
					distances[s] = distances[s - 1];
				}

				distances[w] = dSq;
				indices[w] = influences[c];
			}
		}

		// weight particles according to distance
		float wSum = 0.0f;

		for (int w = 0; w < maxBones; ++w)
		{
			if (distances[w] > Sqr(maxdist))
			{
				// clamp bones over a given distance to zero
				weights[w] = 0.0f;
			}
			else
			{
				// weight falls off inversely with distance
				weights[w] = 1.0f / (powf(distances[w], falloff) + 0.0001f);
			}

			wSum += weights[w];
		}

		if (wSum == 0.0f)
		{
			// if all weights are zero then just 
			// rigidly skin to the closest bone
			weights[0] = 1.0f;
		}
		else
		{
			// normalize weights
			for (int w = 0; w < maxBones; ++w)
			{
				weights[w] = weights[w] / wSum;
			}
		}

		// output
		for (int j = 0; j < maxBones; ++j)
		{
			outWeights[i*maxBones + j] = weights[j];
			outIndices[i*maxBones + j] = indices[j];
		}
	}
}

// creates mesh interior and surface sample points and clusters them into particles
void SampleMesh(const Vec3* vertices, int numVertices, const int* indices, int numIndices, float radius, float volumeSampling, float surfaceSampling, std::vector<Vec3>& outPositions)
{
	Vec3 meshLower(FLT_MAX);
	Vec3 meshUpper(-FLT_MAX);

	for (int i = 0; i < numVertices; ++i)
	{
		meshLower = Min(meshLower, vertices[i]);
		meshUpper = Max(meshUpper, vertices[i]);
	}

	std::vector<Vec3> samples;

	if (volumeSampling > 0.0f)
	{
		// recompute expanded edges
		Vec3 edges = meshUpper - meshLower;

		// use a higher resolution voxelization as a basis for the particle decomposition
		float spacing = radius / volumeSampling;

		// tweak spacing to avoid edge cases for particles laying on the boundary
		// just covers the case where an edge is a whole multiple of the spacing.
		float spacingEps = spacing*(1.0f - 1e-4f);

		// make sure to have at least one particle in each dimension
		int dx, dy, dz;
		dx = spacing > edges.x ? 1 : int(edges.x / spacingEps);
		dy = spacing > edges.y ? 1 : int(edges.y / spacingEps);
		dz = spacing > edges.z ? 1 : int(edges.z / spacingEps);

		int maxDim = max(max(dx, dy), dz);

		// expand border by two voxels to ensure adequate sampling at edges
		meshLower -= 2.0f*Vec3(spacing);
		meshUpper += 2.0f*Vec3(spacing);
		maxDim += 4;

		vector<uint32_t> voxels(maxDim*maxDim*maxDim);

		// we shift the voxelization bounds so that the voxel centers
		// lie symmetrically to the center of the object. this reduces the 
		// chance of missing features, and also better aligns the particles
		// with the mesh
		Vec3 meshOffset;
		meshOffset.x = 0.5f * (spacing - (edges.x - (dx - 1)*spacing));
		meshOffset.y = 0.5f * (spacing - (edges.y - (dy - 1)*spacing));
		meshOffset.z = 0.5f * (spacing - (edges.z - (dz - 1)*spacing));
		meshLower -= meshOffset;

		Voxelize(vertices, numVertices, indices, numIndices, maxDim, maxDim, maxDim, &voxels[0], meshLower, meshLower + Vec3(maxDim*spacing));

		// sample interior
		for (int x = 0; x < maxDim; ++x)
		{
			for (int y = 0; y < maxDim; ++y)
			{
				for (int z = 0; z < maxDim; ++z)
				{
					const int index = z*maxDim*maxDim + y*maxDim + x;

					// if voxel is marked as occupied the add a particle
					if (voxels[index])
					{
						Vec3 position = meshLower + spacing*Vec3(float(x) + 0.5f, float(y) + 0.5f, float(z) + 0.5f);

						// normalize the sdf value and transform to world scale
						samples.push_back(position);
					}
				}
			}
		}
	}

	if (surfaceSampling > 0.0f)
	{
		// sample vertices
		for (int i = 0; i < numVertices; ++i)
			samples.push_back(vertices[i]);

		// random surface sampling (non-uniform)
		const int numSamples = int(50000 * surfaceSampling);
		const int numTriangles = numIndices/3;

		RandInit();

		for (int i = 0; i < numSamples; ++i)
		{
			int t = Rand() % numTriangles;
			float u = Randf();
			float v = Randf()*(1.0f - u);
			float w = 1.0f - u - v;

			int a = indices[t*3 + 0];
			int b = indices[t*3 + 1];
			int c = indices[t*3 + 2];

			Vec3 p = vertices[a] * u + vertices[b] * v + vertices[c] * w;

			samples.push_back(p);
		}
	}

	std::vector<int> clusterIndices;
	std::vector<int> clusterOffsets;
	std::vector<Vec3> clusterPositions;
	std::vector<float> priority(samples.size());

	// cluster mesh sample points into actual particles
	CreateClusters(&samples[0], &priority[0], int(samples.size()), clusterOffsets, clusterIndices, outPositions, radius);
}

} // anonymous namespace

// API methods

NvFlexExtAsset* NvFlexExtCreateSoftFromMesh(const float* vertices, int numVertices, const int* indices, int numIndices, float particleSpacing, float volumeSampling, float surfaceSampling, float clusterSpacing, float clusterRadius, float clusterStiffness, float linkRadius, float linkStiffness, float globalStiffness, float clusterPlasticThreshold, float clusterPlasticCreep)
{
	// Switch to relative coordinates by computing the mean position of the vertices and subtracting the result from every vertex position
	// The increased precision will prevent ghost forces caused by inaccurate center of mass computations
	Vec3 meshOffset(0.0f);
	for (int i = 0; i < numVertices; i++)
	{
		meshOffset += ((Vec3*)vertices)[i];
	}
	meshOffset /= float(numVertices);

	Vec3* relativeVertices = new Vec3[numVertices];
	for (int i = 0; i < numVertices; i++)
	{
		relativeVertices[i] += ((Vec3*)vertices)[i] - meshOffset;
	}

	// construct asset definition
	NvFlexExtAsset* asset = new NvFlexExtAsset();

	// create particle sampling
	std::vector<Vec3> samples;
	SampleMesh(relativeVertices, numVertices, indices, numIndices, particleSpacing, volumeSampling, surfaceSampling, samples);

	delete[] relativeVertices;

	const int numParticles = int(samples.size());	

	std::vector<int> clusterIndices;
	std::vector<int> clusterOffsets;
	std::vector<Vec3> clusterPositions;
	std::vector<float> clusterCoefficients;
	std::vector<float> clusterPlasticThresholds;
	std::vector<float> clusterPlasticCreeps;

	// priority (not currently used)
	std::vector<float> priority(numParticles);
	for (int i = 0; i < int(priority.size()); ++i)
		priority[i] = 0.0f;

	// cluster particles into shape matching groups
	int numClusters = CreateClusters(&samples[0], &priority[0], int(samples.size()), clusterOffsets, clusterIndices, clusterPositions, clusterSpacing, clusterRadius);
	
	// assign all clusters the same stiffness 
	clusterCoefficients.resize(numClusters, clusterStiffness);

	if (clusterPlasticCreep) 
	{
		// assign all clusters the same plastic threshold 
		clusterPlasticThresholds.resize(numClusters, clusterPlasticThreshold);

		// assign all clusters the same plastic creep 
		clusterPlasticCreeps.resize(numClusters, clusterPlasticCreep);
	}

	// create links between clusters 
	if (linkRadius > 0.0f)
	{		
		std::vector<int> springIndices;
		std::vector<float> springLengths;
		std::vector<float> springStiffness;

		// create links between particles
		int numLinks = CreateLinks(&samples[0], int(samples.size()), springIndices, springLengths, springStiffness, linkRadius, linkStiffness);

		// assign links
		if (numLinks)
		{			
			asset->springIndices = new int[numLinks * 2];
			memcpy(asset->springIndices, &springIndices[0], sizeof(int)*springIndices.size());

			asset->springCoefficients = new float[numLinks];
			memcpy(asset->springCoefficients, &springStiffness[0], sizeof(float)*numLinks);

			asset->springRestLengths = new float[numLinks];
			memcpy(asset->springRestLengths, &springLengths[0], sizeof(float)*numLinks);

			asset->numSprings = numLinks;
		}
	}

	// add an additional global cluster with stiffness = globalStiffness
	if (globalStiffness > 0.0f)
	{
		numClusters += 1;
		clusterCoefficients.push_back(globalStiffness);
		if (clusterPlasticCreep) 
		{
			clusterPlasticThresholds.push_back(clusterPlasticThreshold);
			clusterPlasticCreeps.push_back(clusterPlasticCreep);
		}

		for (int i = 0; i < numParticles; ++i)
		{
			clusterIndices.push_back(i);
		}
		clusterOffsets.push_back((int)clusterIndices.size());

		// the mean of the global cluster is the mean of all particles
		Vec3 globalMeanPosition(0.0f);
		for (int i = 0; i < numParticles; ++i)
		{
			globalMeanPosition += samples[i];
		}
		globalMeanPosition /= float(numParticles);
		clusterPositions.push_back(globalMeanPosition);
	}

	// Switch back to absolute coordinates by adding meshOffset to the centers of mass and to each particle positions
	for (int i = 0; i < numParticles; ++i)
	{
		 samples[i] += meshOffset;
	}
	for (int i = 0; i < numClusters; ++i)
	{
		clusterPositions[i] += meshOffset;
	}

	// assign particles
	asset->particles = new float[numParticles * 4];
	asset->numParticles = numParticles;
	asset->maxParticles = numParticles;

	for (int i = 0; i < numParticles; ++i)
	{
		asset->particles[i*4+0] = samples[i].x;
		asset->particles[i*4+1] = samples[i].y;
		asset->particles[i*4+2] = samples[i].z;
		asset->particles[i*4+3] = 1.0f;
	}

	// assign shapes
	asset->shapeIndices = new int[clusterIndices.size()];
	memcpy(asset->shapeIndices, &clusterIndices[0], sizeof(int)*clusterIndices.size());

	asset->shapeOffsets = new int[numClusters];
	memcpy(asset->shapeOffsets, &clusterOffsets[0], sizeof(int)*numClusters);

	asset->shapeCenters = new float[numClusters*3];
	memcpy(asset->shapeCenters, &clusterPositions[0], sizeof(float)*numClusters*3);

	asset->shapeCoefficients = new float[numClusters];
	memcpy(asset->shapeCoefficients, &clusterCoefficients[0], sizeof(float)*numClusters);

	if (clusterPlasticCreep) 
	{
		asset->shapePlasticThresholds = new float[numClusters];
		memcpy(asset->shapePlasticThresholds, &clusterPlasticThresholds[0], sizeof(float)*numClusters);

		asset->shapePlasticCreeps = new float[numClusters];
		memcpy(asset->shapePlasticCreeps, &clusterPlasticCreeps[0], sizeof(float)*numClusters);
	}
	else
	{
		asset->shapePlasticThresholds = NULL;
		asset->shapePlasticCreeps = NULL;
	}

	asset->numShapeIndices = int(clusterIndices.size());
	asset->numShapes = numClusters;

	return asset;
}

void NvFlexExtCreateSoftMeshSkinning(const float* vertices, int numVertices, const float* bones, int numBones, float falloff, float maxDistance, float* skinningWeights, int* skinningIndices)
{
	CreateSkinning((Vec3*)vertices, numVertices, (Vec3*)bones, numBones, skinningWeights, skinningIndices, falloff, maxDistance);
}
