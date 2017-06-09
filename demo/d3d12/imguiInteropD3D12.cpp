/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

 //direct3d headers
#define NOMINMAX
#include <d3d12.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d12.lib")

#include "imguiGraphD3D12.h"

#include "appD3D12Ctx.h"

struct AppGraphCtx;

namespace NvCo = nvidia::Common;

inline void imguiInteropUpdateDesc(ImguiGraphDescD3D12& desc, AppGraphCtx* appctxIn)
{
	auto context = cast_to_AppGraphCtxD3D12(appctxIn);

	desc.device = context->m_device;
	desc.commandList = context->m_commandList;

	desc.lastFenceCompleted = 0;
	desc.nextFenceValue = 1;

	desc.winW = context->m_winW;
	desc.winH = context->m_winH;
	desc.numMSAASamples = context->m_numMsaaSamples;
	desc.dynamicHeapCbvSrvUav.userdata = context;
	desc.dynamicHeapCbvSrvUav.reserveDescriptors = nullptr;
}

IMGUI_GRAPH_API bool imguiInteropGraphInitD3D12(imguiGraphInit_t func, const char* fontpath, AppGraphCtx* appctx);

IMGUI_GRAPH_API void imguiInteropGraphUpdateD3D12(imguiGraphUpdate_t func, AppGraphCtx* appctx);

bool imguiInteropGraphInitD3D12(imguiGraphInit_t func, const char* fontpath, AppGraphCtx* appctx)
{
	ImguiGraphDescD3D12 desc;
	imguiInteropUpdateDesc(desc, appctx);

	return func(fontpath, cast_from_imguiGraphDescD3D12(&desc));
}

void imguiInteropGraphUpdateD3D12(imguiGraphUpdate_t func, AppGraphCtx* appctx)
{
	ImguiGraphDescD3D12 desc;
	imguiInteropUpdateDesc(desc, appctx);

	return func(cast_from_imguiGraphDescD3D12(&desc));
}