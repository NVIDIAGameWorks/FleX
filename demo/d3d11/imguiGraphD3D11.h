/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef IMGUI_GRAPH_D3D11_H
#define IMGUI_GRAPH_D3D11_H

#include <stdint.h>

#include "../d3d/imguiGraph.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

struct ImguiGraphDescD3D11
{
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	int winW;
	int winH;

	uint32_t maxVertices = 64 * 4096u;

	ImguiGraphDescD3D11() {}
};

inline const ImguiGraphDescD3D11* cast_to_imguiGraphDescD3D11(const ImguiGraphDesc* desc)
{
	return (const ImguiGraphDescD3D11*)(desc);
}

inline ImguiGraphDesc* cast_from_imguiGraphDescD3D11(ImguiGraphDescD3D11* desc)
{
	return (ImguiGraphDesc*)(desc);
}

IMGUI_GRAPH_API void imguiGraphContextInitD3D11(const ImguiGraphDesc* desc);

IMGUI_GRAPH_API void imguiGraphContextUpdateD3D11(const ImguiGraphDesc* desc);

IMGUI_GRAPH_API void imguiGraphContextDestroyD3D11();

IMGUI_GRAPH_API void imguiGraphRecordBeginD3D11();

IMGUI_GRAPH_API void imguiGraphRecordEndD3D11();

IMGUI_GRAPH_API void imguiGraphVertex2fD3D11(float x, float y);

IMGUI_GRAPH_API void imguiGraphVertex2fvD3D11(const float* v);

IMGUI_GRAPH_API void imguiGraphTexCoord2fD3D11(float u, float v);

IMGUI_GRAPH_API void imguiGraphColor4ubD3D11(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

IMGUI_GRAPH_API void imguiGraphColor4ubvD3D11(const uint8_t* v);

IMGUI_GRAPH_API void imguiGraphFontTextureEnableD3D11();

IMGUI_GRAPH_API void imguiGraphFontTextureDisableD3D11();

IMGUI_GRAPH_API void imguiGraphEnableScissorD3D11(int x, int y, int width, int height);

IMGUI_GRAPH_API void imguiGraphDisableScissorD3D11();

IMGUI_GRAPH_API void imguiGraphFontTextureInitD3D11(unsigned char* data);

IMGUI_GRAPH_API void imguiGraphFontTextureReleaseD3D11();

#endif