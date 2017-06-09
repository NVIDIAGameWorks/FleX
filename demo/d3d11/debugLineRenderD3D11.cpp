/*
* Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define NOMINMAX
#include <d3d11.h>

#include <math.h>

#include "../d3d/shaders/debugLineVS.hlsl.h"
#include "../d3d/shaders/debugLinePS.hlsl.h"

// this
#include "debugLineRenderD3D11.h"

void DebugLineRenderD3D11::init(ID3D11Device* d, ID3D11DeviceContext* c) 
{
	m_device = d;
	m_context = c;

	// create the rasterizer state
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.FrontCounterClockwise = TRUE;	// This is non-default
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.f;
		desc.SlopeScaledDepthBias = 0.f;
		desc.DepthClipEnable = TRUE;
		desc.ScissorEnable = FALSE;		// This is non-default
		desc.MultisampleEnable = TRUE;
		desc.AntialiasedLineEnable = FALSE;			

		m_device->CreateRasterizerState(&desc, m_rasterizerState.GetAddressOf());
	}

	{
		D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
		depthStateDesc.DepthEnable = FALSE;	// disable depth test
		depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		m_device->CreateDepthStencilState(&depthStateDesc, m_depthStencilState.GetAddressOf());
	}

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 2, g_debugLineVS, sizeof(g_debugLineVS), m_inputLayout.GetAddressOf());
	}

	// create the blend state
	{
		D3D11_BLEND_DESC blendDesc = {};

		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = false;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());
	}

	// create the shaders
	m_device->CreateVertexShader(g_debugLineVS, sizeof(g_debugLineVS), nullptr, m_vertexShader.GetAddressOf());
	m_device->CreatePixelShader(g_debugLinePS, sizeof(g_debugLinePS), nullptr, m_pixelShader.GetAddressOf());

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(Matrix44);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, NULL, m_constantBuffer.GetAddressOf());
	}
}

void DebugLineRenderD3D11::addLine(const Vec3& p, const Vec3& q, const Vec4& color)
{
	Vertex v = { p, color };
	Vertex w = { q, color };

	m_queued.push_back(v);
	m_queued.push_back(w);
}

void DebugLineRenderD3D11::flush(const Matrix44& viewProj)
{
	if (m_queued.empty())
		return;

	// recreate vertex buffer if not big enough for queued lines
	if (m_vertexBufferSize < int(m_queued.size()))
	{
		m_vertexBuffer = nullptr;

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage            = D3D11_USAGE_DYNAMIC;
		bufferDesc.ByteWidth        = UINT(sizeof(Vertex)*m_queued.size());
		bufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags        = 0;

		m_device->CreateBuffer(&bufferDesc, 0, &m_vertexBuffer);				

		m_vertexBufferSize = int(m_queued.size());
	}
		
	// update vertex buffer
	D3D11_MAPPED_SUBRESOURCE res;
	m_context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	memcpy(res.pData, &m_queued[0], sizeof(Vertex)*m_queued.size());
	m_context->Unmap(m_vertexBuffer.Get(), 0);
		
	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			memcpy(mappedResource.pData, &viewProj, sizeof(viewProj));
			m_context->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	// configure for line renderering
	m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0u);
	m_context->GSSetShader(nullptr, nullptr, 0u);
	m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0u);

	m_context->IASetInputLayout(m_inputLayout.Get());
	m_context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);

	m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	m_context->RSSetState(m_rasterizerState.Get());

	UINT vertexStride = sizeof(Vertex);
	UINT offset = 0u;
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &offset);

	// kick
	m_context->Draw(UINT(m_queued.size()), 0);

	// empty queue
	m_queued.resize(0);

}
