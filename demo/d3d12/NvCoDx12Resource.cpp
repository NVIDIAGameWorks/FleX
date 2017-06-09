/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#include "NvCoDx12Resource.h"

#include <assert.h>

#define NOMINMAX
#include <wrl.h>
using namespace Microsoft::WRL;

namespace nvidia {
namespace Common {

/* !!!!!!!!!!!!!!!!!!!!!!!!! Dx12BarrierSubmitter !!!!!!!!!!!!!!!!!!!!!!!! */

void Dx12BarrierSubmitter::_flush()
{
	assert(m_numBarriers > 0);

	if (m_commandList)
	{
		m_commandList->ResourceBarrier(UINT(m_numBarriers), m_barriers);
	}
	m_numBarriers = 0;
}

D3D12_RESOURCE_BARRIER& Dx12BarrierSubmitter::_expandOne()
{
	_flush();
	return m_barriers[m_numBarriers++];
}

/* !!!!!!!!!!!!!!!!!!!!!!!!! Dx12ResourceBase !!!!!!!!!!!!!!!!!!!!!!!! */

void Dx12ResourceBase::transition(D3D12_RESOURCE_STATES nextState, Dx12BarrierSubmitter& submitter)
{
	// If there is no resource, then there is nothing to transition
	if (!m_resource)
	{
		return;
	}
	
	if (nextState != m_state)
	{
		D3D12_RESOURCE_BARRIER& barrier = submitter.expandOne();

		const UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		const D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		memset(&barrier, 0, sizeof(barrier)); 
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = flags;
		barrier.Transition.pResource = m_resource;
		barrier.Transition.StateBefore = m_state;
		barrier.Transition.StateAfter = nextState;
		barrier.Transition.Subresource = subresource;
	
		m_prevState = m_state;
		m_state = nextState;
	}
	else
	{
		if (nextState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			D3D12_RESOURCE_BARRIER& barrier = submitter.expandOne();

			memset(&barrier, 0, sizeof(barrier));
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.UAV.pResource = m_resource;

			m_state = nextState;
		}
	}
}

/* static */DXGI_FORMAT Dx12ResourceBase::calcFormat(DxFormatUtil::UsageType usage, ID3D12Resource* resource)
{
	return resource ? DxFormatUtil::calcFormat(usage, resource->GetDesc().Format) : DXGI_FORMAT_UNKNOWN;
}

/* !!!!!!!!!!!!!!!!!!!!!!!!! Dx12Resource !!!!!!!!!!!!!!!!!!!!!!!! */

/* static */void Dx12Resource::setDebugName(ID3D12Resource* resource, const std::wstring& name)
{
	if (resource)
	{
		resource->SetName(name.c_str());
	}
}

void Dx12Resource::setDebugName(const std::wstring& name)
{
	setDebugName(m_resource, name);
}

void Dx12Resource::setDebugName(const wchar_t* name) 
{ 
	if (m_resource) 
	{
		m_resource->SetName(name); 
	}
}

void Dx12Resource::setResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState)
{
	if (resource != m_resource)
	{
		if (resource)
		{
			resource->AddRef();
		}
		if (m_resource)
		{
			m_resource->Release();
		}
		m_resource = resource;
	}
	m_prevState = initialState;
	m_state = initialState;
}

void Dx12Resource::setResourceNull()
{
	if (m_resource)
	{
		m_resource->Release();
		m_resource = nullptr;
	}
}

int Dx12Resource::initCommitted(ID3D12Device* device, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initState, const D3D12_CLEAR_VALUE * clearValue)
{
	setResourceNull();
	ComPtr<ID3D12Resource> resource;
	NV_RETURN_ON_FAIL(device->CreateCommittedResource(&heapProps, heapFlags, &resourceDesc, initState, clearValue, IID_PPV_ARGS(&resource)));
	setResource(resource.Get(), initState);
	return NV_OK;
}

ID3D12Resource* Dx12Resource::detach()
{
	ID3D12Resource* resource = m_resource;
	m_resource = nullptr;
	return resource;
}

void Dx12Resource::swap(ComPtr<ID3D12Resource>& resourceInOut)
{
	ID3D12Resource* tmp = m_resource;
	m_resource = resourceInOut.Detach();
	resourceInOut.Attach(tmp);
}

void Dx12Resource::setState(D3D12_RESOURCE_STATES state)
{
	m_prevState = state;
	m_state = state;
}


} // namespace Common 
} // namespace nvidia 
