/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

//direct3d headers
#define NOMINMAX
#include <d3d12.h>

#include "imguiGraphD3D12.h"

#include "../d3d/shaders/imguiVS.hlsl.h"
#include "../d3d/shaders/imguiPS.hlsl.h"

namespace
{
	template <class T>
	void inline COMRelease(T& t)
	{
		if (t) t->Release();
		t = nullptr;
	}
}

namespace
{
	ImguiGraphDescD3D12 m_desc = {};

	struct Vertex
	{
		float x, y;
		float u, v;
		uint8_t rgba[4];
	};

	ID3D12RootSignature* m_rootSignature = nullptr;
	ID3D12PipelineState* m_pipelineState = nullptr;
	ID3D12Resource* m_constantBuffer = nullptr;
	UINT8* m_constantBufferData = nullptr;
	ID3D12Resource* m_vertexBuffer = nullptr;
	Vertex* m_vertexBufferData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};

	struct Scissor
	{
		int beginIdx;
		int stopIdx;
		int x;
		int y;
		int width;
		int height;
	};
	Scissor m_stateScissor = {};

	ID3D12Resource* m_textureUploadHeap = nullptr;
	ID3D12Resource* m_texture = nullptr;

	ID3D12DescriptorHeap* m_srvUavHeapCPU = nullptr;
	ID3D12DescriptorHeap* m_srvUavHeapGPU = nullptr;

	Vertex m_stateVert;
	uint32_t m_stateVertIdx = 0u;

	struct Params
	{
		float projection[16];

		float padding[64 - 16];
	};
	static const int frameCount = 4;
	int frameIndex = 0;
}

void imguiGraphContextInitD3D12(const ImguiGraphDesc* descIn)
{
	const auto desc = cast_to_imguiGraphDescD3D12(descIn);

	m_desc = *desc;

	// create the root signature
	{
		D3D12_DESCRIPTOR_RANGE ranges[1];
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors = 1u;
		ranges[0].BaseShaderRegister = 0u;
		ranges[0].RegisterSpace = 0u;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER params[2];
		params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[0].Descriptor.ShaderRegister = 0u;
		params[0].Descriptor.RegisterSpace = 0u;
		params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[1].DescriptorTable.NumDescriptorRanges = 1;
		params[1].DescriptorTable.pDescriptorRanges = ranges;
		params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.NumParameters = 2;
		desc.pParameters = params;
		desc.NumStaticSamplers = 1u;
		desc.pStaticSamplers = &sampler;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* signature = nullptr;
		ID3DBlob* error = nullptr;
		HRESULT hr = S_OK;
		if (hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))
		{
			return;
		}
		if (hr = m_desc.device->CreateRootSignature(0u, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))
		{
			return;
		}
		COMRelease(signature);
		COMRelease(error);
	}

	// create the pipeline state object
	{
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		const bool wireFrame = false;

		D3D12_RASTERIZER_DESC rasterDesc;
		if (wireFrame)
		{
			rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
		}
		else
		{
			rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
			rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
		}
		rasterDesc.FrontCounterClockwise = TRUE; // FALSE;
		rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.MultisampleEnable = FALSE;
		rasterDesc.AntialiasedLineEnable = FALSE;
		rasterDesc.ForcedSampleCount = 0;
		rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_BLEND_DESC blendDesc;
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		{
			const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
			{
				TRUE,FALSE,
				D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};
			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout.NumElements = 3;
		psoDesc.InputLayout.pInputElementDescs = inputElementDescs;
		psoDesc.pRootSignature = m_rootSignature;
		psoDesc.VS.pShaderBytecode = g_imguiVS;
		psoDesc.VS.BytecodeLength = sizeof(g_imguiVS);
		psoDesc.PS.pShaderBytecode = g_imguiPS;
		psoDesc.PS.BytecodeLength = sizeof(g_imguiPS);
		psoDesc.RasterizerState = rasterDesc;
		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = desc->numMSAASamples;
		HRESULT hr = S_OK;
		if (hr = m_desc.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)))
		{
			return;
		}
	}

	// create a constant buffer
	{
		Params params = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		};

		HRESULT hr = S_OK;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0u;
		heapProps.VisibleNodeMask = 0u;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0u;
		desc.Width = frameCount*sizeof(params);
		desc.Height = 1u;
		desc.DepthOrArraySize = 1u;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1u;
		desc.SampleDesc.Quality = 0u;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (hr = m_desc.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&m_constantBuffer)))
		{
			return;
		}

		UINT8* pdata;
		D3D12_RANGE readRange = {};
		if (hr = m_constantBuffer->Map(0, &readRange, (void**)&pdata))
		{
			return;
		}
		else
		{
			memcpy(pdata, &params, sizeof(params));
			m_constantBufferData = pdata;
			//m_constantBuffer->Unmap(0, nullptr);		// leave mapped
		}
	}

	// create a vertex buffer
	{
		HRESULT hr = S_OK;

		UINT bufferSize = (UINT)(m_desc.maxVertices * frameCount) * sizeof(Vertex);

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0u;
		heapProps.VisibleNodeMask = 0u;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0u;
		desc.Width = bufferSize;
		desc.Height = 1u;
		desc.DepthOrArraySize = 1u;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1u;
		desc.SampleDesc.Quality = 0u;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (hr = m_desc.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&m_vertexBuffer)))
		{
			return;
		}

		UINT8* pdata;
		D3D12_RANGE readRange = {};
		if (hr = m_vertexBuffer->Map(0, &readRange, (void**)&pdata))
		{
			return;
		}
		else
		{
			m_vertexBufferData = (Vertex*)pdata;
			//m_vertexBufferUpload->Unmap(0, nullptr);
		}

		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = bufferSize;
	}
}

