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
#include <dxgi1_4.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "DXGI.lib")

#include "appD3D12Ctx.h"

#include <stdio.h>

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_syswm.h>

namespace
{
	// COM object release utilities
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
}

AppGraphProfilerD3D12* appGraphCreateProfilerD3D12(AppGraphCtx* ctx);
void appGraphProfilerD3D12FrameBegin(AppGraphProfilerD3D12* profiler);
void appGraphProfilerD3D12FrameEnd(AppGraphProfilerD3D12* profiler);
void appGraphProfilerD3D12Enable(AppGraphProfilerD3D12* profiler, bool enabled);
void appGraphProfilerD3D12Begin(AppGraphProfilerD3D12* profiler, const char* label);
void appGraphProfilerD3D12End(AppGraphProfilerD3D12* profiler, const char* label);
bool appGraphProfilerD3D12Get(AppGraphProfilerD3D12* profiler, const char** plabel, float* cpuTime, float* gpuTime, int index);
void appGraphReleaseProfiler(AppGraphProfilerD3D12* profiler);

AppGraphCtxD3D12::AppGraphCtxD3D12()
{
	m_profiler = appGraphCreateProfilerD3D12(cast_from_AppGraphCtxD3D12(this));

	m_targetInfo.init();

	memset(m_commandAllocators, 0, sizeof(m_commandAllocators));
	memset(m_fenceValues, 0, sizeof(m_fenceValues));
}

AppGraphCtxD3D12::~AppGraphCtxD3D12()
{
	AppGraphCtxReleaseRenderTargetD3D12(cast_from_AppGraphCtxD3D12(this));

	COMRelease(m_device);
	COMRelease(m_commandQueue);
	COMRelease(m_rtvHeap);
	COMRelease(m_dsvHeap);
	COMRelease(m_depthSrvHeap);
	COMRelease(m_commandAllocators, m_frameCount);

	COMRelease(m_fence);
	CloseHandle(m_fenceEvent);

	COMRelease(m_commandList);

	m_dynamicHeapCbvSrvUav.release();

	appGraphReleaseProfiler(m_profiler);
	m_profiler = nullptr;
}

AppGraphCtx* AppGraphCtxCreateD3D12(int deviceID)
{
	AppGraphCtxD3D12* context = new AppGraphCtxD3D12;

	HRESULT hr = S_OK;

#if defined(_DEBUG)
#if !ENABLE_AFTERMATH_SUPPORT	// we cannot use debug layer together with aftermath
	// Enable the D3D12 debug layer.
	{
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
		COMRelease(debugController);
	}
#endif
#endif

	UINT debugFlags = 0;
#ifdef _DEBUG
	debugFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	// enumerate devices
	IDXGIFactory4* pFactory = NULL;
	CreateDXGIFactory2(debugFlags, IID_PPV_ARGS(&pFactory));
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

	// create device
	if (hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&context->m_device))
	{
		delete context;
		return nullptr;
	}

	// to disable annoying warning
#if 0
	context->m_device->SetStablePowerState(TRUE);
#endif

	// create command queue
	{
		D3D12_COMMAND_QUEUE_DESC desc;
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		if (hr = context->m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&context->m_commandQueue)))
		{
			delete context;
			return nullptr;
		}
	}

	// cleanup adapter and factory
	COMRelease(pAdapter);
	COMRelease(pFactory);

	// create RTV descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = context->m_renderTargetCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (hr = context->m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&context->m_rtvHeap)))
		{
			delete context;
			return nullptr;
		}
		context->m_rtvDescriptorSize = context->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// create DSV descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (hr = context->m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&context->m_dsvHeap)))
		{
			delete context;
			return nullptr;
		}
	}

	// create depth SRV descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = 1;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (hr = context->m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&context->m_depthSrvHeap)))
		{
			delete context;
			return nullptr;
		}
	}

	// Create per frame resources
	{
		for (UINT idx = 0; idx < context->m_frameCount; idx++)
		{
			if (hr = context->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&context->m_commandAllocators[idx])))
			{
				delete context;
				return nullptr;
			}
		}
	}

	// create dynamic heap
	{
		context->m_dynamicHeapCbvSrvUav.init(context->m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256u * 1024u);
	}

	// Create command list and close it
	{
		if (hr = context->m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
			context->m_commandAllocators[context->m_frameIndex], nullptr, IID_PPV_ARGS(&context->m_commandList))
			)
		{
			delete context;
			return nullptr;
		}
		context->m_commandList->Close();
	}

	// create synchronization objects
	{
		if (hr = context->m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&context->m_fence)))
		{
			delete context;
			return nullptr;
		}

		context->m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (context->m_fenceEvent == nullptr)
		{
			delete context;
			return nullptr;
		}
	}

	return cast_from_AppGraphCtxD3D12(context);
}


