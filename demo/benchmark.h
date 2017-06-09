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
#include <algorithm>
#include <stdint.h>

const char* g_benchmarkFilename = "../../benchmark.txt";
std::wofstream g_benchmarkFile;

const int benchmarkPhaseFrameCount = 400;
const int benchmarkEndWarmup = 200;

const int benchmarkAsyncOffDummyOnBeginFrame = benchmarkPhaseFrameCount;
const int benchmarkAsyncOnDummyOnBeginFrame = benchmarkPhaseFrameCount*2;
const int benchmarkEndFrame = benchmarkPhaseFrameCount*3;
const char* benchmarkList[] = { "Env Cloth Small", "Viscosity Med", "Inflatables", "Game Mesh Particles", "Rigid4" };
const char* benchmarkChartPrefix[] = { "EnvClothSmall", "ViscosityMed", "Inflatables", "GameMeshParticles", "Rigid4" }; //no spaces
int numBenchmarks = sizeof(benchmarkList)/sizeof(benchmarkList[0]);

struct GpuTimers
{
	unsigned long long renderBegin;
	unsigned long long renderEnd;
	unsigned long long renderFreq;
	unsigned long long computeBegin;
	unsigned long long computeEnd;
	unsigned long long computeFreq;

	static const int maxTimerCount = 4;
	double timers[benchmarkEndFrame][maxTimerCount];
	int timerCount[benchmarkEndFrame];
};


struct TimerTotals
{
	std::vector<NvFlexDetailTimer> detailTimers;
	
	float frameTime;
	int samples;

	float frameTimeAsync;
	int samplesAsync;

	float computeTimeAsyncOff;
	float computeTimeAsyncOn;
	int computeSamples;

	TimerTotals() : frameTime(0), samples(0), frameTimeAsync(0), samplesAsync(0), computeTimeAsyncOff(0), computeTimeAsyncOn(0), computeSamples(0) {}
};

GpuTimers g_GpuTimers;

int g_benchmarkFrame = 0;
int g_benchmarkScene = 0;
int g_benchmarkSceneNumber;

#if defined(__linux__)
int sprintf_s(char*  const buffer, size_t  const bufferCount,
              const char*  format,...)
{
    va_list args;
    va_start(args, format);
    int retval = vsprintf(buffer, format, args);
    va_end(args);
    
    return retval;	
}	      
#endif