void imguiGraphContextUpdateD3D12(const ImguiGraphDesc* descIn)
{
	const auto desc = cast_to_imguiGraphDescD3D12(descIn);

	m_desc = *desc;
}

void imguiGraphContextDestroyD3D12()
{
	COMRelease(m_rootSignature);
	COMRelease(m_pipelineState);
	COMRelease(m_constantBuffer);
	COMRelease(m_vertexBuffer);
}

void imguiGraphRecordBeginD3D12()
{
	frameIndex = (frameIndex + 1) % frameCount;

	Params params = {
		2.f / float(m_desc.winW), 0.f, 0.f, -1.f,
		0.f, 2.f / float(m_desc.winH), 0.f, -1.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};

	memcpy(m_constantBufferData + frameIndex*sizeof(Params), &params, sizeof(Params));

	// clear state
	m_stateVert = Vertex{ 0.f, 0.f, -1.f, -1.f, 0,0,0,0 };
	m_stateVertIdx = 0u;

	m_stateScissor = Scissor { 0, 0, 0, 0, m_desc.winW, m_desc.winH };

	// configure for triangle renderering
	ID3D12GraphicsCommandList* commandList = m_desc.commandList;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	ID3D12DescriptorHeap* heap = nullptr;
	if (m_desc.dynamicHeapCbvSrvUav.reserveDescriptors)
	{
		auto handle = m_desc.dynamicHeapCbvSrvUav.reserveDescriptors(m_desc.dynamicHeapCbvSrvUav.userdata, 
			1u, m_desc.lastFenceCompleted, m_desc.nextFenceValue);
		heap = handle.heap;
		srvHandleCPU = handle.cpuHandle;
		srvHandleGPU = handle.gpuHandle;
	}
	else
	{
		if (m_srvUavHeapGPU == nullptr)
		{
			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = 1;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			m_desc.device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvUavHeapGPU));
		}
		heap = m_srvUavHeapGPU;
		srvHandleCPU = m_srvUavHeapGPU->GetCPUDescriptorHandleForHeapStart();
		srvHandleGPU = m_srvUavHeapGPU->GetGPUDescriptorHandleForHeapStart();
	}

	commandList->SetDescriptorHeaps(1, &heap);

	m_desc.device->CopyDescriptorsSimple(1u, srvHandleCPU, m_srvUavHeapCPU->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	commandList->SetGraphicsRootSignature(m_rootSignature);
	commandList->SetPipelineState(m_pipelineState);

	D3D12_GPU_VIRTUAL_ADDRESS cbvHandle = m_constantBuffer->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(0, cbvHandle + frameIndex * sizeof(Params));

	commandList->SetGraphicsRootDescriptorTable(1, srvHandleGPU);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
}

