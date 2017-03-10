/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

//direct3d headers
#include <d3d11.h>
#include <dxgi.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "DXGI.lib")

#include "appD3D11Ctx.h"

#include <stdio.h>
#include <math.h>

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>

AppGraphProfiler* appGraphCreateProfiler(AppGraphCtx* ctx);
void appGraphProfilerFrameBegin(AppGraphProfiler* profiler);
void appGraphProfilerFrameEnd(AppGraphProfiler* profiler);
void appGraphProfilerEnable(AppGraphProfiler* profiler, bool enabled);
void appGraphProfilerBegin(AppGraphProfiler* profiler, const char* label);
void appGraphProfilerEnd(AppGraphProfiler* profiler, const char* label);
bool appGraphProfilerGet(AppGraphProfiler* profiler, const char** plabel, float* cpuTime, float* gpuTime, int index);
void appGraphReleaseProfiler(AppGraphProfiler* profiler);

AppGraphCtx::AppGraphCtx()
{
	m_profiler = appGraphCreateProfiler(this);
	memset(&m_viewport, 0, sizeof(m_viewport));
}

AppGraphCtx::~AppGraphCtx()
{
	AppGraphCtxReleaseRenderTarget(this);

	COMRelease(m_swapChain);
	COMRelease(m_backBuffer);
	COMRelease(m_rtv);
	COMRelease(m_depthStencil);
	COMRelease(m_dsv);
	COMRelease(m_depthSRV);
	COMRelease(m_depthState);
	COMRelease(m_blendState);
	COMRelease(m_resolvedTarget);
	COMRelease(m_resolvedTargetSRV);
	COMRelease(m_deviceContext);
	COMRelease(m_device);

	appGraphReleaseProfiler(m_profiler);
	m_profiler = nullptr;
}

AppGraphCtx* AppGraphCtxCreate(int deviceID)
{
	AppGraphCtx* context = new AppGraphCtx;

	HRESULT hr = S_OK;

	// enumerate devices
	IDXGIFactory1* pFactory = NULL;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	IDXGIAdapter1* pAdapterTemp = NULL;
	IDXGIAdapter1* pAdapter = NULL;
	DXGI_ADAPTER_DESC1 adapterDesc;
	int adapterIdx = 0;
	while (S_OK == pFactory->EnumAdapters1(adapterIdx, &pAdapterTemp))
	{
		pAdapterTemp->GetDesc1(&adapterDesc);

		context->m_dedicatedVideoMemory = (size_t)adapterDesc.DedicatedVideoMemory;

		if (deviceID == adapterIdx)
		{
			pAdapter = pAdapterTemp;
			break;
		}
		else
		{
			pAdapterTemp->Release();
		}
		adapterIdx++;
	}

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_UNKNOWN,
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = 4;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = 4;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		D3D_FEATURE_LEVEL featureLevel;
		D3D_DRIVER_TYPE driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(pAdapter, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &context->m_device, &featureLevel, &context->m_deviceContext);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	if (FAILED(hr))
	{
		delete context;
		return nullptr;
	}

	// cleanup adapter and factory
	COMRelease(pAdapter);
	COMRelease(pFactory);

	// create depth state
	D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
	depthStateDesc.DepthEnable = TRUE;
	depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	if (hr = context->m_device->CreateDepthStencilState(&depthStateDesc, &context->m_depthState))
	{
		delete context;
		return nullptr;
	}

	// create blend state
	D3D11_BLEND_DESC blendStateDesc = {};
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	if (hr = context->m_device->CreateBlendState(&blendStateDesc, &context->m_blendState))
	{
		delete context;
		return nullptr;
	}

	return context;
}

