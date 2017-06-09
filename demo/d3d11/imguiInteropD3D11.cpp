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
#include <d3d11.h>

#include "appD3D11Ctx.h"

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")

#include "appD3D11Ctx.h"
#include "imguiGraphD3D11.h"

IMGUI_GRAPH_API bool imguiInteropGraphInitD3D11(imguiGraphInit_t func, const char* fontpath, AppGraphCtx* appctx);

IMGUI_GRAPH_API void imguiInteropGraphUpdateD3D11(imguiGraphUpdate_t func, AppGraphCtx* appctx);

bool imguiInteropGraphInitD3D11(imguiGraphInit_t func, const char* fontpath, AppGraphCtx* appctxIn)
{
	ImguiGraphDescD3D11 desc;

	AppGraphCtxD3D11* appctx = cast_to_AppGraphCtxD3D11(appctxIn);

	desc.device = appctx->m_device;
	desc.deviceContext = appctx->m_deviceContext;
	desc.winW = appctx->m_winW;
	desc.winH = appctx->m_winH;

	return func(fontpath, cast_from_imguiGraphDescD3D11(&desc));
}

void imguiInteropGraphUpdateD3D11(imguiGraphUpdate_t func, AppGraphCtx* appctxIn)
{
	ImguiGraphDescD3D11 desc;
	AppGraphCtxD3D11* appctx = cast_to_AppGraphCtxD3D11(appctxIn);

	desc.device = appctx->m_device;
	desc.deviceContext = appctx->m_deviceContext;
	desc.winW = appctx->m_winW;
	desc.winH = appctx->m_winH;

	return func(cast_from_imguiGraphDescD3D11(&desc));
}