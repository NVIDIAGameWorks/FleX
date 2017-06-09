/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_DX12_RESOURCE_SCOPE_MANAGER_H
#define NV_CO_DX12_RESOURCE_SCOPE_MANAGER_H

#include "NvCoDx12Resource.h"
#include "NvCoDx12CounterFence.h"

// Dx12 types
#define NOMINMAX
#include <d3d12.h>

#include <deque>

/** \addtogroup common
@{
*/

namespace nvidia {
namespace Common { 

/*! \brief A class to manage the lifetime of resources that are transitory (as used only for a single specific piece of GPU work), 
but arbitrarily sized - such as textures. 

With Dx12 there is no management for the lifetime of resources within the Api directly. It is the duty of 
the application to make sure resources are released when they are no longer being referenced. The Dx12ResourceScopeManager
tries to simplify this management for some common use cases. Firstly when creating a GPU based resource and filling it 
with data, the data must first be copied to an upload buffer, and then to the target gpu based buffer. The upload buffer
is only needed whilst copying takes place. The method 'upload' will create a temporary upload buffer, and initiate the
copy into target. 

The upload resource will ONLY BE RELEASED when the fences value is greater than the current value. Assuming the command list 
is added to the queue, and the fence is added afterwards (which is the appropriate way), the resources will only be released 
after the fence has been hit AND when updateCompleted is called. 

The Dx12ResourceScopeManager is most useful for resource uploads that happen at startup, and might be very large. If you want to 
upload constrained sized (like constant buffer) resources quickly and efficiently at runtime you might be better using Dx12CircularResourceHeap.

The Dx12ResourceScopeManager uses the addSync/updateCompleted idiom, see details in Dx12CircularResourceHeap.

*/
class Dx12ResourceScopeManager
{
	//NV_CO_DECLARE_CLASS_BASE(Dx12ResourceScopeManager);
public:
		/// Initialize
	int init(ID3D12Device* device, Dx12CounterFence* fence);
	
		/// Allocate constant buffer of specified size
	ID3D12Resource* newUploadResource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
		/// Do an upload on the specified command list
	int upload(ID3D12GraphicsCommandList* commandList, const void* srcDataIn, ID3D12Resource* target, D3D12_RESOURCE_STATES targetState);
		/// Upload data to the target, and set state. Will add barriers if target is in the wrong state
	int uploadWithState(ID3D12GraphicsCommandList* commandList, const void* srcDataIn, Dx12Resource& target, D3D12_RESOURCE_STATES targetState);

		/// Add a resource, which will be released when updateCompleted has a greater value
	void add(ID3D12Resource* resource);

		/// Look where the GPU has got to and release anything not currently used
	void updateCompleted();
		/// Add a sync point - meaning that when this point is hit in the queue
		/// all of the resources up to this point will no longer be used.
	void addSync(uint64_t signalValue);

		/// Get the device associated with this manager
	inline ID3D12Device* getDevice() const { return m_device; }

		/// Ctor
	Dx12ResourceScopeManager();
		/// Dtor
	~Dx12ResourceScopeManager();

		/// Copy from src to dst
	static void copy(const D3D12_SUBRESOURCE_DATA& src, size_t rowSizeInBytes, int numRows, int numSlices, const D3D12_MEMCPY_DEST& dst);

	protected:
	
	struct Entry
	{
		uint64_t m_completedValue;		///< If less than fence value this is completed
		ID3D12Resource* m_resource;		///< The mapped resource
	};
	
	std::deque<Entry> m_entryQueue;

	Dx12CounterFence* m_fence;			///< The fence to use
	ID3D12Device* m_device;				///< The device that resources will be constructed on
};

} // namespace Common
} // namespace nvidia

/** @} */

#endif // NV_CO_DX12_RESOURCE_SCOPE_MANAGER_H