void AppGraphCtxInitRenderTargetD3D12(AppGraphCtx* contextIn, SDL_Window* window, bool fullscreen, int numMSAASamples)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);

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

	UINT debugFlags = 0;
#ifdef _DEBUG
	debugFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	// enumerate devices
	IDXGIFactory4* pFactory = NULL;
	CreateDXGIFactory2(debugFlags, IID_PPV_ARGS(&pFactory));

	// create the swap chain
	for (int i = 0; i < 2; i++)
	{
		DXGI_SWAP_CHAIN_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.BufferCount = context->m_renderTargetCount;
		desc.BufferDesc.Width = context->m_winW;
		desc.BufferDesc.Height = context->m_winH;
		desc.BufferDesc.Format = context->m_rtv_format; // DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 0;
		desc.BufferDesc.RefreshRate.Denominator = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.OutputWindow = context->m_hWnd;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = context->m_fullscreen ? FALSE : TRUE;
		desc.Flags = context->m_fullscreen ? 0u : DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		context->m_current_rtvDesc.Format = context->m_rtv_format;
		context->m_current_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		context->m_current_rtvDesc.Texture2D.MipSlice = 0u;
		context->m_current_rtvDesc.Texture2D.PlaneSlice = 0u;

		hr = pFactory->CreateSwapChain(context->m_commandQueue, &desc, (IDXGISwapChain**)&context->m_swapChain);

		if (hr != S_OK)
		{
			COMRelease(context->m_swapChain);
			context->m_fullscreen = false;
			continue;
		}

		if (!context->m_fullscreen)
		{
			context->m_swapChainWaitableObject = context->m_swapChain->GetFrameLatencyWaitableObject();
			context->m_swapChain->SetMaximumFrameLatency(context->m_renderTargetCount - 2);
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

			context->m_winW = desc.BufferDesc.Width;
			context->m_winH = desc.BufferDesc.Height;
		}

		context->m_frameIndex = context->m_swapChain->GetCurrentBackBufferIndex();
		break;
	}

	// configure scissor and viewport
	{
		context->m_viewport.Width = float(context->m_winW);
		context->m_viewport.Height = float(context->m_winH);
		context->m_viewport.MaxDepth = 1.f;

		context->m_scissorRect.right = context->m_winW;
		context->m_scissorRect.bottom = context->m_winH;
	}

	COMRelease(pFactory);

	// create per render target resources
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = context->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

		for (UINT idx = 0; idx < context->m_renderTargetCount; idx++)
		{
			ComPtr<ID3D12Resource> backBuffer;
			if (hr = context->m_swapChain->GetBuffer(idx, IID_PPV_ARGS(&backBuffer)))
			{
				return;
			}
			context->m_backBuffers[idx].setDebugName(L"Backbuffer");
			context->m_backBuffers[idx].setResource(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
			// Assume they are the same thing for now...
			context->m_renderTargets[idx] = &context->m_backBuffers[idx];

			// If we are multi-sampling - create a render target separate from the back buffer
			if (context->m_numMsaaSamples > 1)
			{
				D3D12_HEAP_PROPERTIES heapProps = {};
				heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
				D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();

				DXGI_FORMAT resourceFormat;

				if (desc.Format == DXGI_FORMAT_R32_FLOAT || desc.Format == DXGI_FORMAT_D32_FLOAT)
				{
					resourceFormat = DXGI_FORMAT_R32_TYPELESS;
				}
				else if (desc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
				{
					resourceFormat = DXGI_FORMAT_R24G8_TYPELESS;
				}
				else
				{
					resourceFormat = desc.Format;
				}

				DXGI_FORMAT targetFormat = nvidia::Common::DxFormatUtil::calcFormat(nvidia::Common::DxFormatUtil::USAGE_TARGET, resourceFormat);

				// Set the target format
				context->m_targetInfo.m_renderTargetFormats[0] = targetFormat;

				D3D12_CLEAR_VALUE clearValue = {}; clearValue.Color[3] = 1.0f;
				clearValue.Format = targetFormat;

				desc.Format = resourceFormat;
				desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				desc.SampleDesc.Count = context->m_targetInfo.m_numSamples;
				desc.SampleDesc.Quality = context->m_targetInfo.m_sampleQuality;
				desc.Alignment = 0;

				context->m_renderTargetResources[idx].initCommitted(context->m_device, heapProps, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);
				context->m_renderTargets[idx] = &context->m_renderTargetResources[idx];

				context->m_renderTargetResources[idx].setDebugName(L"Render Target");
			}

			context->m_device->CreateRenderTargetView(*context->m_renderTargets[idx], nullptr, rtvHandle);
			rtvHandle.ptr += context->m_rtvDescriptorSize;
		}
	}

	// create the depth stencil
	{
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0u;
		heapProps.VisibleNodeMask = 0u;

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.MipLevels = 1u;
		texDesc.Format = context->m_depth_format; // DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_R24G8_TYPELESS
		texDesc.Width = context->m_winW;
		texDesc.Height = context->m_winH;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL /*| D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE*/;
		texDesc.DepthOrArraySize = 1u;
		texDesc.SampleDesc.Count = context->m_targetInfo.m_numSamples;
		texDesc.SampleDesc.Quality = context->m_targetInfo.m_sampleQuality;
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = context->m_dsv_format; // DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.f;
		clearValue.DepthStencil.Stencil = 0;

		if (hr = context->m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&context->m_depthStencil)
		))
		{
			return;
		}

		// create the depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
		viewDesc.Format = context->m_dsv_format; // DXGI_FORMAT_D32_FLOAT;
		viewDesc.ViewDimension = (context->m_numMsaaSamples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Flags = D3D12_DSV_FLAG_NONE;
		viewDesc.Texture2D.MipSlice = 0;

		context->m_current_dsvDesc = viewDesc;

		context->m_device->CreateDepthStencilView(context->m_depthStencil, &viewDesc, context->m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		context->m_targetInfo.m_depthStencilFormat = context->m_dsv_format;
	}
}

