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

#include <windows.h>
#include <d3d11.h>


template <class T>
void inline COMRelease(T& t)
{
	if (t) t->Release();
	t = nullptr;
}

template <class T>
void inline COMRelease(T& t, UINT arraySize)
{
	for (UINT i = 0; i < arraySize; i++)
	{
		if (t[i]) t[i]->Release();
		t[i] = nullptr;
	}
}



struct SDL_Window;
struct AppGraphProfiler;

struct AppGraphCtx
{
	HWND                    m_hWnd = nullptr;

	int m_winW = 0;
	int m_winH = 0;
	bool m_fullscreen = false;

	size_t m_dedicatedVideoMemory = 0u;

	// D3D11 objects
	D3D11_VIEWPORT				m_viewport;
	ID3D11Device*				m_device = nullptr;
	ID3D11DeviceContext*		m_deviceContext = nullptr;
	IDXGISwapChain*				m_swapChain = nullptr;
	ID3D11Texture2D*			m_backBuffer = nullptr;
	ID3D11RenderTargetView*		m_rtv = nullptr;
	ID3D11Texture2D*			m_depthStencil = nullptr;
	ID3D11DepthStencilView*		m_dsv = nullptr;
	ID3D11ShaderResourceView*	m_depthSRV = nullptr;
	ID3D11DepthStencilState*	m_depthState = nullptr;
	ID3D11BlendState*			m_blendState = nullptr;

	ID3D11Texture2D*			m_resolvedTarget = nullptr;
	ID3D11ShaderResourceView*	m_resolvedTargetSRV = nullptr;

	AppGraphProfiler* m_profiler = nullptr;

	AppGraphCtx();
	~AppGraphCtx();
};

AppGraphCtx* AppGraphCtxCreate(int deviceID);

void AppGraphCtxInitRenderTarget(AppGraphCtx* context, SDL_Window* window, bool fullscreen, int);

void AppGraphCtxReleaseRenderTarget(AppGraphCtx* context);

void AppGraphCtxRelease(AppGraphCtx* context);

void AppGraphCtxFrameStart(AppGraphCtx* context, float clearColor[4]);

void AppGraphCtxFramePresent(AppGraphCtx* context, bool fullsync);

void AppGraphCtxResolveFrame(AppGraphCtx* context);

void AppGraphCtxProfileEnable(AppGraphCtx* context, bool enabled);

void AppGraphCtxProfileBegin(AppGraphCtx* context, const char* label);

void AppGraphCtxProfileEnd(AppGraphCtx* context, const char* label);

bool AppGraphCtxProfileGet(AppGraphCtx* context, const char** plabel, float* cpuTime, float* gpuTime, int index);

size_t AppGraphCtxDedicatedVideoMemory(AppGraphCtx* context);