void AppGraphCtxInitRenderTarget(AppGraphCtx* context, SDL_Window* window, bool fullscreen, int samples)
{
	// TODO: fix iflip fullscreen support
	fullscreen = false;

	HWND hWnd = nullptr;
	// get Windows handle to this SDL window
	SDL_SysWMinfo winInfo;
	SDL_VERSION(&winInfo.version);
	if (SDL_GetWindowWMInfo(window, &winInfo))
	{
		if (winInfo.subsystem == SDL_SYSWM_WINDOWS)
		{
			hWnd = winInfo.info.win.window;
		}
	}
	context->m_hWnd = hWnd;
	context->m_fullscreen = fullscreen;

	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(context->m_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	context->m_winW = width;
	context->m_winH = height;

	// enumerate devices
	IDXGIFactory1* pFactory = NULL;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));

	// create the swap chain
	for (int i = 0; i < 2; i++)
	{
		DXGI_SWAP_CHAIN_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BufferCount = 1;
		desc.BufferDesc.Width = context->m_winW;
		desc.BufferDesc.Height = context->m_winH;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 0;
		desc.BufferDesc.RefreshRate.Denominator = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.OutputWindow = context->m_hWnd;
		desc.SampleDesc.Count = samples;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = TRUE; // m_fullscreen ? FALSE : TRUE;
		desc.Flags = 0u;

		if (hr = pFactory->CreateSwapChain(context->m_device, &desc, (IDXGISwapChain**)&context->m_swapChain))
		{
			COMRelease(context->m_swapChain);
			context->m_fullscreen = false;
			continue;
		}

		if (!context->m_fullscreen)
		{

		}
		else
		{
			hr = context->m_swapChain->SetFullscreenState(true, nullptr);
			if (hr != S_OK)
			{
				COMRelease(context->m_swapChain);
				context->m_fullscreen = false;
				continue;
			}
			DXGI_SWAP_CHAIN_DESC desc = {};
			context->m_swapChain->GetDesc(&desc);
			context->m_swapChain->ResizeBuffers(1, context->m_winW, context->m_winH, desc.BufferDesc.Format, desc.Flags);
		}
		break;
	}

	// configure scissor and viewport
	{
		context->m_viewport.Width = float(context->m_winW);
		context->m_viewport.Height = float(context->m_winH);
		context->m_viewport.MaxDepth = 1.f;
	}

	COMRelease(pFactory);

	// Create a render target view
	context->m_backBuffer = NULL;
	hr = context->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&context->m_backBuffer);
	if (FAILED(hr))
	{
		return;
	}

	hr = context->m_device->CreateRenderTargetView(context->m_backBuffer, NULL, &context->m_rtv);
	if (FAILED(hr))
	{
		return;
	}

	context->m_deviceContext->OMSetRenderTargets(1, &context->m_rtv, NULL);

	// viewport
	context->m_deviceContext->RSSetViewports(1, &context->m_viewport);

	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R24G8_TYPELESS
		texDesc.SampleDesc.Count = samples;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (hr = context->m_device->CreateTexture2D(&texDesc, nullptr, &context->m_depthStencil))
		{
			return;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
		viewDesc.Flags = 0u;
		viewDesc.Texture2D.MipSlice = 0;

		if (hr = context->m_device->CreateDepthStencilView(context->m_depthStencil, &viewDesc, &context->m_dsv))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		if (hr = context->m_device->CreateShaderResourceView(context->m_depthStencil, &srvDesc, &context->m_depthSRV))
		{
			return;
		}
	}

	// resolved texture target (for refraction / scene sampling)
	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (hr = context->m_device->CreateTexture2D(&texDesc, nullptr, &context->m_resolvedTarget))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		if (hr = context->m_device->CreateShaderResourceView(context->m_resolvedTarget, &srvDesc, &context->m_resolvedTargetSRV))
		{
			return;
		}
	}
}

void AppGraphCtxReleaseRenderTarget(AppGraphCtx* context)
{
	if (context->m_swapChain == nullptr)
	{
		return;
	}

	BOOL bFullscreen = FALSE;
	context->m_swapChain->GetFullscreenState(&bFullscreen, nullptr);
	if (bFullscreen == TRUE) context->m_swapChain->SetFullscreenState(FALSE, nullptr);

	COMRelease(context->m_swapChain);
	COMRelease(context->m_backBuffer);
	COMRelease(context->m_rtv);
	COMRelease(context->m_depthStencil);
	COMRelease(context->m_dsv);
	COMRelease(context->m_depthSRV);
	COMRelease(context->m_resolvedTarget);
	COMRelease(context->m_resolvedTargetSRV);
}

void AppGraphCtxRelease(AppGraphCtx* context)
{
	if (context == nullptr) return;

	delete context;
}

