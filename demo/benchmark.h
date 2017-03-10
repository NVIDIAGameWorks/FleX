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

#pragma once

#include <iomanip>

const char* g_benchmarkFilename = "../../benchmark.txt";
std::ofstream g_benchmarkFile;

// returns the new scene if one is requested
int BenchmarkUpdate()
{
	// Enable console benchmark profiling
	static NvFlexTimers sTimersSum;
	static std::vector<NvFlexDetailTimer> sDTimersSum;
	static float sTotalFrameTime = 0.0f;
	static int sSamples = 0;

	static int benchmarkIter = 0;
	const int numBenchmarks = 5;
	const char* benchmarkList[numBenchmarks] = { "Env Cloth Small", "Viscosity Med", "Inflatables", "Game Mesh Particles", "Rigid4" };
	const char* benchmarkChartPrefix[numBenchmarks] = { "EnvClothSmall", "ViscosityMed", "Inflatables", "GameMeshParticles", "Rigid4" }; //no spaces
	//float benchmarkEnergyCheck[numBenchmarks] = { 6000, 1000, 1000, 150426, 63710 };

	int newScene = -1;

	if (g_benchmark && benchmarkIter == 0 && g_frame == 1)
	{
		// check and see if the first scene is the same as the first benchmark
		// switch to benchmark if it is not the same
		if (strcmp(benchmarkList[0], g_scenes[g_scene]->GetName()) == 0)
			benchmarkIter++;
		else
			g_frame = -1;
	}

	if (g_frame == 200)
	{
		memset(&sTimersSum, 0, sizeof(NvFlexTimers));
		sTotalFrameTime = 0.0f;
		sSamples = 0;
		g_emit = true;
		sDTimersSum.resize(g_numDetailTimers);
	}
	if (g_frame >= 200 && g_frame < 400)
	{
		sTotalFrameTime += g_realdt * 1000.0f; //convert to milliseconds

		for (int i = 0; i < g_numDetailTimers; i++) {
			sDTimersSum[i].name = g_detailTimers[i].name;
			sDTimersSum[i].time += g_detailTimers[i].time;
		}

		sTimersSum.total += g_timers.total;

		sSamples++;
	}
	if (g_frame == 400)
	{

		for (int i = 0; i < g_numDetailTimers; i++) {
			sDTimersSum[i].time /= sSamples;
		}

		if (g_teamCity)
		{
			const char* prefix = benchmarkChartPrefix[benchmarkIter - 1];

			float exclusive = 0.0f;

			for (int i = 0; i < g_numDetailTimers - 1; i++) {
				exclusive += sDTimersSum[i].time;
			}

			printf("##teamcity[buildStatisticValue key='%s_FrameTime' value='%f']\n", prefix, sTotalFrameTime / sSamples);
			printf("##teamcity[buildStatisticValue key='%s_SumKernel' value='%f']\n", prefix, exclusive);

			for (int i = 0; i < g_numDetailTimers - 1; i++) {
				printf("##teamcity[buildStatisticValue key='%s_%s' value='%f']\n", prefix, sDTimersSum[i].name, sDTimersSum[i].time);
			}
			printf("\n");
		}
		else
		{
			printf("Scene: %s\n", g_scenes[g_scene]->GetName());
			printf("FrameTime               %f\n", sTotalFrameTime / sSamples);
			printf("________________________________\n");
			float exclusive = 0.0f;

			for (int i = 0; i < g_numDetailTimers-1; i++) {
				exclusive += sDTimersSum[i].time;
				printf("%s %f\n", sDTimersSum[i].name, sDTimersSum[i].time);
			}
			printf("Sum(exclusive) %f\n", exclusive);
			printf("Sum(inclusive) %f\n", sDTimersSum[g_numDetailTimers - 1].time);
			printf("________________________________\n");
		}

		// Dumping benchmark data to txt files

		g_benchmarkFile.open(g_benchmarkFilename, std::ofstream::out | std::ofstream::app);
		g_benchmarkFile << std::fixed << std::setprecision(6);
		g_benchmarkFile << "Scene: " << g_scenes[g_scene]->GetName() << std::endl;
		g_benchmarkFile << "FrameTime               " << sTotalFrameTime / sSamples << std::endl;
		g_benchmarkFile << "________________________________" << std::endl;
		float exclusive = 0.0f;

		for (int i = 0; i < g_numDetailTimers - 1; i++) {
			exclusive += sDTimersSum[i].time;
			g_benchmarkFile << sDTimersSum[i].name<<" "<< sDTimersSum[i].time << std::endl;
		}

		g_benchmarkFile << "Sum(exclusive) "<< exclusive << std::endl;
		g_benchmarkFile << "Sum(inclusive) "<< sDTimersSum[g_numDetailTimers - 1].time<< std::endl;
		g_benchmarkFile << "________________________________" << std::endl << std::endl;
		g_benchmarkFile.close();

		if (g_benchmark)
		{

#if 0
			// Do basic kinetic energy verification check to ensure that the benchmark runs correctly
			NvFlexGetVelocities(g_flex, g_buffers->velocities.buffer, g_buffers->velocities.size());

			float sumVelocities = 0.0f;
			for (int i = 0; i < g_buffers->velocities.size(); ++i)
			{
				sumVelocities += g_buffers->velocities[i].x * g_buffers->velocities[i].x + g_buffers->velocities[i].y * g_buffers->velocities[i].y + g_buffers->velocities[i].z * g_buffers->velocities[i].z;
			}
			// Tolerance 50%
			int benchmark_id = benchmarkIter - 1;
			if (sumVelocities > (benchmarkEnergyCheck[benchmark_id] * 1.50) ||
				sumVelocities < (benchmarkEnergyCheck[benchmark_id] * 0.50))
				printf("Benchmark kinetic energy verification failed! Expected: [%f], Actual: [%f]\n\n", benchmarkEnergyCheck[benchmark_id], sumVelocities);
#endif

			g_frame = -1;
		}
	}

	if (g_benchmark && g_frame == -1)
	{
		if (benchmarkIter == numBenchmarks)
			exit(0);

		for (int i = 0; i < int(g_scenes.size()); ++i)
		{
			if (strcmp(benchmarkList[benchmarkIter], g_scenes[i]->GetName()) == 0)
				newScene = i;
		}
		assert(newScene != -1);

		benchmarkIter++;
	}

	return newScene;
}

void BenchmarkInit()
{
	g_benchmarkFile.open(g_benchmarkFilename, std::ofstream::out | std::ofstream::app);
	g_benchmarkFile << "Compute Device: " << g_deviceName << std::endl;
	g_benchmarkFile << "HLSL Extensions: " << (g_extensions ? "ON" : "OFF") << std::endl << std::endl;
	g_benchmarkFile.close();
}


