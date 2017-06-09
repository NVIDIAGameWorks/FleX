/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef IMGUI_GRAPH_D3D12_H
#define IMGUI_GRAPH_D3D12_H

#include <stdint.h>

#include "../d3d/imguiGraph.h"

struct ImguiDescriptorReserveHandleD3D12
{
	ID3D12DescriptorHeap* heap;
	UINT descriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

struct ImguiDynamicDescriptorHeapD3D12
{
	void* userdata;
	ImguiDescriptorReserveHandleD3D12(*reserveDescriptors)(void* userdata, UINT numDescriptors, UINT64 lastFenceCompleted, UINT64 nextFenceValue);
};

struct ImguiGraphDescD3D12
{
	ID3D12Device* device = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;
	UINT64 lastFenceCompleted; 
	UINT64 nextFenceValue;
	int winW;
	int winH;
	int numMSAASamples = 1;

	uint32_t maxVertices = 64 * 4096u;

	ImguiDynamicDescriptorHeapD3D12 dynamicHeapCbvSrvUav = {};

	ImguiGraphDescD3D12() {}
};

inline const ImguiGraphDescD3D12* cast_to_imguiGraphDescD3D12(const ImguiGraphDesc* desc)
{
	return (const ImguiGraphDescD3D12*)(desc);
}

inline ImguiGraphDesc* cast_from_imguiGraphDescD3D12(ImguiGraphDescD3D12* desc)
{
	return (ImguiGraphDesc*)(desc);
}

IMGUI_GRAPH_API void imguiGraphContextInitD3D12(const ImguiGraphDesc* desc);

IMGUI_GRAPH_API void imguiGraphContextUpdateD3D12(const ImguiGraphDesc* desc);

IMGUI_GRAPH_API void imguiGraphContextDestroyD3D12();

IMGUI_GRAPH_API void imguiGraphRecordBeginD3D12();

IMGUI_GRAPH_API void imguiGraphRecordEndD3D12();

IMGUI_GRAPH_API void imguiGraphVertex2fD3D12(float x, float y);

IMGUI_GRAPH_API void imguiGraphVertex2fvD3D12(const float* v);

IMGUI_GRAPH_API void imguiGraphTexCoord2fD3D12(float u, float v);

IMGUI_GRAPH_API void imguiGraphColor4ubD3D12(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

IMGUI_GRAPH_API void imguiGraphColor4ubvD3D12(const uint8_t* v);

IMGUI_GRAPH_API void imguiGraphFontTextureEnableD3D12();

IMGUI_GRAPH_API void imguiGraphFontTextureDisableD3D12();

IMGUI_GRAPH_API void imguiGraphEnableScissorD3D12(int x, int y, int width, int height);

IMGUI_GRAPH_API void imguiGraphDisableScissorD3D12();

IMGUI_GRAPH_API void imguiGraphFontTextureInitD3D12(unsigned char* data);

IMGUI_GRAPH_API void imguiGraphFontTextureReleaseD3D12();

#endif