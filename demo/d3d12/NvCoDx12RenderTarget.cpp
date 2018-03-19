
#include "NvCoDx12RenderTarget.h"
#include "appD3D12Ctx.h"

#include <external/D3D12/include/d3dx12.h>

namespace nvidia {
namespace Common {

Dx12RenderTarget::Dx12RenderTarget()
{
	memset(m_srvIndices, -1, sizeof(m_srvIndices));
}

int Dx12RenderTarget::init(AppGraphCtxD3D12* renderContext, const Desc& desc)
{
	ID3D12Device* device = renderContext->m_device;

	m_desc = desc;

	// set viewport
	{
		m_viewport.Width = float(desc.m_width);
		m_viewport.Height = float(desc.m_height);
		m_viewport.MinDepth = 0;
		m_viewport.MaxDepth = 1;
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
	}

	{
		m_scissorRect.left = 0;
		m_scissorRect.top = 0;
		m_scissorRect.right = desc.m_width;
		m_scissorRect.bottom = desc.m_height;
	}

	if (desc.m_targetFormat != DXGI_FORMAT_UNKNOWN)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		NV_RETURN_ON_FAIL(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeapRtv)));

		DXGI_FORMAT resourceFormat = DxFormatUtil::calcResourceFormat(DxFormatUtil::USAGE_TARGET, m_desc.m_usageFlags, desc.m_targetFormat);
		DXGI_FORMAT targetFormat = DxFormatUtil::calcFormat(DxFormatUtil::USAGE_TARGET, resourceFormat);

		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(resourceFormat, UINT(desc.m_width), UINT(desc.m_height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		// Depending on usage this format may not be right...
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = targetFormat;
		clearValue.Color[0] = desc.m_targetClearColor[0];
		clearValue.Color[1] = desc.m_targetClearColor[1];
		clearValue.Color[2] = desc.m_targetClearColor[2];
		clearValue.Color[3] = desc.m_targetClearColor[3];

		NV_RETURN_ON_FAIL(m_renderTarget.initCommitted(device, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue));

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = targetFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		device->CreateRenderTargetView(m_renderTarget, &rtvDesc, m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart());
	}

	// Create the depth stencil descriptor heap
	if (desc.m_depthStencilFormat != DXGI_FORMAT_UNKNOWN)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		NV_RETURN_ON_FAIL(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeapDsv)));
	
		DXGI_FORMAT resourceFormat = DxFormatUtil::calcResourceFormat(DxFormatUtil::USAGE_DEPTH_STENCIL, m_desc.m_usageFlags, desc.m_depthStencilFormat);
		DXGI_FORMAT depthStencilFormat = DxFormatUtil::calcFormat(DxFormatUtil::USAGE_DEPTH_STENCIL, resourceFormat);

		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(resourceFormat, UINT(desc.m_width), UINT(desc.m_height), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = depthStencilFormat; 
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		NV_RETURN_ON_FAIL(m_depthStencilBuffer.initCommitted(device, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue));
		
		// Create the depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = depthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		device->CreateDepthStencilView(m_depthStencilBuffer, &dsvDesc, m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart());
	}

	return NV_OK;
}

/* static */DxFormatUtil::UsageType Dx12RenderTarget::getUsageType(BufferType type)
{
	switch (type)
	{
		case BUFFER_DEPTH_STENCIL:	return DxFormatUtil::USAGE_DEPTH_STENCIL;
		case BUFFER_TARGET:			return DxFormatUtil::USAGE_TARGET;
		default:					return DxFormatUtil::USAGE_UNKNOWN;
	}
}

void Dx12RenderTarget::bind(AppGraphCtxD3D12* renderContext)
{
	ID3D12GraphicsCommandList* commandList = renderContext->m_commandList;

	// Work out what is bound
	int numTargets = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};

	if (m_renderTarget)
	{
		rtvHandle = m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart();
		numTargets = 1;
	}
	if (m_depthStencilBuffer)
	{
		dsvHandle = m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(numTargets, &rtvHandle, FALSE, &dsvHandle);
	}
	else
	{
		commandList->OMSetRenderTargets(numTargets, &rtvHandle, FALSE, nullptr);
	}

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
}


void Dx12RenderTarget::clear(AppGraphCtxD3D12* renderContext)
{
	ID3D12GraphicsCommandList* commandList = renderContext->m_commandList;
	D3D12_RECT rect = { 0, 0, m_desc.m_width, m_desc.m_height };

	// Clear
	if (m_renderTarget)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart();
		commandList->ClearRenderTargetView(rtvHandle, m_desc.m_targetClearColor, 1, &rect);
	}
	if (m_depthStencilBuffer)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart();
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, m_desc.m_depthStencilClearDepth, 0, 1, &rect);
	}
}