bool AppGraphCtxUpdateSizeD3D12(AppGraphCtx* contextIn, SDL_Window* window, bool fullscreen, int numMSAASamples)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);

	// TODO: fix iflip fullscreen support
	fullscreen = false;

	bool sizeChanged = false;
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	// sync with window
	{
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

		if (context->m_winW != width || context->m_winH != height)
		{
			context->m_winW = width;
			context->m_winH = height;
			sizeChanged = true;
			context->m_valid = (context->m_winW != 0 && context->m_winH != 0);
		}
	}

	context->m_numMsaaSamples = numMSAASamples;
	context->m_targetInfo.m_numSamples = numMSAASamples;

	if (sizeChanged)
	{
		const bool wasValid = context->m_valid;
		// Release
		AppGraphCtxReleaseRenderTargetD3D12(cast_from_AppGraphCtxD3D12(context));
		// If was valid recreate it
		if (wasValid)
		{
			// Reset the size (the release sets it, to 0,0)
			context->m_winW = width;
			context->m_winH = height;
			// 
			AppGraphCtxInitRenderTargetD3D12(cast_from_AppGraphCtxD3D12(context), window, fullscreen, numMSAASamples);
		}
	}

	return context->m_valid;
}

void AppGraphCtxReleaseRenderTargetD3D12(AppGraphCtx* contextIn)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);

	if (context->m_swapChain == nullptr)
	{
		return;
	}

	// need to make sure the pipeline is flushed
	for (UINT i = 0; i < context->m_frameCount; i++)
	{
		// check dependencies
		UINT64 fenceCompleted = context->m_fence->GetCompletedValue();
		if (fenceCompleted < context->m_fenceValues[i])
		{
			context->m_fence->SetEventOnCompletion(context->m_fenceValues[i], context->m_fenceEvent);
			WaitForSingleObjectEx(context->m_fenceEvent, INFINITE, FALSE);
		}
	}

	BOOL bFullscreen = FALSE;
	context->m_swapChain->GetFullscreenState(&bFullscreen, nullptr);
	if (bFullscreen == TRUE) context->m_swapChain->SetFullscreenState(FALSE, nullptr);

	for (int i = 0; i != context->m_renderTargetCount; i++)
	{
		context->m_backBuffers[i].setResourceNull();
		if (context->m_numMsaaSamples > 1)
			context->m_renderTargets[i]->setResourceNull();
	}
	COMRelease(context->m_swapChain);
	COMRelease(context->m_depthStencil);

	context->m_valid = false;
	context->m_winW = 0u;
	context->m_winH = 0u;
}

