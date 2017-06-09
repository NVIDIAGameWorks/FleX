/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */


#include <NvCoDx12HelperUtil.h>
#include <external/D3D12/include/d3dx12.h>

#include "bufferD3D12.h"

#include <vector>

namespace FlexSample {

int IndexBufferD3D12::init(const RenderStateD3D12& state, int stride, ptrdiff_t numIndices, const void* sysMem)
{
	assert(sysMem);
	assert(stride == 4);
	const size_t bufferSize = stride * numIndices;

	{
		ComPtr<ID3D12Resource> resource;
		CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_DESC resourceDesc(CD3DX12_RESOURCE_DESC::Buffer(bufferSize));
		if (sysMem)
		{
			NV_RETURN_ON_FAIL(state.m_device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, 
				IID_PPV_ARGS(&resource)));
			NV_RETURN_ON_FAIL(state.m_scopeManager->upload(state.m_commandList, sysMem, resource.Get(), D3D12_RESOURCE_STATE_COMMON));
		}
		else
		{
			NV_RETURN_ON_FAIL(state.m_device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
				IID_PPV_ARGS(&resource)));
		}
		setResource(resource.Get(), D3D12_RESOURCE_STATE_COMMON);
	}
		
	memset(&m_indexBufferView, 0, sizeof(m_indexBufferView));
	m_indexBufferView.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = UINT(bufferSize);
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	return NV_OK;
}

int VertexBufferD3D12::init(const RenderStateD3D12& state, int stride, ptrdiff_t numElements, const void* sysMem, D3D12_RESOURCE_FLAGS resourceFlags)
{
	if (!sysMem)
	{
		memset(&m_vertexBufferView, 0, sizeof(m_vertexBufferView));
		return NV_OK;
	}

	size_t bufferSize = size_t(numElements * stride);

	{
		CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_DESC resourceDesc(CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resourceFlags));
		ComPtr<ID3D12Resource> resource;

		if (sysMem)
		{
			NV_RETURN_ON_FAIL(state.m_device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, 
				IID_PPV_ARGS(&resource)));
			NV_RETURN_ON_FAIL(state.m_scopeManager->upload(state.m_commandList, sysMem, resource.Get(), D3D12_RESOURCE_STATE_COMMON));
		}
		else
		{
			NV_RETURN_ON_FAIL(state.m_device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
				IID_PPV_ARGS(&resource)));
		}
		setResource(resource.Get(), D3D12_RESOURCE_STATE_COMMON);
	}

	{
		memset(&m_vertexBufferView, 0, sizeof(m_vertexBufferView));
		m_vertexBufferView.BufferLocation = m_resource->GetGPUVirtualAddress();
		m_vertexBufferView.SizeInBytes = UINT(bufferSize);
		m_vertexBufferView.StrideInBytes = stride;
	}
	
	return NV_OK;
}

int VertexBufferD3D12::init(const RenderStateD3D12& state, int srcStride, int dstStride, ptrdiff_t numElements, const void* sysMem, D3D12_RESOURCE_FLAGS resourceFlags)
{
	if (srcStride == dstStride || sysMem == nullptr)
	{
		return init(state, dstStride, numElements, sysMem, resourceFlags);
	}
	else
	{
		if (srcStride == 4 * 4 && dstStride == 4 * 3)
		{
			std::vector<uint32_t> buf(numElements * 3);
			uint32_t* dst = &buf.front();
			const uint32_t* src = (const uint32_t*)sysMem;

			for (ptrdiff_t i = 0; i < numElements; i++, dst += 3, src += 4)
			{
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
			}

			return init(state, dstStride, numElements, &buf.front(), resourceFlags);
		}
	}

	assert(!"Unhandled conversion");
	return NV_FAIL;
}


} // namespace FlexSample