//-----------------------------------------------------------------------------
char* removeSpaces(const char* in)
{
	int len = strlen(in);
	char* out = new char[len+1];

	int i = 0;
	int j = 0;
	while (in[i] != 0)
	{
		if (in[i] != ' ')
		{
			out[j] = in[i];
			j++;
		}
		i++;
	}
	out[j] = 0;

	return out;
}
//-----------------------------------------------------------------------------
void ProcessGpuTimes()
{
	static bool timerfirstTime = true;

	double renderTime;
	double compTime;
	double unionTime;
	double overlapBeginTime;

	int numParticles = NvFlexGetActiveCount(g_solver);

	renderTime = double(g_GpuTimers.renderEnd - g_GpuTimers.renderBegin) / double(g_GpuTimers.renderFreq);
	compTime = double(g_GpuTimers.computeEnd - g_GpuTimers.computeBegin) / double(g_GpuTimers.computeFreq);

	uint64_t minTime = min(g_GpuTimers.renderBegin, g_GpuTimers.computeBegin);
	uint64_t maxTime = max(g_GpuTimers.renderEnd, g_GpuTimers.computeEnd);
	unionTime = double(maxTime - minTime) / double(g_GpuTimers.computeFreq);

	overlapBeginTime = abs((long long)g_GpuTimers.renderBegin - (long long)g_GpuTimers.computeBegin) / double(g_GpuTimers.computeFreq);

	if (!timerfirstTime && g_benchmarkFrame < benchmarkEndFrame)
	{
		if (g_useAsyncCompute)
		{
			g_GpuTimers.timers[g_benchmarkFrame][0] = numParticles;
			g_GpuTimers.timers[g_benchmarkFrame][1] = unionTime * 1000;
			g_GpuTimers.timers[g_benchmarkFrame][2] = overlapBeginTime * 1000;
			g_GpuTimers.timers[g_benchmarkFrame][3] = g_realdt * 1000;
			g_GpuTimers.timerCount[g_benchmarkFrame] = 4;
		}
		else
		{
			g_GpuTimers.timers[g_benchmarkFrame][0] = numParticles;
			g_GpuTimers.timers[g_benchmarkFrame][1] = renderTime * 1000;
			g_GpuTimers.timers[g_benchmarkFrame][2] = compTime * 1000;
			g_GpuTimers.timers[g_benchmarkFrame][3] = g_realdt * 1000;
			g_GpuTimers.timerCount[g_benchmarkFrame] = 4;
		}
	}
	timerfirstTime = false;
}
//-----------------------------------------------------------------------------
void UpdateTotals(TimerTotals& totals)
{
	// Phase 0B, async off, dummy work off
	if (benchmarkEndWarmup <= g_benchmarkFrame && g_benchmarkFrame < benchmarkAsyncOffDummyOnBeginFrame)
	{
		totals.frameTime += g_realdt * 1000.0f; //convert to milliseconds

		for (int i = 0; i < g_numDetailTimers; i++) {
			strcpy(totals.detailTimers[i].name,g_detailTimers[i].name);
			totals.detailTimers[i].time += g_detailTimers[i].time;
		}

		totals.samples++;
	}

	// Phase 2B, async on, dummy work on
	if (benchmarkAsyncOnDummyOnBeginFrame + benchmarkEndWarmup <= g_benchmarkFrame)
	{
		float offGraphics = (float)g_GpuTimers.timers[g_benchmarkFrame - benchmarkPhaseFrameCount][1];
		float offCompute = (float)g_GpuTimers.timers[g_benchmarkFrame - benchmarkPhaseFrameCount][2];
		float onBoth = (float)g_GpuTimers.timers[g_benchmarkFrame][1];

		float onCompute = onBoth - offGraphics;

		totals.computeTimeAsyncOff += offCompute;
		totals.computeTimeAsyncOn += onCompute;
		totals.computeSamples++;

		totals.frameTimeAsync += g_realdt * 1000.0f; //convert to milliseconds
		totals.samplesAsync++;
	}
}
//-----------------------------------------------------------------------------
void BeginNewPhaseIfNecessary(int& sceneToSwitchTo,TimerTotals& totals)
{
	// Are we beginning phase 0B?
	if (g_benchmarkFrame == benchmarkEndWarmup)
	{
		totals.frameTime = 0.0f;
		totals.samples = 0;
		g_emit = true;
		totals.detailTimers.resize(g_numDetailTimers);

		for (int i = 0; i != g_numDetailTimers; i++)
		{
			totals.detailTimers[i].name = new char[256];
		}
	}

	// Are we beginning phase 1?
	if (g_benchmarkFrame == benchmarkAsyncOffDummyOnBeginFrame)
	{
		sceneToSwitchTo = g_benchmarkSceneNumber;
		g_useAsyncCompute = false;
		g_increaseGfxLoadForAsyncComputeTesting = true;
	}

	// Are we beginning phase 2?
	if (g_benchmarkFrame == benchmarkAsyncOnDummyOnBeginFrame)
	{
		sceneToSwitchTo = g_benchmarkSceneNumber;
		g_useAsyncCompute = true;
		g_increaseGfxLoadForAsyncComputeTesting = true;
	}

	// Are we beginning phase 2B?
	if (g_benchmarkFrame == benchmarkAsyncOnDummyOnBeginFrame + benchmarkEndWarmup)
	{
		totals.frameTimeAsync = 0.0f;
		totals.samplesAsync = 0;
		totals.computeTimeAsyncOff = 0.0f;
		totals.computeTimeAsyncOn = 0.0f;
		totals.computeSamples = 0;
		g_emit = true;
	}
}
//-----------------------------------------------------------------------------
void WriteSceneResults(TimerTotals& totals)
{
	// Write results for scene
	for (int i = 0; i < g_numDetailTimers; i++) {
		totals.detailTimers[i].time /= totals.samples;
	}

	if (g_profile && g_teamCity)
	{
		const char* prefix = benchmarkChartPrefix[g_benchmarkScene - 1];

		float exclusive = 0.0f;

		for (int i = 0; i < g_numDetailTimers - 1; i++) {
			exclusive += totals.detailTimers[i].time;
		}

		printf("##teamcity[buildStatisticValue key='%s_FrameTime' value='%f']\n", prefix, totals.frameTime / totals.samples);
		printf("##teamcity[buildStatisticValue key='%s_SumKernel' value='%f']\n", prefix, exclusive);

		for (int i = 0; i < g_numDetailTimers - 1; i++) {
			printf("##teamcity[buildStatisticValue key='%s_%s' value='%f']\n", prefix, totals.detailTimers[i].name, totals.detailTimers[i].time);
		}
		printf("\n");
	}

	printf("Scene: %s\n", g_scenes[g_scene]->GetName());
	printf("FrameTime               %f\n", totals.frameTime / totals.samples);
	printf("________________________________\n");
	float exclusive = 0.0f;

	for (int i = 0; i < g_numDetailTimers - 1; i++) {
		exclusive += totals.detailTimers[i].time;
		printf("%s %f\n", totals.detailTimers[i].name, totals.detailTimers[i].time);
	}
	printf("Sum(exclusive) %f\n", exclusive);
	printf("Sum(inclusive) %f\n", totals.detailTimers[g_numDetailTimers - 1].time);
	printf("________________________________\n");

	// Dumping benchmark data to txt files

	g_benchmarkFile.open(g_benchmarkFilename, std::ofstream::out | std::ofstream::app);
	g_benchmarkFile << std::fixed << std::setprecision(6);
	g_benchmarkFile << "Scene: " << g_scenes[g_scene]->GetName() << std::endl;
	g_benchmarkFile << "FrameTime               " << totals.frameTime / totals.samples << std::endl;
	g_benchmarkFile << "________________________________" << std::endl;

	if (g_profile)
	{
		float exclusive = 0.0f;

		g_benchmarkFile << std::fixed << std::setprecision(6);

		for (int i = 0; i < g_numDetailTimers - 1; i++) {
			exclusive += totals.detailTimers[i].time;
			g_benchmarkFile << totals.detailTimers[i].name << " " << totals.detailTimers[i].time << std::endl;

			delete totals.detailTimers[i].name;
		}

		g_benchmarkFile << "Sum(exclusive) " << exclusive << std::endl;
		g_benchmarkFile << "Sum(inclusive) " << totals.detailTimers[g_numDetailTimers - 1].time << std::endl;
		g_benchmarkFile << "________________________________" << std::endl << std::endl;
	}

	if (g_outputAllFrameTimes)
	{
		for (int i = 0; i != benchmarkEndFrame; i++)
		{
			g_benchmarkFile << g_GpuTimers.timers[i][3] << std::endl;
		}

		// Per frame timers
		for (int i = benchmarkAsyncOffDummyOnBeginFrame; i != benchmarkAsyncOnDummyOnBeginFrame; i++)
		{
			for (int j = 0; j != g_GpuTimers.timerCount[i]; j++)
			{
				g_benchmarkFile << g_GpuTimers.timers[i][j] << " ";
			}

			for (int j = 0; j != g_GpuTimers.timerCount[i + benchmarkPhaseFrameCount]; j++)
			{
				g_benchmarkFile << g_GpuTimers.timers[i + benchmarkPhaseFrameCount][j] << " ";
			}

			g_benchmarkFile << std::endl;
		}

	}

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
		int benchmark_id = g_benchmarkScene - 1;
		if (sumVelocities >(benchmarkEnergyCheck[benchmark_id] * 1.50) ||
			sumVelocities < (benchmarkEnergyCheck[benchmark_id] * 0.50))
			printf("Benchmark kinetic energy verification failed! Expected: [%f], Actual: [%f]\n\n", benchmarkEnergyCheck[benchmark_id], sumVelocities);
#endif

	}
}
//-----------------------------------------------------------------------------
int GoToNextScene()
{
	int sceneToSwitchTo = -1;

	// Advance to next benchmark scene
	for (int i = 0; i < int(g_scenes.size()); ++i)
	{
		if (strcmp(benchmarkList[g_benchmarkScene], g_scenes[i]->GetName()) == 0)
		{
			sceneToSwitchTo = i;
			g_benchmarkSceneNumber = i;
		}
	}
	assert(sceneToSwitchTo != -1);

	g_useAsyncCompute = false;
	g_increaseGfxLoadForAsyncComputeTesting = false;

	return sceneToSwitchTo;
}
//-----------------------------------------------------------------------------
// Returns scene number if benchmark wants to switch scene, -1 otherwise
//-----------------------------------------------------------------------------
int BenchmarkUpdate()
{
	static TimerTotals s_totals;
	int sceneToSwitchTo = -1;

	if (!g_benchmark) return sceneToSwitchTo;

	ProcessGpuTimes();
	UpdateTotals(s_totals);

	// Next frame
	g_benchmarkFrame++;

	BeginNewPhaseIfNecessary(sceneToSwitchTo, s_totals);

	// Check whether at end of scene
	if (g_benchmarkFrame == benchmarkEndFrame)
	{
		WriteSceneResults(s_totals);

		// Next scene
		g_benchmarkScene++;

		// Go to next scene, or exit if all scenes done
		if (g_benchmarkScene != numBenchmarks)
		{
			sceneToSwitchTo = GoToNextScene();

			g_benchmarkFrame = 0;
			g_frame = -1;
		}
		else
		{
			exit(0);
		}
	}

	return sceneToSwitchTo;
}
//-----------------------------------------------------------------------------
int BenchmarkInit()
{
	int sceneToSwitchTo = GoToNextScene();

	return sceneToSwitchTo;
}
//-----------------------------------------------------------------------------
void BenchmarkUpdateGraph()
{
}
//-----------------------------------------------------------------------------
