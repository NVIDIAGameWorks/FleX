/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <SDL.h>

#include "loader.h"
#include "loaderMacros.h"

#include "appGraphCtx.h"

#include <stdio.h>

#define LOADER_APPC_CTX_FUNCTIONS(op, inst, inst_func) \
	op(inst, inst_func, AppGraphCtx*, AppGraphCtxCreate,		1, ((int, deviceID)) ) \
	op(inst, inst_func, bool, AppGraphCtxUpdateSize,			4, ((AppGraphCtx*, context), (SDL_Window*, window), (bool, fullscreen), (int, numMSAASamples)) ) \
	op(inst, inst_func, void, AppGraphCtxReleaseRenderTarget,	1, ((AppGraphCtx*, context)) ) \
	op(inst, inst_func, void, AppGraphCtxRelease,				1, ((AppGraphCtx*, context)) ) \
	op(inst, inst_func, void, AppGraphCtxFrameStart,			2, ((AppGraphCtx*, context), (AppGraphColor, clearColor)) ) \
	op(inst, inst_func, void, AppGraphCtxFramePresent,			2, ((AppGraphCtx*, context), (bool, fullsync)) ) \
	op(inst, inst_func, void, AppGraphCtxWaitForFrames,			2, ((AppGraphCtx*, context), (unsigned int, maxFramesInFlight)) ) \
	op(inst, inst_func, void, AppGraphCtxProfileEnable,			2, ((AppGraphCtx*, context), (bool, enabled)) ) \
	op(inst, inst_func, void, AppGraphCtxProfileBegin,			2, ((AppGraphCtx*, context), (const char*, label)) ) \
	op(inst, inst_func, void, AppGraphCtxProfileEnd,			2, ((AppGraphCtx*, context), (const char*, label)) ) \
	op(inst, inst_func, bool, AppGraphCtxProfileGet,			5, ((AppGraphCtx*, context), (const char**, plabel), (float*, cpuTime), (float*, gpuTime), (int, index)) ) \
	op(inst, inst_func, size_t, AppGraphCtxDedicatedVideoMemory,	1, ((AppGraphCtx*, context)) ) 

LOADER_APPC_CTX_FUNCTIONS(LOADER_DECLARE_FUNCTION_PTR, (), ())

struct AppCtxFunctionSet
{
	LOADER_APPC_CTX_FUNCTIONS(LOADER_DECLARE_FUNCTION_VAR, (), ())
};

// Declare D3D11/D3D12 versions
#define APP_CTX_D3D11_FUNCTION_NAME(x) x##D3D11
#define APP_CTX_D3D12_FUNCTION_NAME(x) x##D3D12

extern "C" {

LOADER_APPC_CTX_FUNCTIONS(LOADER_DECLARE_FUNCTION_NAME, (), APP_CTX_D3D11_FUNCTION_NAME)
LOADER_APPC_CTX_FUNCTIONS(LOADER_DECLARE_FUNCTION_NAME, (), APP_CTX_D3D12_FUNCTION_NAME)

} // extern "C"

static AppCtxFunctionSet g_functionSet;

void loadAppGraphCtx(AppGraphCtxType type)
{
	if (type == APP_CONTEXT_D3D11)
	{
		LOADER_APPC_CTX_FUNCTIONS(LOADER_SET_FUNCTION, g_functionSet, APP_CTX_D3D11_FUNCTION_NAME)
	}
	if (type == APP_CONTEXT_D3D12)
	{
		LOADER_APPC_CTX_FUNCTIONS(LOADER_SET_FUNCTION, g_functionSet, APP_CTX_D3D12_FUNCTION_NAME)
	}
}

void unloadAppGraphCtx()
{
	g_functionSet = AppCtxFunctionSet{};
}

// Generate the patches 
LOADER_APPC_CTX_FUNCTIONS(LOADER_FUNCTION_PTR_CALL, g_functionSet, ())