void AppGraphCtxReleaseD3D12(AppGraphCtx* context)
{
	if (context == nullptr) return;

	delete cast_to_AppGraphCtxD3D12(context);
}

void AppGraphCtxFrameStartD3D12(AppGraphCtx* contextIn, AppGraphColor clearColor)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);

	// Get back render target index
	context->m_renderTargetIndex = context->m_swapChain->GetCurrentBackBufferIndex();

	// check dependencies
	UINT64 fenceCompleted = context->m_fence->GetCompletedValue();
	if (fenceCompleted < context->m_fenceValues[context->m_frameIndex])
	{
		context->m_fence->SetEventOnCompletion(context->m_fenceValues[context->m_frameIndex], context->m_fenceEvent);
		WaitForSingleObjectEx(context->m_fenceEvent, INFINITE, FALSE);
	}

	// The fence ID associated with completion of this frame
	context->m_thisFrameFenceID = context->m_frameID + 1;
	context->m_lastFenceComplete = context->m_fence->GetCompletedValue();

	// reset this frame's command allocator
	context->m_commandAllocators[context->m_frameIndex]->Reset();

	// reset command list with this frame's allocator
	context->m_commandList->Reset(context->m_commandAllocators[context->m_frameIndex], nullptr);

	appGraphProfilerD3D12FrameBegin(context->m_profiler);

	context->m_commandList->RSSetViewports(1, &context->m_viewport);
	context->m_commandList->RSSetScissorRects(1, &context->m_scissorRect);

	{
		nvidia::Common::Dx12BarrierSubmitter submitter(context->m_commandList);
		context->m_renderTargets[context->m_renderTargetIndex]->transition(D3D12_RESOURCE_STATE_RENDER_TARGET, submitter);
	}
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = context->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += context->m_renderTargetIndex * context->m_rtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = context->m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	context->m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	context->m_commandList->ClearRenderTargetView(rtvHandle, &clearColor.r, 0, nullptr);
	context->m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	/// to simplify interop implementation
	context->m_current_renderTarget = context->m_renderTargets[context->m_renderTargetIndex]->getResource();
	context->m_current_rtvHandle = rtvHandle;
	context->m_current_dsvHandle = dsvHandle;
	context->m_current_depth_srvHandle = context->m_depthSrvHeap->GetCPUDescriptorHandleForHeapStart();
}

void AppGraphCtxFramePresentD3D12(AppGraphCtx* contextIn, bool fullsync)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);

	// check if now is good time to present
#if 0 // disable frame latency waitable object check because it will cause vsync to fail
	bool shouldPresent = context->m_fullscreen ? true : WaitForSingleObjectEx(context->m_swapChainWaitableObject, 0, TRUE) != WAIT_TIMEOUT;
	if (shouldPresent)
#endif
	{
		context->m_swapChain->Present(fullsync, 0);
		context->m_renderTargetID++;
	}

	appGraphProfilerD3D12FrameEnd(context->m_profiler);

	// signal for this frame id
	context->m_frameID++;
	context->m_fenceValues[context->m_frameIndex] = context->m_frameID;
	context->m_commandQueue->Signal(context->m_fence, context->m_frameID);

	// increment frame index after signal
	context->m_frameIndex = (context->m_frameIndex + 1) % context->m_frameCount;

	if (fullsync)
	{
		// check dependencies
		for (int frameIndex = 0; frameIndex < context->m_frameCount; frameIndex++)
		{
			UINT64 fenceCompleted = context->m_fence->GetCompletedValue();
			if (fenceCompleted < context->m_fenceValues[frameIndex])
			{
				context->m_fence->SetEventOnCompletion(context->m_fenceValues[frameIndex], context->m_fenceEvent);
				WaitForSingleObjectEx(context->m_fenceEvent, INFINITE, FALSE);
			}
		}
	}
}

