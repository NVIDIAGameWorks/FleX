/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#include "NvCoDx12CounterFence.h"

namespace nvidia {
namespace Common {

Dx12CounterFence::~Dx12CounterFence()
{
	if (m_event)
	{
		CloseHandle(m_event);
	}
}

int Dx12CounterFence::init(ID3D12Device* device, uint64_t initialValue)
{
	m_currentValue = initialValue;

	NV_RETURN_ON_FAIL(device->CreateFence(m_currentValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	// Create an event handle to use for frame synchronization.
	m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_event == nullptr)
	{
		int res = HRESULT_FROM_WIN32(GetLastError());
		return NV_FAILED(res) ? res : NV_FAIL;
	}
	return NV_OK;
}

uint64_t Dx12CounterFence::nextSignal(ID3D12CommandQueue* commandQueue)
{
	// Increment the fence value. Save on the frame - we'll know that frame is done when the fence value >= 
	m_currentValue++;
	// Schedule a Signal command in the queue.
	int res = commandQueue->Signal(m_fence.Get(), m_currentValue);
	if (NV_FAILED(res))
	{
		printf("Signal failed");	
	}
	return m_currentValue;
}

void Dx12CounterFence::waitUntilCompleted(uint64_t completedValue)
{
	// You can only wait for a value that is less than or equal to the current value
	assert(completedValue <= m_currentValue);

	// Wait until the previous frame is finished.
	while (m_fence->GetCompletedValue() < completedValue)
	{
		// Make it signal with the current value
		HRESULT res = m_fence->SetEventOnCompletion(completedValue, m_event);
		if (FAILED(res)) { assert(0); return; }

		WaitForSingleObject(m_event, INFINITE);
	}
}

void Dx12CounterFence::nextSignalAndWait(ID3D12CommandQueue* commandQueue)
{
	waitUntilCompleted(nextSignal(commandQueue));
}

} // namespace Common
} // namespace nvidia
