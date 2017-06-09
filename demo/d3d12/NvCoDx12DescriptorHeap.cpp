/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#include "NvCoDx12DescriptorHeap.h"

namespace nvidia { 
namespace Common {

Dx12DescriptorHeap::Dx12DescriptorHeap():
	m_size(0),
	m_currentIndex(0),
	m_descriptorSize(0)
{
}

int Dx12DescriptorHeap::init(ID3D12Device* device, int size, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = size;
	srvHeapDesc.Flags = flags;
	srvHeapDesc.Type = type;
	NV_RETURN_ON_FAIL(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_heap)));

	m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
	m_size = size;

	return NV_OK;
}

int Dx12DescriptorHeap::init(ID3D12Device* device, const D3D12_CPU_DESCRIPTOR_HANDLE* handles, int numHandles, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	NV_RETURN_ON_FAIL(init(device, numHandles, type, flags));
	D3D12_CPU_DESCRIPTOR_HANDLE dst = m_heap->GetCPUDescriptorHandleForHeapStart();

	// Copy them all 
	for (int i = 0; i < numHandles; i++, dst.ptr += m_descriptorSize)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE src = handles[i];
		if (src.ptr != 0)
		{
			device->CopyDescriptorsSimple(1, dst, src, type);
		}
	}

	return NV_OK;
}

} // namespace Common
} // namespace nvidia