static void imguiGraphFlush()
{
	ID3D12GraphicsCommandList* commandList = m_desc.commandList;

	Scissor& p = m_stateScissor;
	if (p.beginIdx < p.stopIdx)
	{
		int winH = m_desc.winH;
		D3D12_RECT rect;
		rect.left = p.x;
		rect.right = p.x + p.width;
		rect.top = (winH) - (p.y + p.height);
		rect.bottom = (winH) - (p.y);
		commandList->RSSetScissorRects(1, &rect);

		UINT vertexCount = (p.stopIdx - p.beginIdx);
		UINT startIndex = p.beginIdx + frameIndex * m_desc.maxVertices;
		commandList->DrawInstanced(vertexCount, 1, startIndex, 0);
	}
}

void imguiGraphRecordEndD3D12()
{
	ID3D12GraphicsCommandList* commandList = m_desc.commandList;

	// no need to hold onto this
	COMRelease(m_textureUploadHeap);

	// restore scissor
	Scissor& p = m_stateScissor;
	int winH = m_desc.winH;
	D3D12_RECT rect;
	rect.left = p.x;
	rect.right = p.x + p.width;
	rect.top = (winH) - (p.y + p.height);
	rect.bottom = (winH) - (p.y);
	commandList->RSSetScissorRects(1, &rect);
}

void imguiGraphEnableScissorD3D12(int x, int y, int width, int height)
{
	// mark end of last region
	m_stateScissor.stopIdx = m_stateVertIdx;

	imguiGraphFlush();

	m_stateScissor.beginIdx = m_stateVertIdx;
	m_stateScissor.stopIdx = m_stateVertIdx;
	m_stateScissor.x = x;
	m_stateScissor.y = y;
	m_stateScissor.width = width;
	m_stateScissor.height = height;
}

void imguiGraphDisableScissorD3D12()
{
	if (m_stateVertIdx == 0) return;

	// mark end of last region
	m_stateScissor.stopIdx = m_stateVertIdx;

	imguiGraphFlush();

	m_stateScissor.beginIdx = m_stateVertIdx;
	m_stateScissor.stopIdx = m_stateVertIdx;
	m_stateScissor.x = 0;
	m_stateScissor.y = 0;
	m_stateScissor.width = m_desc.winW;
	m_stateScissor.height = m_desc.winH;
}

void imguiGraphVertex2fD3D12(float x, float y)
{
	float v[2] = { x,y };
	imguiGraphVertex2fvD3D12(v);
}

void imguiGraphVertex2fvD3D12(const float* v)
{
	// update state
	m_stateVert.x = v[0];
	m_stateVert.y = v[1];

	Vertex* vdata = &m_vertexBufferData[frameIndex * m_desc.maxVertices];

	// push vertex
	if ((m_stateVertIdx) < m_desc.maxVertices)
	{
		vdata[m_stateVertIdx++] = m_stateVert;
	}
}

void imguiGraphTexCoord2fD3D12(float u, float v)
{
	m_stateVert.u = u;
	m_stateVert.v = v;
}

void imguiGraphColor4ubD3D12(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
	m_stateVert.rgba[0] = red;
	m_stateVert.rgba[1] = green;
	m_stateVert.rgba[2] = blue;
	m_stateVert.rgba[3] = alpha;
}

