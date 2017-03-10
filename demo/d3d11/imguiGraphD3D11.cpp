/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "imguiGraphD3D11.h"

//direct3d headers
#include <d3d11.h>

#include "shaders/imguiVS.hlsl.h"
#include "shaders/imguiPS.hlsl.h"

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
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;
	int m_winW = 0;
	int m_winH = 0;

	uint32_t m_maxVertices = 0;

	struct Vertex
	{
		float x, y;
		float u, v;
		uint8_t rgba[4];
	};

	ID3D11RasterizerState* m_rasterizerState = nullptr;
	ID3D11SamplerState* m_samplerState = nullptr;
	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11BlendState* m_blendState = nullptr;
	ID3D11VertexShader* m_imguiVS = nullptr;
	ID3D11PixelShader* m_imguiPS = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	Vertex* m_vertexBufferData = nullptr;

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

	ID3D11Texture2D* m_texture = nullptr;
	ID3D11ShaderResourceView* m_textureSRV = nullptr;

	Vertex m_stateVert;
	uint32_t m_stateVertIdx = 0u;

	struct Params
	{
		float projection[16];

		float padding[64 - 16];
	};
}

void imguiGraphContextDestroy()
{
	COMRelease(m_rasterizerState);
	COMRelease(m_samplerState);
	COMRelease(m_inputLayout);
	COMRelease(m_blendState);
	COMRelease(m_imguiVS);
	COMRelease(m_imguiPS);
	COMRelease(m_constantBuffer);
	COMRelease(m_vertexBuffer);
}

void imguiGraphContextInit(const ImguiGraphDesc* desc)
{
	m_device = desc->device;
	m_deviceContext = desc->deviceContext;
	m_winW = desc->winW;
	m_winH = desc->winH;

	m_maxVertices = desc->maxVertices;

	// create the rastersizer state
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.FrontCounterClockwise = TRUE;	// This is non-default
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.f;
		desc.SlopeScaledDepthBias = 0.f;
		desc.DepthClipEnable = TRUE;
		desc.ScissorEnable = TRUE;		// This is non-default
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;

		m_device->CreateRasterizerState(&desc, &m_rasterizerState);
	}

	// create the sampler state
	{
		D3D11_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampler.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
		//sampler.BorderColor = D3D11_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.f;
		sampler.MaxLOD = D3D11_FLOAT32_MAX;

		m_device->CreateSamplerState(&sampler, &m_samplerState);
	}

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 3, g_imguiVS, sizeof(g_imguiVS), &m_inputLayout);
	}

	// create the blend state
	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_device->CreateBlendState(&blendDesc, &m_blendState);
	}

	// create the shaders
	m_device->CreateVertexShader(g_imguiVS, sizeof(g_imguiVS), nullptr, &m_imguiVS);
	m_device->CreatePixelShader(g_imguiPS, sizeof(g_imguiPS), nullptr, &m_imguiPS);

	// create a constant buffer
	{
		Params params = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		};

		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(params);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = &params;

		m_device->CreateBuffer(&bufDesc, &data, &m_constantBuffer);
	}

	// create a vertex buffer
	{
		UINT bufferSize = (UINT)(m_maxVertices) * sizeof(Vertex);

		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = bufferSize;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, nullptr, &m_vertexBuffer);
	}
}

void imguiGraphContextUpdate(const ImguiGraphDesc* desc)
{
	m_device = desc->device;
	m_deviceContext = desc->deviceContext;
	m_winW = desc->winW;
	m_winH = desc->winH;
}

void imguiGraphRecordBegin()
{
	Params params = {
		2.f / float(m_winW), 0.f, 0.f, -1.f,
		0.f, 2.f / float(m_winH), 0.f, -1.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};

	ID3D11DeviceContext* context = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == context->Map(m_constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			memcpy(mappedResource.pData, &params, sizeof(Params));

			context->Unmap(m_constantBuffer, 0u);
		}
	}

	// clear state
	m_stateVert = Vertex{ 0.f, 0.f, -1.f, -1.f, 0,0,0,0 };
	m_stateVertIdx = 0u;

	m_stateScissor = Scissor { 0, 0, 0, 0, m_winW, m_winH };

	// configure for triangle renderering
	context->VSSetShader(m_imguiVS, nullptr, 0u);
	context->GSSetShader(nullptr, nullptr, 0u);
	context->PSSetShader(m_imguiPS, nullptr, 0u);

	context->IASetInputLayout(m_inputLayout);
	context->OMSetBlendState(m_blendState, nullptr, 0xFFFFFFFF);
	context->PSSetSamplers(0, 1, &m_samplerState);

	context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

	context->PSSetShaderResources(0, 1, &m_textureSRV);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->RSSetState(m_rasterizerState);

	// trigger allocation of new vertex buffer as needed
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == context->Map(m_vertexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			context->Unmap(m_vertexBuffer, 0u);
		}
	}

	UINT vertexStride = sizeof(Vertex);
	UINT offset = 0u;
	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &vertexStride, &offset);

	// map allocated vertex buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == context->Map(m_vertexBuffer, 0u, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource))
		{
			m_vertexBufferData = (Vertex*)mappedResource.pData;
		}
		else
		{
			m_vertexBufferData = nullptr;
		}
	}
}

