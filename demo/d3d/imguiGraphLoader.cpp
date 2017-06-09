/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

//#include <SDL.h>

#include "loader.h"

#include "imguiGraph.h"
#include "loaderMacros.h"

#include <stdio.h>

#include "imguiGraph.h"

struct AppGraphCtx;

#define LOADER_IMGUI_GRAPH_FUNCTIONS(op, inst, inst_func) \
	op(inst, inst_func, void, imguiGraphContextInit,		1, ((const ImguiGraphDesc*, desc)) ) \
	op(inst, inst_func, void, imguiGraphContextUpdate,		1, ((const ImguiGraphDesc*, desc)) ) \
	op(inst, inst_func, void, imguiGraphContextDestroy,		0, (()) ) \
	op(inst, inst_func, void, imguiGraphRecordBegin,		0, (()) ) \
	op(inst, inst_func, void, imguiGraphRecordEnd,			0, (()) ) \
	op(inst, inst_func, void, imguiGraphVertex2f,			2, ((float, x), (float, y)) ) \
	op(inst, inst_func, void, imguiGraphVertex2fv,			1, ((const float*, v)) ) \
	op(inst, inst_func, void, imguiGraphTexCoord2f,			2, ((float, u), (float, v)) ) \
	op(inst, inst_func, void, imguiGraphColor4ub,			4, ((uint8_t, red), (uint8_t, green), (uint8_t, blue), (uint8_t, alpha)) ) \
	op(inst, inst_func, void, imguiGraphColor4ubv,			1, ((const uint8_t*, v)) ) \
	op(inst, inst_func, void, imguiGraphFontTextureEnable,	0, (()) ) \
	op(inst, inst_func, void, imguiGraphFontTextureDisable,	0, (()) ) \
	op(inst, inst_func, void, imguiGraphEnableScissor,		4, ((int, x), (int, y), (int, width), (int, height)) ) \
	op(inst, inst_func, void, imguiGraphDisableScissor,		0, (()) ) \
	op(inst, inst_func, void, imguiGraphFontTextureInit,	1, ((unsigned char*, data)) ) \
	op(inst, inst_func, void, imguiGraphFontTextureRelease,	0, (()) ) \
	op(inst, inst_func, bool, imguiInteropGraphInit,		3, ((imguiGraphInit_t, func), (const char*, fontpath), (AppGraphCtx*, appctx)) ) \
	op(inst, inst_func, void, imguiInteropGraphUpdate,		2, ((imguiGraphUpdate_t, func), (AppGraphCtx*, appctx)) )

LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_DECLARE_FUNCTION_PTR, (), ())

struct ImguiFunctionSet 
{
LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_DECLARE_FUNCTION_VAR, (), ())
};

// Declare D3D11/D3D12 versions
#define IMGUI_D3D11_FUNCTION_NAME(x) x##D3D11
#define IMGUI_D3D12_FUNCTION_NAME(x) x##D3D12

extern "C" {

LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_DECLARE_FUNCTION_NAME, (), IMGUI_D3D11_FUNCTION_NAME)
LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_DECLARE_FUNCTION_NAME, (), IMGUI_D3D12_FUNCTION_NAME)

} // extern "C"

static ImguiFunctionSet g_functionSet;

void loadImgui(AppGraphCtxType type)
{
	if (type == APP_CONTEXT_D3D11)
	{
		LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_SET_FUNCTION, g_functionSet, IMGUI_D3D11_FUNCTION_NAME)
	}
	if (type == APP_CONTEXT_D3D12)
	{
		LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_SET_FUNCTION, g_functionSet, IMGUI_D3D12_FUNCTION_NAME)
	}
}

void unloadImgui()
{
	g_functionSet = ImguiFunctionSet{};
}

// Generate the patches 
LOADER_IMGUI_GRAPH_FUNCTIONS(LOADER_FUNCTION_PTR_CALL, g_functionSet, ())


