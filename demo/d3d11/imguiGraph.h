/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include <stdint.h>

#define IMGUI_GRAPH_API extern "C" __declspec(dllexport)

struct ImguiGraphDesc;

typedef bool (*imguiGraphInit_t)(const char* fontpath, const ImguiGraphDesc* desc);

typedef void (*imguiGraphUpdate_t)(const ImguiGraphDesc* desc);

bool imguiGraphInit(const char* fontpath, const ImguiGraphDesc* desc);

void imguiGraphUpdate(const ImguiGraphDesc* desc);

void imguiGraphDestroy();

void imguiGraphDraw();