void AppGraphCtxWaitForFramesD3D12(AppGraphCtx* contextIn, unsigned int maxFramesInFlight)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);

	unsigned int framesActive = maxFramesInFlight;
	while (framesActive >= maxFramesInFlight)
	{
		// reset count each cycle, and get latest fence value
		framesActive = 0u;
		UINT64 fenceCompleted = context->m_fence->GetCompletedValue();

		// determine how many frames are in flight
		for (int frameIndex = 0; frameIndex < context->m_frameCount; frameIndex++)
		{
			if (fenceCompleted < context->m_fenceValues[frameIndex])
			{
				framesActive++;
			}
		}

		if (framesActive >= maxFramesInFlight)
		{
			// find the active frame with the lowest fence ID
			UINT64 minFenceID = 0;
			unsigned int minFrameIdx = 0;
			for (int frameIndex = 0; frameIndex < context->m_frameCount; frameIndex++)
			{
				if (fenceCompleted < context->m_fenceValues[frameIndex])
				{
					if (minFenceID == 0)
					{
						minFenceID = context->m_fenceValues[frameIndex];
						minFrameIdx = frameIndex;
					}
					else if (context->m_fenceValues[frameIndex] < minFenceID)
					{
						minFenceID = context->m_fenceValues[frameIndex];
						minFrameIdx = frameIndex;
					}
				}
			}
			// Wait for min frame
			{
				unsigned int frameIndex = minFrameIdx;
				fenceCompleted = context->m_fence->GetCompletedValue();
				if (fenceCompleted < context->m_fenceValues[frameIndex])
				{
					context->m_fence->SetEventOnCompletion(context->m_fenceValues[frameIndex], context->m_fenceEvent);
					WaitForSingleObjectEx(context->m_fenceEvent, INFINITE, FALSE);
				}
			}
		}
	}
}

void AppGraphCtxProfileEnableD3D12(AppGraphCtx* contextIn, bool enabled)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);
	appGraphProfilerD3D12Enable(context->m_profiler, enabled);
}

void AppGraphCtxProfileBeginD3D12(AppGraphCtx* contextIn, const char* label)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);
	appGraphProfilerD3D12Begin(context->m_profiler, label);
}

void AppGraphCtxProfileEndD3D12(AppGraphCtx* contextIn, const char* label)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);
	appGraphProfilerD3D12End(context->m_profiler, label);
}

bool AppGraphCtxProfileGetD3D12(AppGraphCtx* contextIn, const char** plabel, float* cpuTime, float* gpuTime, int index)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);
	return appGraphProfilerD3D12Get(context->m_profiler, plabel, cpuTime, gpuTime, index);
}

// ******************************* Dynamic descriptor heap ******************************

void AppDynamicDescriptorHeapD3D12::init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT minHeapSize)
{
	m_device = device;
	m_heapSize = minHeapSize;
	m_startSlot = 0u;
	m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(heapType);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = m_heapSize;
	desc.Type = heapType;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
}

void AppDynamicDescriptorHeapD3D12::release()
{
	m_device = nullptr;
	COMRelease(m_heap);
	m_descriptorSize = 0u;
	m_startSlot = 0u;
	m_heapSize = 0u;
}

