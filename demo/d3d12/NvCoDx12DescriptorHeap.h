/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_DX12_DESCRIPTOR_HEAP_H
#define NV_CO_DX12_DESCRIPTOR_HEAP_H

#include <NvResult.h>

#define NOMINMAX
#include <dxgi.h>
#include <d3d12.h>
#include <assert.h>
#include <wrl.h>

using namespace Microsoft::WRL;

namespace nvidia {
namespace Common {

/*! \brief A simple class to manage an underlying Dx12 Descriptor Heap. Allocations are made linearly in order. It is not possible to free
individual allocations, but all allocations can be deallocated with 'deallocateAll'. */
class Dx12DescriptorHeap
{
	//NV_CO_DECLARE_CLASS_BASE(Dx12DescriptorHeap);
public:
		/// Initialize
	int init(ID3D12Device* device, int size, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		/// Initialize with an array of handles copying over the representation
	int init(ID3D12Device* device, const D3D12_CPU_DESCRIPTOR_HANDLE* handles, int numHandles, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

		/// Get the total amount of descriptors possible on the heap
	inline int getSize() const { return m_size; }
		/// Allocate a descriptor. Returns the index, or -1 if none left.
	inline int allocate();
		/// Allocate a number of descriptors. Returns the start index (or -1 if not possible)
	inline int allocate(int numDescriptors);

		/// Deallocates all allocations, and starts allocation from the start of the underlying heap again
	inline void deallocateAll() { m_currentIndex = 0; }

		/// Get the size of each 
	inline int getDescriptorSize() const { return m_descriptorSize; }

		/// Get the GPU heap start
	inline D3D12_GPU_DESCRIPTOR_HANDLE getGpuStart() const { return m_heap->GetGPUDescriptorHandleForHeapStart(); }
		/// Get the CPU heap start
	inline D3D12_CPU_DESCRIPTOR_HANDLE getCpuStart() const { return m_heap->GetCPUDescriptorHandleForHeapStart(); }

		/// Get the GPU handle at the specified index
	inline D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(int index) const;
		/// Get the CPU handle at the specified index
	inline D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(int index) const;

		/// Get the underlying heap
	inline ID3D12DescriptorHeap* getHeap() const { return m_heap.Get(); }

		/// Ctor
	Dx12DescriptorHeap();

protected:
	ComPtr<ID3D12DescriptorHeap> m_heap;	///< The underlying heap being allocated from
	int m_size;								///< Total amount of allocations available on the heap
	int m_currentIndex;						///< The current descriptor
	int m_descriptorSize;					///< The size of each descriptor
};

// ---------------------------------------------------------------------------
int Dx12DescriptorHeap::allocate()
{
	assert(m_currentIndex < m_size);
	if (m_currentIndex < m_size)
	{
		return m_currentIndex++;	
	}
	return -1;
}
// ---------------------------------------------------------------------------
int Dx12DescriptorHeap::allocate(int numDescriptors)
{
	assert(m_currentIndex + numDescriptors <= m_size);
	if (m_currentIndex + numDescriptors <= m_size)
	{
		const int index = m_currentIndex;
		m_currentIndex += numDescriptors;
		return index;
	}
	return -1;
}
// ---------------------------------------------------------------------------
inline D3D12_CPU_DESCRIPTOR_HANDLE Dx12DescriptorHeap::getCpuHandle(int index) const
{ 
	assert(index >= 0 && index < m_size);  
	D3D12_CPU_DESCRIPTOR_HANDLE start = m_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE dst;
	dst.ptr = start.ptr + m_descriptorSize * index;
	return dst;
}	
// ---------------------------------------------------------------------------
inline D3D12_GPU_DESCRIPTOR_HANDLE Dx12DescriptorHeap::getGpuHandle(int index) const
{ 
	assert(index >= 0 && index < m_size);  
	D3D12_GPU_DESCRIPTOR_HANDLE start = m_heap->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE dst;
	dst.ptr = start.ptr + m_descriptorSize * index;
	return dst;
}

} // namespace Common
} // namespace nvidia


#endif // NV_CO_DX12_DESCRIPTOR_HEAP_H