void AppGraphCtxResolveFrame(AppGraphCtx* context)
{
	context->m_deviceContext->ResolveSubresource(context->m_resolvedTarget, 0, context->m_backBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void AppGraphCtxFrameStart(AppGraphCtx* context, float clearColor[4])
{
	appGraphProfilerFrameBegin(context->m_profiler);

	context->m_deviceContext->RSSetViewports(1, &context->m_viewport);
	context->m_deviceContext->RSSetScissorRects(0, nullptr);

	context->m_deviceContext->OMSetRenderTargets(1, &context->m_rtv, context->m_dsv);

	context->m_deviceContext->ClearRenderTargetView(context->m_rtv, clearColor);
	context->m_deviceContext->ClearDepthStencilView(context->m_dsv, D3D11_CLEAR_DEPTH, 1.f, 0u);

	context->m_deviceContext->OMSetDepthStencilState(context->m_depthState, 0u);
}

void AppGraphCtxFramePresent(AppGraphCtx* context, bool fullsync)
{
	context->m_swapChain->Present(fullsync, 0);
	
	appGraphProfilerFrameEnd(context->m_profiler);
}

void AppGraphCtxProfileEnable(AppGraphCtx* context, bool enabled)
{
	appGraphProfilerEnable(context->m_profiler, enabled);
}

void AppGraphCtxProfileBegin(AppGraphCtx* context, const char* label)
{
	appGraphProfilerBegin(context->m_profiler, label);
}

void AppGraphCtxProfileEnd(AppGraphCtx* context, const char* label)
{
	appGraphProfilerEnd(context->m_profiler, label);
}

bool AppGraphCtxProfileGet(AppGraphCtx* context, const char** plabel, float* cpuTime, float* gpuTime, int index)
{
	return appGraphProfilerGet(context->m_profiler, plabel, cpuTime, gpuTime, index);
}

// ******************************* Profiler *********************************

namespace
{
	struct TimerCPU
	{
		LARGE_INTEGER oldCount;
		LARGE_INTEGER count;
		LARGE_INTEGER freq;
		TimerCPU()
		{
			QueryPerformanceCounter(&count);
			QueryPerformanceFrequency(&freq);
			oldCount = count;
		}
		double getDeltaTime()
		{
			QueryPerformanceCounter(&count);
			double dt = double(count.QuadPart - oldCount.QuadPart) / double(freq.QuadPart);
			oldCount = count;
			return dt;
		}
	};

	struct TimerGPU
	{
		ID3D11Query* m_begin = nullptr;
		ID3D11Query* m_end = nullptr;

		TimerGPU() {}
		~TimerGPU()
		{
			COMRelease(m_begin);
			COMRelease(m_end);
		}
	};

	struct Timer
	{
		TimerCPU m_cpu;
		TimerGPU m_gpu;

		const char* m_label = nullptr;
		float m_cpuTime = 0.f;
		float m_gpuTime = 0.f;

		Timer() {}
		~Timer() {}
	};

	struct TimerValue
	{
		const char* m_label = nullptr;
		float m_cpuTime = 0.f;
		float m_gpuTime = 0.f;

		struct Stat
		{
			float m_time = 0.f;
			float m_maxTime = 0.f;
			float m_maxTimeAge = 0.f;

			float m_smoothTime = 0.f;
			float m_smoothTimeSum = 0.f;
			float m_smoothTimeCount = 0.f;

			Stat() {}
			void push(float time)
			{
				m_time = time;

				if (m_time > m_maxTime)
				{
					m_maxTime = m_time;
					m_maxTimeAge = 0.f;
				}

				if (fabsf(m_time - m_maxTime) < 0.25f * m_maxTime)
				{
					m_smoothTimeSum += m_time;
					m_smoothTimeCount += 1.f;
					m_smoothTimeSum *= 0.98f;
					m_smoothTimeCount *= 0.98f;
					m_smoothTime = m_smoothTimeSum / m_smoothTimeCount;
				}
			}

			float pull(float frameTime)
			{
				m_maxTimeAge += frameTime;

				if (m_maxTimeAge > 1.f)
				{
					m_maxTimeAge = 0.f;
					m_maxTime = m_time;
					m_smoothTimeSum = 0.f;
					m_smoothTimeCount = 0.f;
				}
				return m_smoothTime;
			}
		};

		Stat m_cpu;
		Stat m_gpu;

		void push(float cpuTime, float gpuTime)
		{
			m_cpu.push(cpuTime);
			m_gpu.push(gpuTime);
		}

		void pull(float frameTime)
		{
			m_cpuTime = m_cpu.pull(frameTime);
			m_gpuTime = m_gpu.pull(frameTime);
		}
	};
}

struct AppGraphProfiler
{
	AppGraphCtx* m_context;

	int m_state = 0;
	bool m_enabled = false;

	TimerCPU m_frameTimer;
	float m_frameTime = 0.f;

	ID3D11Query* m_disjoint = nullptr;

	static const int m_timersCap = 64;
	Timer m_timers[m_timersCap];
	int m_timersSize = 0;

	TimerValue m_timerValues[m_timersCap];
	int m_timerValuesSize = 0;

	AppGraphProfiler(AppGraphCtx* context);
	~AppGraphProfiler();
};

AppGraphProfiler::AppGraphProfiler(AppGraphCtx* context) : m_context(context) 
{
}

AppGraphProfiler::~AppGraphProfiler()
{
	COMRelease(m_disjoint);
}

AppGraphProfiler* appGraphCreateProfiler(AppGraphCtx* ctx)
{
	return new AppGraphProfiler(ctx);
}

void appGraphReleaseProfiler(AppGraphProfiler* profiler)
{
	delete profiler;
}

void appGraphProfilerFrameBegin(AppGraphProfiler* p)
{
	p->m_frameTime = (float)p->m_frameTimer.getDeltaTime();

	if (p->m_state == 0 && p->m_enabled)
	{
		auto device = p->m_context->m_device;
		auto deviceContext = p->m_context->m_deviceContext;

		if (p->m_disjoint == nullptr)
		{
			D3D11_QUERY_DESC queryDesc;
			queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			queryDesc.MiscFlags = 0u;
			device->CreateQuery(&queryDesc, &p->m_disjoint);
		}

		deviceContext->Begin(p->m_disjoint);

		p->m_timersSize = 0;

		p->m_state = 1;
	}
}

void appGraphProfilerFrameEnd(AppGraphProfiler* p)
{
	if (p->m_state == 1)
	{
		auto deviceContext = p->m_context->m_deviceContext;

		deviceContext->End(p->m_disjoint);

		p->m_state = 2;
	}
}

void appGraphProfilerEnable(AppGraphProfiler* p, bool enabled)
{
	p->m_enabled = enabled;
}

void appGraphProfilerBegin(AppGraphProfiler* p, const char* label)
{
	if (p->m_state == 1 && p->m_timersSize < p->m_timersCap)
	{
		auto& timer = p->m_timers[p->m_timersSize++];
		timer.m_label = label;
		timer.m_cpu.getDeltaTime();

		auto device = p->m_context->m_device;
		auto deviceContext = p->m_context->m_deviceContext;

		if (timer.m_gpu.m_begin == nullptr)
		{
			D3D11_QUERY_DESC queryDesc;
			queryDesc.MiscFlags = 0u;
			queryDesc.Query = D3D11_QUERY_TIMESTAMP;
			device->CreateQuery(&queryDesc, &timer.m_gpu.m_begin);
			device->CreateQuery(&queryDesc, &timer.m_gpu.m_end);
		}

		deviceContext->End(timer.m_gpu.m_begin);
	}
}

void appGraphProfilerEnd(AppGraphProfiler* p, const char* label)
{
	if (p->m_state == 1)
	{
		Timer* timer = nullptr;
		for (int i = 0; i < p->m_timersSize; i++)
		{
			if (strcmp(p->m_timers[i].m_label, label) == 0)
			{
				timer = &p->m_timers[i];
				break;
			}
		}
		if (timer)
		{
			auto deviceContext = p->m_context->m_deviceContext;

			deviceContext->End(timer->m_gpu.m_end);

			timer->m_cpuTime = (float)timer->m_cpu.getDeltaTime();
		}
	}
}

bool appGraphProfilerFlush(AppGraphProfiler* p)
{
	if (p->m_state == 2)
	{
		auto deviceContext = p->m_context->m_deviceContext;

		// check disjoint for completion
		if (deviceContext->GetData(p->m_disjoint, nullptr, 0u, 0u) != S_OK)
		{
			return false;
		}

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
		deviceContext->GetData(p->m_disjoint, &tsDisjoint, sizeof(tsDisjoint), 0u);
		if (tsDisjoint.Disjoint)
		{
			return false;
		}

		for (int i = 0; i < p->m_timersSize; i++)
		{
			Timer& timer = p->m_timers[i];

			UINT64 tsBegin, tsEnd;
			if (deviceContext->GetData(timer.m_gpu.m_begin, &tsBegin, sizeof(UINT64), 0) != S_OK)
			{
				return false;
			}
			if (deviceContext->GetData(timer.m_gpu.m_end, &tsEnd, sizeof(UINT64), 0) != S_OK)
			{
				return false;
			}

			timer.m_gpuTime = float(tsEnd - tsBegin) / float(tsDisjoint.Frequency);

			// update TimerValue
			int j = 0;
			for (; j < p->m_timerValuesSize; j++)
			{
				TimerValue& value = p->m_timerValues[j];
				if (strcmp(value.m_label, timer.m_label) == 0)
				{
					value.push(timer.m_cpuTime, timer.m_gpuTime);
					break;
				}
			}
			if (j >= p->m_timerValuesSize && p->m_timerValuesSize < p->m_timersCap)
			{
				TimerValue& value = p->m_timerValues[p->m_timerValuesSize++];
				value.m_label = timer.m_label;
				value.push(timer.m_cpuTime, timer.m_gpuTime);
			}
		}

		p->m_state = 0;
	}
	return false;
}

bool appGraphProfilerGet(AppGraphProfiler* p, const char** plabel, float* cpuTime, float* gpuTime, int index)
{
	appGraphProfilerFlush(p);
	{
		if (index < p->m_timerValuesSize)
		{
			TimerValue& timer = p->m_timerValues[index];

			timer.pull(p->m_frameTime);

			if (plabel) *plabel = timer.m_label;
			if (cpuTime) *cpuTime = timer.m_cpuTime;
			if (gpuTime) *gpuTime = timer.m_gpuTime;

			return true;
		}
	}
	return false;
}

size_t AppGraphCtxDedicatedVideoMemory(AppGraphCtx* context)
{
	return context->m_dedicatedVideoMemory;
}
