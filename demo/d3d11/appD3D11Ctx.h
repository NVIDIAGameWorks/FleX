/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include "../d3d/appGraphCtx.h"

#include <windows.h>
#include <d3d11.h>

struct AppGraphProfilerD3D11;

struct AppGraphCtxD3D11
{
	HWND                    m_hWnd = nullptr;

	int m_winW = 0;
	int m_winH = 0;
	bool m_fullscreen = false;
	bool m_valid = false;

	size_t m_dedicatedVideoMemory = 0u;

	// D3D11 objects
	D3D11_VIEWPORT				m_viewport = {};
	ID3D11Device*				m_device = nullptr;
	ID3D11DeviceContext*		m_deviceContext = nullptr;
	IDXGISwapChain*				m_swapChain = nullptr;
	ID3D11Texture2D*			m_backBuffer = nullptr;
	ID3D11RenderTargetView*		m_rtv = nullptr;
	ID3D11Texture2D*			m_depthStencil = nullptr;
	ID3D11DepthStencilView*		m_dsv = nullptr;
	ID3D11ShaderResourceView*	m_depthSRV = nullptr;
	ID3D11DepthStencilState*	m_depthState = nullptr;
	
	AppGraphProfilerD3D11* m_profiler = nullptr;

	AppGraphCtxD3D11();
	~AppGraphCtxD3D11();
};

inline AppGraphCtxD3D11* cast_to_AppGraphCtxD3D11(AppGraphCtx* appctx)
{
	return (AppGraphCtxD3D11*)(appctx);
}

inline AppGraphCtx* cast_from_AppGraphCtxD3D11(AppGraphCtxD3D11* appctx)
{
	return (AppGraphCtx*)(appctx);
}

APP_GRAPH_CTX_API AppGraphCtx* AppGraphCtxCreateD3D11(int deviceID);

APP_GRAPH_CTX_API bool AppGraphCtxUpdateSizeD3D11(AppGraphCtx* context, SDL_Window* window, bool fullscreen, int numMSAASamples);

APP_GRAPH_CTX_API void AppGraphCtxReleaseRenderTargetD3D11(AppGraphCtx* context);

APP_GRAPH_CTX_API void AppGraphCtxReleaseD3D11(AppGraphCtx* context);

APP_GRAPH_CTX_API void AppGraphCtxFrameStartD3D11(AppGraphCtx* context, AppGraphColor clearColor);

APP_GRAPH_CTX_API void AppGraphCtxFramePresentD3D11(AppGraphCtx* context, bool fullsync);

APP_GRAPH_CTX_API void AppGraphCtxWaitForFramesD3D11(AppGraphCtx* context, unsigned int maxFramesInFlight);

APP_GRAPH_CTX_API void AppGraphCtxProfileEnableD3D11(AppGraphCtx* context, bool enabled);

APP_GRAPH_CTX_API void AppGraphCtxProfileBeginD3D11(AppGraphCtx* context, const char* label);

APP_GRAPH_CTX_API void AppGraphCtxProfileEndD3D11(AppGraphCtx* context, const char* label);

APP_GRAPH_CTX_API bool AppGraphCtxProfileGetD3D11(AppGraphCtx* context, const char** plabel, float* cpuTime, float* gpuTime, int index);

APP_GRAPH_CTX_API size_t AppGraphCtxDedicatedVideoMemoryD3D11(AppGraphCtx* context);
