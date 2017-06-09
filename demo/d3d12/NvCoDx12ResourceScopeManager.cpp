/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#include "NvCoDx12ResourceScopeManager.h"

#include <assert.h>

namespace nvidia {
namespace Common {

Dx12ResourceScopeManager::Dx12ResourceScopeManager():
	m_fence(nullptr),
	m_device(nullptr)
{
}

Dx12ResourceScopeManager::~Dx12ResourceScopeManager()
{
	while (!m_entryQueue.empty())
	{
		Entry& entry = m_entryQueue.front();
		entry.m_resource->Release();
		m_entryQueue.pop_front();
	}
}

int Dx12ResourceScopeManager::init(ID3D12Device* device, Dx12CounterFence* fence)
{
	m_fence = fence;
	m_device = device;
	return NV_OK;
}

void Dx12ResourceScopeManager::addSync(uint64_t signalValue)
{	
	(void)signalValue;

	assert(m_fence->getCurrentValue() == signalValue);
}

void Dx12ResourceScopeManager::updateCompleted()
{
	const uint64_t completedValue = m_fence->getCompletedValue();
	if (!m_entryQueue.empty())
	{
		const Entry& entry = m_entryQueue.front();
		if (entry.m_completedValue >= completedValue)
		{
			return;
		}
		entry.m_resource->Release();
		m_entryQueue.pop_front();
	}
}

ID3D12Resource* Dx12ResourceScopeManager::newUploadResource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
{
	D3D12_HEAP_PROPERTIES heapProps;
	{
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;
	}
	const D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

	const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

	ComPtr<ID3D12Resource> resource;
	int res = m_device->CreateCommittedResource(&heapProps, heapFlags, &resourceDesc, initialState, clearValue, IID_PPV_ARGS(&resource));
	if (NV_FAILED(res)) return nullptr;

	// Get the current fence count
	const uint64_t completedValue = m_fence->getCurrentValue();

	Entry entry;
	entry.m_completedValue = completedValue;
	entry.m_resource = resource.Detach();
	m_entryQueue.push_back(entry);

	return entry.m_resource;
}

/* static */void Dx12ResourceScopeManager::copy(const D3D12_SUBRESOURCE_DATA& src, size_t rowSizeInBytes, int numRows, int numSlices, const D3D12_MEMCPY_DEST& dst)
{
	for (int i = 0; i < numSlices; ++i)
	{
		uint8_t* dstSlice = reinterpret_cast<uint8_t*>(dst.pData) + dst.SlicePitch * i;
		const uint8_t* srcSlice = reinterpret_cast<const uint8_t*>(src.pData) + src.SlicePitch * i;
		for (int j = 0; j < numRows; ++j)
		{
			memcpy(dstSlice + dst.RowPitch * j, srcSlice + src.RowPitch * j, rowSizeInBytes);
		}
	}
}

int Dx12ResourceScopeManager::upload(ID3D12GraphicsCommandList* commandList, const void* srcDataIn, ID3D12Resource* targetResource, D3D12_RESOURCE_STATES targetState)
{
	// Get the targetDesc
	const D3D12_RESOURCE_DESC targetDesc = targetResource->GetDesc();
	// Ensure it is just a regular buffer
	assert(targetDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
	assert(targetDesc.Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR);

	// The buffer size is the width 
	const size_t bufferSize = size_t(targetDesc.Width);

	D3D12_RESOURCE_DESC uploadDesc = targetDesc;
	uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// Create the upload resource
	ID3D12Resource* uploadResource = newUploadResource(uploadDesc);

	// Map it and copy 
	{
		uint8_t* uploadMapped;
		NV_RETURN_ON_FAIL(uploadResource->Map(0, nullptr, (void**)&uploadMapped));
		memcpy(uploadMapped, srcDataIn, bufferSize);
		uploadResource->Unmap(0, nullptr);
	}

	// Add the copy
	commandList->CopyBufferRegion(targetResource, 0, uploadResource, 0, bufferSize);

	// Add the barrier
	{
		D3D12_RESOURCE_BARRIER barrier = {};

		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		D3D12_RESOURCE_TRANSITION_BARRIER& transition = barrier.Transition;
		transition.pResource = targetResource;
		transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		transition.StateAfter = targetState;
		transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		commandList->ResourceBarrier(1, &barrier);
	}

	return NV_OK;
}

void Dx12ResourceScopeManager::add(ID3D12Resource* resource)
{
	assert(resource);

	// Get the current fence count
	const uint64_t completedValue = m_fence->getCurrentValue();

	Entry entry;
	entry.m_completedValue = completedValue;
	resource->AddRef();
	entry.m_resource = resource;

	m_entryQueue.push_back(entry);
}

int Dx12ResourceScopeManager::uploadWithState(ID3D12GraphicsCommandList* commandList, const void* srcDataIn, Dx12Resource& target, D3D12_RESOURCE_STATES targetState)
{
	// make sure we are in the correct initial state
	if (target.getState() != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		Dx12BarrierSubmitter submitter(commandList);
		target.transition(D3D12_RESOURCE_STATE_COPY_DEST, submitter);
	}
	// Do the upload
	NV_RETURN_ON_FAIL(upload(commandList, srcDataIn, target.getResource(), targetState));
	target.setState(targetState);
	return NV_OK;
}


} // namespace Common
} // namespace nvidia
