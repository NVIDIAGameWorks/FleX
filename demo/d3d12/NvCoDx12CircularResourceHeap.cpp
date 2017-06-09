/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#include "NvCoDx12CircularResourceHeap.h"

namespace nvidia {
namespace Common {

Dx12CircularResourceHeap::Dx12CircularResourceHeap():
	m_fence(nullptr),
	m_device(nullptr),
	m_blockFreeList(sizeof(Block), __alignof(Block), 16),
	m_blocks(nullptr)
{
	m_back.m_block = nullptr;
	m_back.m_position = nullptr;
	m_front.m_block = nullptr;
	m_front.m_position = nullptr;
}

Dx12CircularResourceHeap::~Dx12CircularResourceHeap()
{
	_freeBlockListResources(m_blocks);
}

void Dx12CircularResourceHeap::_freeBlockListResources(const Block* start)
{
	if (start)
	{
		{
			ID3D12Resource* resource = start->m_resource;
			resource->Unmap(0, nullptr);
			resource->Release();
		}
		for (Block* block = start->m_next; block != start; block = block->m_next)
		{
			ID3D12Resource* resource = block->m_resource;
			resource->Unmap(0, nullptr);
			resource->Release();
		}
	} 
}

int Dx12CircularResourceHeap::init(ID3D12Device* device, const Desc& desc, Dx12CounterFence* fence)
{
	assert(m_blocks == nullptr); 
	assert(desc.m_blockSize > 0);

	m_fence = fence;
	m_desc = desc;
	m_device = device;

	return NV_OK;
}

void Dx12CircularResourceHeap::addSync(uint64_t signalValue)
{
	assert(signalValue == m_fence->getCurrentValue());
	PendingEntry entry;
	entry.m_completedValue = signalValue;
	entry.m_cursor = m_front;
	m_pendingQueue.push_back(entry); 
}

void Dx12CircularResourceHeap::updateCompleted()
{
	const uint64_t completedValue = m_fence->getCompletedValue();
	while (!m_pendingQueue.empty())
	{
		const PendingEntry& entry = m_pendingQueue.front();
		if (entry.m_completedValue <= completedValue)
		{
			m_back = entry.m_cursor;
			m_pendingQueue.pop_front();
		}
		else
		{
			break;
		}
	}
}

Dx12CircularResourceHeap::Block* Dx12CircularResourceHeap::_newBlock()
{
	D3D12_RESOURCE_DESC desc;

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = m_desc.m_blockSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ComPtr<ID3D12Resource> resource;
	int res = m_device->CreateCommittedResource(&m_desc.m_heapProperties, m_desc.m_heapFlags, &desc, m_desc.m_initialState, nullptr, IID_PPV_ARGS(&resource));
	if (NV_FAILED(res))
	{
		printf("Resource allocation failed");
		return nullptr;
	}

	uint8_t* data = nullptr;
	if (m_desc.m_heapProperties.Type == D3D12_HEAP_TYPE_READBACK)
	{
	}
	else
	{
		// Map it, and keep it mapped
		resource->Map(0, nullptr, (void**)&data);
	}

	// We have no blocks -> so lets allocate the first
	Block* block = (Block*)m_blockFreeList.allocate();
	block->m_next = nullptr;

	block->m_resource = resource.Detach();
	block->m_start = data;
	return block;
}

Dx12CircularResourceHeap::Cursor Dx12CircularResourceHeap::allocate(size_t size, size_t alignment)
{
	const size_t blockSize = getBlockSize();

	assert(size <= blockSize);

	// If nothing is allocated add the first block
	if (m_blocks == nullptr)
	{
		Block* block = _newBlock();
		if (!block)
		{
			Cursor cursor = {};
			return cursor;
		}
		m_blocks = block;
		// Make circular
		block->m_next = block;

		// Point front and back to same position, as currently it is all free
		m_back = { block, block->m_start };
		m_front = m_back;
	}

	// If front and back are in the same block then front MUST be ahead of back (as that defined as 
	// an invariant and is required for block insertion to be possible
	Block* block = m_front.m_block;

	// Check the invariant
	assert(block != m_back.m_block || m_front.m_position >= m_back.m_position);

	{
		uint8_t* cur = (uint8_t*)((size_t(m_front.m_position) + alignment - 1) & ~(alignment - 1));
		// Does the the allocation fit?
		if (cur + size <= block->m_start + blockSize)
		{
			// It fits
			// Move the front forward
			m_front.m_position = cur + size;
			Cursor cursor = { block, cur };
			return cursor;
		}
	}

	// Okay I can't fit into current block...
	 
	// If the next block contains front, we need to add a block, else we can use that block
	if (block->m_next == m_back.m_block)
	{
		Block* newBlock = _newBlock();
		// Insert into the list
		newBlock->m_next = block->m_next;
		block->m_next = newBlock;
	}
	
	// Use the block we are going to add to
	block = block->m_next;
	uint8_t* cur = (uint8_t*)((size_t(block->m_start) + alignment - 1) & ~(alignment - 1));
	// Does the the allocation fit?
	if (cur + size > block->m_start + blockSize)
	{
		printf("Couldn't fit into a free block(!) Alignment breaks it?");
		Cursor cursor = {};
		return cursor;
	}
	// It fits
	// Move the front forward
	m_front.m_block = block;
	m_front.m_position = cur + size;
	Cursor cursor = { block, cur };
	return cursor;
}

} // namespace Common
} // namespace nvidia
