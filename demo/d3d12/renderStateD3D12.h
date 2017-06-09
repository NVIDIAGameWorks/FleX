/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef RENDER_STATE_D3D12_H
#define RENDER_STATE_D3D12_H

#include <NvCoDx12ResourceScopeManager.h>
#include <NvCoDx12CircularResourceHeap.h>
#include <NvCoDx12DescriptorHeap.h>
#include "appD3D12Ctx.h"

namespace NvCo = nvidia::Common;

namespace FlexSample {
using namespace nvidia;

struct PipelineStateD3D12
{
		/// True if contents is all set and therefore 'valid'
	inline bool isValid() const { return m_rootSignature && m_pipelineState; }

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
};

struct RenderStateD3D12
{
	AppGraphCtxD3D12* m_renderContext;

	ID3D12Device* m_device;
	ID3D12GraphicsCommandList* m_commandList;

	NvCo::Dx12ResourceScopeManager* m_scopeManager;				///< Holds resources in scope
	NvCo::Dx12DescriptorHeap* m_srvCbvUavDescriptorHeap;			///< Can hold cbv, srv, uavs 
	NvCo::Dx12DescriptorHeap* m_samplerDescriptorHeap;			///< Descriptor heap
	NvCo::Dx12CircularResourceHeap* m_constantHeap;				///< Can hold transitory constant buffers, and vertex buffers
	NvCo::Dx12CounterFence* m_fence;								///< Main fence to track lifetimes
};

struct RenderStateManagerD3D12
{
	RenderStateManagerD3D12():
		m_renderContext(nullptr),
		m_device(nullptr)
	{}

	int initialize(AppGraphCtxD3D12* renderContext, size_t maxHeapAlloc);

	void onGpuWorkSubmitted(ID3D12CommandQueue* handle);
	void updateCompleted();

		/// Get the render state
	RenderStateD3D12 getState();

	AppGraphCtxD3D12* m_renderContext;
	ID3D12Device* m_device;

	NvCo::Dx12ResourceScopeManager m_scopeManager;			///< Holds resources in scope
	NvCo::Dx12DescriptorHeap m_srvCbvUavDescriptorHeap;		///< Can hold cbv, srv, uavs 
	NvCo::Dx12DescriptorHeap m_samplerDescriptorHeap;		///< Holds sampler descriptions
	NvCo::Dx12CircularResourceHeap m_constantHeap;			///< Can hold transitory constant buffers, and vertex buffers
	NvCo::Dx12CounterFence m_fence;							///< Main fence to track lifetimes
};

} // namespace FlexSample

#endif