AppDescriptorReserveHandleD3D12 AppDynamicDescriptorHeapD3D12::reserveDescriptors(UINT numDescriptors, UINT64 lastFenceCompleted, UINT64 nextFenceValue)
{
	UINT endSlot = m_startSlot + numDescriptors;
	if (endSlot >= m_heapSize)
	{
		m_startSlot = 0u;
		endSlot = numDescriptors;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += m_startSlot * m_descriptorSize;
	gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += m_startSlot * m_descriptorSize;

	// advance start slot
	m_startSlot = endSlot;

	AppDescriptorReserveHandleD3D12 handle = {};
	handle.heap = m_heap;
	handle.descriptorSize = m_descriptorSize;
	handle.cpuHandle = cpuHandle;
	handle.gpuHandle = gpuHandle;
	return handle;
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
		ID3D12QueryHeap* m_queryHeap = nullptr;
		ID3D12Resource* m_queryReadback = nullptr;
		UINT64 m_queryFrequency = 0;
		UINT64 m_queryReadbackFenceVal = ~0llu;

		TimerGPU() {}
		~TimerGPU()
		{
			COMRelease(m_queryHeap);
			COMRelease(m_queryReadback);
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

	struct HeapPropsReadback : public D3D12_HEAP_PROPERTIES
	{
		HeapPropsReadback()
		{
			Type = D3D12_HEAP_TYPE_READBACK;
			CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			CreationNodeMask = 0u;
			VisibleNodeMask = 0u;
		}
	};
	struct ResourceDescBuffer : public D3D12_RESOURCE_DESC
	{
		ResourceDescBuffer(UINT64 size)
		{
			Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			Alignment = 0u;
			Width = size;
			Height = 1u;
			DepthOrArraySize = 1u;
			MipLevels = 1;
			Format = DXGI_FORMAT_UNKNOWN;
			SampleDesc.Count = 1u;
			SampleDesc.Quality = 0u;
			Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			Flags = D3D12_RESOURCE_FLAG_NONE;
		}
	};
}

struct AppGraphProfilerD3D12
{
	AppGraphCtxD3D12* m_context;

	int m_state = 0;
	bool m_enabled = false;

	TimerCPU m_frameTimer;
	float m_frameTime = 0.f;

	static const int m_timersCap = 64;
	Timer m_timers[m_timersCap];
	int m_timersSize = 0;

	TimerValue m_timerValues[m_timersCap];
	int m_timerValuesSize = 0;

	AppGraphProfilerD3D12(AppGraphCtx* context);
	~AppGraphProfilerD3D12();
};

AppGraphProfilerD3D12::AppGraphProfilerD3D12(AppGraphCtx* context) : m_context(cast_to_AppGraphCtxD3D12(context))
{
}

AppGraphProfilerD3D12::~AppGraphProfilerD3D12()
{
}

AppGraphProfilerD3D12* appGraphCreateProfilerD3D12(AppGraphCtx* ctx)
{
	return new AppGraphProfilerD3D12(ctx);
}

void appGraphReleaseProfiler(AppGraphProfilerD3D12* profiler)
{
	delete profiler;
}

void appGraphProfilerD3D12FrameBegin(AppGraphProfilerD3D12* p)
{
	p->m_frameTime = (float)p->m_frameTimer.getDeltaTime();

	if (p->m_state == 0 && p->m_enabled)
	{
		p->m_timersSize = 0;

		p->m_state = 1;
	}
}

void appGraphProfilerD3D12FrameEnd(AppGraphProfilerD3D12* p)
{
	if (p->m_state == 1)
	{
		p->m_state = 2;
	}
}

void appGraphProfilerD3D12Enable(AppGraphProfilerD3D12* p, bool enabled)
{
	p->m_enabled = enabled;
}

void appGraphProfilerD3D12Begin(AppGraphProfilerD3D12* p, const char* label)
{
	if (p->m_state == 1 && p->m_timersSize < p->m_timersCap)
	{
		auto& timer = p->m_timers[p->m_timersSize++];
		timer.m_label = label;
		timer.m_cpu.getDeltaTime();

		auto device = p->m_context->m_device;

		if (timer.m_gpu.m_queryHeap == nullptr)
		{
			D3D12_QUERY_HEAP_DESC queryDesc = {};
			queryDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			queryDesc.Count = 2;
			queryDesc.NodeMask = 0;

			device->CreateQueryHeap(&queryDesc, IID_PPV_ARGS(&timer.m_gpu.m_queryHeap));

			HeapPropsReadback readbackProps;
			ResourceDescBuffer resDesc(2 * sizeof(UINT64));
			resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			device->CreateCommittedResource(&readbackProps, D3D12_HEAP_FLAG_NONE,
				&resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr, IID_PPV_ARGS(&timer.m_gpu.m_queryReadback));
		}

		p->m_context->m_commandQueue->GetTimestampFrequency(&timer.m_gpu.m_queryFrequency);

		p->m_context->m_commandList->EndQuery(timer.m_gpu.m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}
}

void appGraphProfilerD3D12End(AppGraphProfilerD3D12* p, const char* label)
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
			p->m_context->m_commandList->EndQuery(timer->m_gpu.m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);

			p->m_context->m_commandList->ResolveQueryData(timer->m_gpu.m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, timer->m_gpu.m_queryReadback, 0u);

			timer->m_gpu.m_queryReadbackFenceVal = p->m_context->m_thisFrameFenceID;

			timer->m_cpuTime = (float)timer->m_cpu.getDeltaTime();
		}
	}
}