void imguiGraphColor4ubvD3D12(const uint8_t* v)
{
	m_stateVert.rgba[0] = v[0];
	m_stateVert.rgba[1] = v[1];
	m_stateVert.rgba[2] = v[2];
	m_stateVert.rgba[3] = v[3];
}

void imguiGraphFontTextureEnableD3D12()
{

}

void imguiGraphFontTextureDisableD3D12()
{
	m_stateVert.u = -1.f;
	m_stateVert.v = -1.f;
}

void imguiGraphFontTextureInitD3D12(unsigned char* data)
{
	ID3D12GraphicsCommandList* commandList = m_desc.commandList;

	// Create the texture
	{
		UINT width = 512;
		UINT height = 512;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 0u;
		heapProps.VisibleNodeMask = 0u;

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.MipLevels = 1u;
		texDesc.Format = DXGI_FORMAT_R8_UNORM;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		texDesc.DepthOrArraySize = 1u;
		texDesc.SampleDesc.Count = 1u;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		if (m_desc.device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_texture)
		))
		{
			return;
		}

		// get footprint information
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		UINT64 uploadHeapSize = 0u;
		m_desc.device->GetCopyableFootprints(&texDesc, 0u, 1u, 0u, &footprint, nullptr, nullptr, &uploadHeapSize);

		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC bufferDesc = texDesc;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0u;
		bufferDesc.Width = uploadHeapSize;
		bufferDesc.Height = 1u;
		bufferDesc.DepthOrArraySize = 1u;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		if (m_desc.device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_textureUploadHeap)
		))
		{
			return;
		}

		// map upload heap, and convert rgb bitmap to rgba
		UINT8* pdata;
		D3D12_RANGE readRange = {};
		if (m_textureUploadHeap->Map(0, &readRange, (void**)&pdata))
		{
			return;
		}
		else
		{
			UINT8* dst = pdata;
			UINT elements = width*height;
			UINT8* src = data;
			for (UINT j = 0; j < height; j++)
			{
				for (UINT i = 0; i < width; i++)
				{
					UINT idx = j * (footprint.Footprint.RowPitch) + i;

					UINT8 a = src[j * width + i];
					dst[idx] = a;
				}
			}

			m_textureUploadHeap->Unmap(0, nullptr);
		}

		// add copy from upload heap to default heap to command list
		D3D12_TEXTURE_COPY_LOCATION dstCopy = {};
		D3D12_TEXTURE_COPY_LOCATION srcCopy = {};
		dstCopy.pResource = m_texture;
		dstCopy.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstCopy.SubresourceIndex = 0u;
		srcCopy.pResource = m_textureUploadHeap;
		srcCopy.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcCopy.PlacedFootprint = footprint;
		commandList->CopyTextureRegion(&dstCopy, 0, 0, 0, &srcCopy, nullptr);

		D3D12_RESOURCE_BARRIER barrier[1] = {};
		auto textureBarrier = [&barrier](UINT idx, ID3D12Resource* texture)
		{
			barrier[idx].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier[idx].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier[idx].Transition.pResource = texture;
			barrier[idx].Transition.Subresource = 0u;
			barrier[idx].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier[idx].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		};
		textureBarrier(0, m_texture);
		commandList->ResourceBarrier(1, barrier);

		// create an SRV heap and descriptor for the texture
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (m_desc.device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvUavHeapCPU)))
		{
			return;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE srvUavHandle = m_srvUavHeapCPU->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texDesc.Format;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

		m_desc.device->CreateShaderResourceView(m_texture, &srvDesc, srvUavHandle);
	}
}

void imguiGraphFontTextureReleaseD3D12()
{
	COMRelease(m_texture);
	COMRelease(m_textureUploadHeap);
	COMRelease(m_srvUavHeapCPU);
	COMRelease(m_srvUavHeapGPU);
}
