/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_DX12_COUNTER_FENCE_H
#define NV_CO_DX12_COUNTER_FENCE_H

#include <NvResult.h>

#define NOMINMAX
#include <d3d12.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <wrl.h>

using namespace Microsoft::WRL;

/** \addtogroup common
@{
*/

namespace nvidia {
namespace Common { 

/*! \brief A class to simplify using Dx12 fences. 

A fence is a mechanism to track GPU work. This is achieved by having a counter that the CPU holds 
called the current value. Calling nextSignal will increase the CPU counter, and add a fence 
with that value to the commandQueue. When the GPU has completed all the work before the fence it will 
update the completed value. This is typically used when 
the CPU needs to know the GPU has finished some piece of work has completed. To do this the CPU
can check the completed value, and when it is greater or equal to the value returned by nextSignal the 
CPU will know that all the work prior to when the nextSignal was added to the queue will have completed. 

NOTE! This cannot be used across threads, as for amongst other reasons SetEventOnCompletion 
only works with a single value.

Signal on the CommandQueue updates the fence on the GPU side. Signal on the fence object changes 
the value on the CPU side (not used here).

Useful article describing how Dx12 synchronization works:
https://msdn.microsoft.com/en-us/library/windows/desktop/dn899217%28v=vs.85%29.aspx
*/
class Dx12CounterFence
{
	//NV_CO_DECLARE_CLASS_BASE(Dx12CounterFence);
public:
		/// Must be called before used
	int init(ID3D12Device* device, uint64_t initialValue = 0);
		/// Increases the counter, signals the queue and waits for the signal to be hit
	void nextSignalAndWait(ID3D12CommandQueue* queue);
		/// Signals with next counter value. Returns the value the signal was called on
	uint64_t nextSignal(ID3D12CommandQueue* commandQueue);
		/// Get the current value
	inline uint64_t getCurrentValue() const { return m_currentValue; }
		/// Get the completed value
	inline uint64_t getCompletedValue() const { return m_fence->GetCompletedValue(); }

		/// Waits for the the specified value 
	void waitUntilCompleted(uint64_t completedValue);

		/// Ctor
	Dx12CounterFence():m_event(nullptr), m_currentValue(0) {}
		/// Dtor
	~Dx12CounterFence();

	protected:
	HANDLE m_event;
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_currentValue;
};

} // namespace Common
} // namespace nvidia

/** @} */

#endif // NV_CO_DX12_COUNTER_FENCE_H
