/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
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

#include "imguiGraph.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

struct ImguiGraphDesc
{
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	int winW;
	int winH;

	uint32_t maxVertices = 64 * 4096u;

	ImguiGraphDesc() {}
};

// Below are the functions that must be implemented per graphics API

void imguiGraphContextInit(const ImguiGraphDesc* desc);

void imguiGraphContextUpdate(const ImguiGraphDesc* desc);

void imguiGraphContextDestroy();

void imguiGraphRecordBegin();

void imguiGraphRecordEnd();

void imguiGraphVertex2f(float x, float y);

void imguiGraphVertex2fv(const float* v);

void imguiGraphTexCoord2f(float u, float v);

void imguiGraphColor4ub(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

void imguiGraphColor4ubv(const uint8_t* v);

void imguiGraphFontTextureEnable();

void imguiGraphFontTextureDisable();

void imguiGraphEnableScissor(int x, int y, int width, int height);

void imguiGraphDisableScissor();

void imguiGraphFontTextureInit(unsigned char* data);

void imguiGraphFontTextureRelease();

#endif