bool appGraphProfilerD3D12Flush(AppGraphProfilerD3D12* p)
{
	if (p->m_state == 2)
	{
		for (int i = 0; i < p->m_timersSize; i++)
		{
			Timer& timer = p->m_timers[i];

			if (timer.m_gpu.m_queryReadbackFenceVal > p->m_context->m_lastFenceComplete)
			{
				return false;
			}

			UINT64 tsBegin, tsEnd;
			{
				void* data;
				// Read range is nullptr, meaning full read access
				D3D12_RANGE readRange;
				readRange.Begin = 0u;
				readRange.End = 2 * sizeof(UINT64);
				timer.m_gpu.m_queryReadback->Map(0u, &readRange, &data);
				if (data)
				{
					auto mapped = (UINT64*)data;
					tsBegin = mapped[0];
					tsEnd = mapped[1];

					D3D12_RANGE writeRange{};
					timer.m_gpu.m_queryReadback->Unmap(0u, &writeRange);
				}
			}

			timer.m_gpuTime = float(tsEnd - tsBegin) / float(timer.m_gpu.m_queryFrequency);

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

bool appGraphProfilerD3D12Get(AppGraphProfilerD3D12* p, const char** plabel, float* cpuTime, float* gpuTime, int index)
{
	appGraphProfilerD3D12Flush(p);
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

size_t AppGraphCtxDedicatedVideoMemoryD3D12(AppGraphCtx* contextIn)
{
	auto context = cast_to_AppGraphCtxD3D12(contextIn);
	return context->m_dedicatedVideoMemory;
}

void AppGraphCtxBeginGpuWork(AppGraphCtxD3D12* context)
{
	if (context->m_commandListOpenCount == 0)
	{
		// It's not open so open it
		ID3D12GraphicsCommandList* commandList = context->m_commandList;

		commandList->Reset(context->m_commandAllocators[context->m_frameIndex], nullptr);
	}
	context->m_commandListOpenCount++;
}

void AppGraphCtxEndGpuWork(AppGraphCtxD3D12* context)
{
	assert(context->m_commandListOpenCount);
	ID3D12GraphicsCommandList* commandList = context->m_commandList;

	NV_CORE_ASSERT_VOID_ON_FAIL(commandList->Close());
	{
		// Execute the command list.
		ID3D12CommandList* commandLists[] = { commandList };
		context->m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	}

	AppGraphCtxWaitForGPU(context);

	// Dec the count. If >0 it needs to still be open
	--context->m_commandListOpenCount;

	// Reopen if needs to be open
	if (context->m_commandListOpenCount)
	{
		// Reopen
		context->m_commandList->Reset(context->m_commandAllocators[context->m_frameIndex], nullptr);
	}
}

void AppGraphCtxPrepareRenderTarget(AppGraphCtxD3D12* context)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = context->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += context->m_renderTargetIndex * context->m_rtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = context->m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	context->m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Set necessary state.
	context->m_commandList->RSSetViewports(1, &context->m_viewport);
	context->m_commandList->RSSetScissorRects(1, &context->m_scissorRect);
}

void AppGraphCtxWaitForGPU(AppGraphCtxD3D12* context)
{
	context->m_frameID++;
	context->m_fenceValues[context->m_frameIndex] = context->m_frameID;

	context->m_commandQueue->Signal(context->m_fence, context->m_frameID);

	for (int frameIndex = 0; frameIndex < context->m_frameCount; frameIndex++)
	{
		UINT64 fenceCompleted = context->m_fence->GetCompletedValue();
		if (fenceCompleted < context->m_fenceValues[frameIndex])
		{
			context->m_fence->SetEventOnCompletion(context->m_fenceValues[frameIndex], context->m_fenceEvent);
			WaitForSingleObjectEx(context->m_fenceEvent, INFINITE, FALSE);
		}
	}
}