static void imguiGraphFlush()
{
	ID3D11DeviceContext* context = m_deviceContext;

	// unmap vertex buffer
	context->Unmap(m_vertexBuffer, 0u);

	Scissor& p = m_stateScissor;
	if (p.beginIdx < p.stopIdx)
	{
		int winH = m_winH;
		D3D11_RECT rect;
		rect.left = p.x;
		rect.right = p.x + p.width;
		rect.top = (winH) - (p.y + p.height);
		rect.bottom = (winH) - (p.y);
		context->RSSetScissorRects(1, &rect);

		UINT vertexCount = (p.stopIdx - p.beginIdx);
		UINT startIndex = p.beginIdx;
		context->DrawInstanced(vertexCount, 1, startIndex, 0);
	}

	// map allocated vertex buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == context->Map(m_vertexBuffer, 0u, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource))
		{
			m_vertexBufferData = (Vertex*)mappedResource.pData;
		}
		else
		{
			m_vertexBufferData = nullptr;
		}
	}
}

void imguiGraphRecordEnd()
{
	ID3D11DeviceContext* context = m_deviceContext;

	context->OMSetBlendState(nullptr,nullptr,0xFFFFFFFF);

	// unmap vertex buffer
	context->Unmap(m_vertexBuffer, 0u);

	// restore scissor
	Scissor& p = m_stateScissor;
	int winH = m_winH;
	D3D11_RECT rect;
	rect.left = p.x;
	rect.right = p.x + p.width;
	rect.top = (winH) - (p.y + p.height);
	rect.bottom = (winH) - (p.y);
	context->RSSetScissorRects(1, &rect);

	context->RSSetState(nullptr);
}

void imguiGraphEnableScissor(int x, int y, int width, int height)
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

void imguiGraphDisableScissor()
{
	if (m_stateVertIdx == 0) return;

	// mark end of last region
	m_stateScissor.stopIdx = m_stateVertIdx;

	imguiGraphFlush();

	m_stateScissor.beginIdx = m_stateVertIdx;
	m_stateScissor.stopIdx = m_stateVertIdx;
	m_stateScissor.x = 0;
	m_stateScissor.y = 0;
	m_stateScissor.width = m_winW;
	m_stateScissor.height = m_winH;
}

void imguiGraphVertex2f(float x, float y)
{
	float v[2] = { x,y };
	imguiGraphVertex2fv(v);
}

void imguiGraphVertex2fv(const float* v)
{
	// update state
	m_stateVert.x = v[0];
	m_stateVert.y = v[1];

	Vertex* vdata = m_vertexBufferData;

	// push vertex
	if ((m_stateVertIdx) < m_maxVertices)
	{
		vdata[m_stateVertIdx++] = m_stateVert;
	}
}

void imguiGraphTexCoord2f(float u, float v)
{
	m_stateVert.u = u;
	m_stateVert.v = v;
}

void imguiGraphColor4ub(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
	m_stateVert.rgba[0] = red;
	m_stateVert.rgba[1] = green;
	m_stateVert.rgba[2] = blue;
	m_stateVert.rgba[3] = alpha;
}

void imguiGraphColor4ubv(const uint8_t* v)
{
	m_stateVert.rgba[0] = v[0];
	m_stateVert.rgba[1] = v[1];
	m_stateVert.rgba[2] = v[2];
	m_stateVert.rgba[3] = v[3];
}

void imguiGraphFontTextureEnable()
{

}

void imguiGraphFontTextureDisable()
{
	m_stateVert.u = -1.f;
	m_stateVert.v = -1.f;
}

void imguiGraphFontTextureInit(unsigned char* data)
{
	ID3D11DeviceContext* context = m_deviceContext;

	// create texture
	{
		UINT width = 512;
		UINT height = 512;

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Usage = D3D11_USAGE_IMMUTABLE;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = data;
		initData.SysMemPitch = width;

		if (m_device->CreateTexture2D(&texDesc, &initData, &m_texture))
		{
			return;
		}

		if (m_device->CreateShaderResourceView(m_texture, nullptr, &m_textureSRV))
		{
			return;
		}
	}

}

void imguiGraphFontTextureRelease()
{
	COMRelease(m_texture);
	COMRelease(m_textureSRV);
}