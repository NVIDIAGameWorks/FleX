
#include "renderStateD3D12.h"

namespace FlexSample {

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Dx12RenderStateManager !!!!!!!!!!!!!!!!!!!!!!!!!! */

int RenderStateManagerD3D12::initialize(AppGraphCtxD3D12* renderContext, size_t maxHeapAlloc)
{
	m_renderContext = renderContext;
	m_device = renderContext->m_device;
	
	m_fence.init(m_device);
	m_scopeManager.init(m_device, &m_fence);
	m_srvCbvUavDescriptorHeap.init(m_device, 256, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	m_samplerDescriptorHeap.init(m_device, 64, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	{
		NvCo::Dx12CircularResourceHeap::Desc desc;
		desc.init();
		desc.m_blockSize = maxHeapAlloc;
		m_constantHeap.init(m_device, desc, &m_fence);
	}

	return NV_OK;
}

void RenderStateManagerD3D12::updateCompleted()
{
	m_scopeManager.updateCompleted();
	m_constantHeap.updateCompleted();
}

void RenderStateManagerD3D12::onGpuWorkSubmitted(ID3D12CommandQueue* commandQueue)
{
	assert(commandQueue);
	if (!commandQueue)
	{
		printf("Must pass a ID3D12CommandQueue to onGpuWorkSubmitted");
		return;
	}

	const uint64_t signalValue = m_fence.nextSignal(commandQueue);
	
	m_scopeManager.addSync(signalValue);
	m_constantHeap.addSync(signalValue);
}

RenderStateD3D12 RenderStateManagerD3D12::getState()
{
	RenderStateD3D12 state;

	state.m_renderContext = m_renderContext;
	state.m_commandList = m_renderContext->m_commandList;
	state.m_device = m_device;

	state.m_constantHeap = &m_constantHeap;
	state.m_srvCbvUavDescriptorHeap = &m_srvCbvUavDescriptorHeap;
	state.m_fence = &m_fence;
	state.m_scopeManager = &m_scopeManager;
	state.m_samplerDescriptorHeap = &m_samplerDescriptorHeap;

	return state;
}


} // namespace FlexSample