void Dx12RenderTarget::bindAndClear(AppGraphCtxD3D12* renderContext)
{
	ID3D12GraphicsCommandList* commandList = renderContext->m_commandList;

	// Work out what is bound
	int numTargets = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {}; 
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {}; 

	if (m_renderTarget)
	{
		rtvHandle = m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart();
		numTargets = 1;
	}
	if (m_depthStencilBuffer)
	{
		dsvHandle = m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(numTargets, &rtvHandle, FALSE, &dsvHandle);
	}
	else
	{
		commandList->OMSetRenderTargets(numTargets, &rtvHandle, FALSE, nullptr);
	}

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	D3D12_RECT rect = { 0, 0, m_desc.m_width, m_desc.m_height };

	// Clear
	if (m_renderTarget)
	{
		commandList->ClearRenderTargetView(rtvHandle, m_desc.m_targetClearColor, 1, &rect);
	}
	if (m_depthStencilBuffer)
	{
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, m_desc.m_depthStencilClearDepth, 0, 1, &rect);
	}
}

void Dx12RenderTarget::setShadowDefaultLight(FXMVECTOR eye, FXMVECTOR at, FXMVECTOR up)
{
	float sizeX = 50.0f;
	float sizeY = 50.0f;
	float znear = -200.0f;
	float zfar = 200.0f;

	setShadowLightMatrices(eye, at, up, sizeX, sizeY, znear, zfar);
}

void Dx12RenderTarget::setShadowLightMatrices(FXMVECTOR eye, FXMVECTOR lookAt, FXMVECTOR up, float sizeX, float sizeY, float zNear, float zFar)
{
	m_shadowLightView = XMMatrixLookAtLH(eye, lookAt, up);
	m_shadowLightProjection = XMMatrixOrthographicLH(sizeX, sizeY, zNear, zFar);
	DirectX::XMMATRIX clip2Tex(0.5, 0, 0, 0,
		0, -0.5, 0, 0,
		0, 0, 1, 0,
		0.5, 0.5, 0, 1);
	DirectX::XMMATRIX viewProjection = m_shadowLightView * m_shadowLightProjection;
	m_shadowLightWorldToTex = viewProjection * clip2Tex;
}

void Dx12RenderTarget::toWritable(AppGraphCtxD3D12* renderContext)
{
	Dx12BarrierSubmitter submitter(renderContext->m_commandList);

	m_renderTarget.transition(D3D12_RESOURCE_STATE_RENDER_TARGET, submitter);
	m_depthStencilBuffer.transition(D3D12_RESOURCE_STATE_DEPTH_WRITE, submitter);
}

void Dx12RenderTarget::toReadable(AppGraphCtxD3D12* renderContext)
{
	Dx12BarrierSubmitter submitter(renderContext->m_commandList);

	m_renderTarget.transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, submitter);
	m_depthStencilBuffer.transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, submitter);
}

DXGI_FORMAT Dx12RenderTarget::getSrvFormat(BufferType type) const
{
	return Dx12Resource::calcFormat(DxFormatUtil::USAGE_SRV, getResource(type));
}

DXGI_FORMAT Dx12RenderTarget::getTargetFormat(BufferType type) const
{
	return Dx12Resource::calcFormat(getUsageType(type), getResource(type));
}

D3D12_SHADER_RESOURCE_VIEW_DESC Dx12RenderTarget::calcDefaultSrvDesc(BufferType type) const
{
	ID3D12Resource* resource = getResource(type);

	D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC desc;

	desc.Format = getSrvFormat(type);
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	desc.Texture2D.MipLevels = resourceDesc.MipLevels;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.PlaneSlice = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;
	return desc;
}

int Dx12RenderTarget::allocateSrvView(BufferType type, ID3D12Device* device, Dx12DescriptorHeap& heap, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
	if (m_srvIndices[type] >= 0)
	{
		printf("A srv index has already been associated!");
		return m_srvIndices[type];
	}

	if (!desc)
	{
		// If not set construct a default desc
		D3D12_SHADER_RESOURCE_VIEW_DESC defaultDesc = calcDefaultSrvDesc(type);
		return allocateSrvView(type, device, heap, &defaultDesc);
	}

	// Create the srv for the shadow buffer
	ID3D12Resource* resource = getResource(type);
	int srvIndex = heap.allocate();

	if (srvIndex < 0)
	{
		return srvIndex;
	}

	// Create on the the heap
	device->CreateShaderResourceView(resource, desc, heap.getCpuHandle(srvIndex));

	m_srvIndices[type] = srvIndex;
	// Return the allocated index
	return srvIndex;
}

void Dx12RenderTarget::setDebugName(BufferType type, const std::wstring& name)
{
	getResource(type).setDebugName(name);
}

void Dx12RenderTarget::setDebugName(const std::wstring& name)
{
	std::wstring buf(name);
	buf += L" [Target]";
	setDebugName(BUFFER_TARGET, buf);
	buf = name;
	buf += L" [DepthStencil]";
	setDebugName(BUFFER_DEPTH_STENCIL, buf);
}


void Dx12RenderTarget::createSrv(ID3D12Device* device, Dx12DescriptorHeap& heap, BufferType type, int descriptorIndex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = calcDefaultSrvDesc(type);
	ID3D12Resource* resource = getResource(type);
	device->CreateShaderResourceView(resource, &srvDesc, heap.getCpuHandle(descriptorIndex));
}

} // namespace Common 
} // namespace nvidia
