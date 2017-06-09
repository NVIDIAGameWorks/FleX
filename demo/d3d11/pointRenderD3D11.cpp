/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

//direct3d headers

#define NOMINMAX

#include <d3d11.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")

#include <math.h>

#include "appD3D11Ctx.h"

#include "shadowMapD3D11.h"

#include "pointRenderD3D11.h"

#include "../d3d/shaders/pointVS.hlsl.h"
#include "../d3d/shaders/pointGS.hlsl.h"
#include "../d3d/shaders/pointPS.hlsl.h"
#include "../d3d/shaders/pointShadowPS.hlsl.h"

#include "../d3d/shaderCommonD3D.h"

void PointRendererD3D11::init(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_device = device;
	m_deviceContext = context;

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "DENSITY", 0, DXGI_FORMAT_R32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "PHASE", 0, DXGI_FORMAT_R32_SINT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 3, g_pointVS, sizeof(g_pointVS), &m_inputLayout);
	}

	// create the shaders
	m_device->CreateVertexShader(g_pointVS, sizeof(g_pointVS), nullptr, &m_pointVs);
	m_device->CreateGeometryShader(g_pointGS, sizeof(g_pointGS), nullptr, &m_pointGs);
	m_device->CreatePixelShader(g_pointPS, sizeof(g_pointPS), nullptr, &m_pointPs);
	m_device->CreatePixelShader(g_pointShadowPS, sizeof(g_pointShadowPS), nullptr, &m_pointShadowPs);

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(Hlsl::PointShaderConst); // 64 * sizeof(float);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, nullptr, &m_constantBuffer);
	}

	// create the rasterizer state
	for (int i = 0; i < NUM_POINT_RENDER_MODES; i++)
	{
		for (int j = 0; j < NUM_POINT_CULL_MODES; j++)

		{
			D3D11_RASTERIZER_DESC desc = {};
			desc.FillMode = (D3D11_FILL_MODE)(D3D11_FILL_WIREFRAME + i);
			desc.CullMode = (D3D11_CULL_MODE)(D3D11_CULL_NONE + j);
			desc.FrontCounterClockwise = TRUE;	// This is non-default
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.f;
			desc.SlopeScaledDepthBias = 0.f;
			desc.DepthClipEnable = TRUE;
			desc.ScissorEnable = FALSE;
			desc.MultisampleEnable = FALSE;
			desc.AntialiasedLineEnable = FALSE;

			m_device->CreateRasterizerState(&desc, &m_rasterizerState[i][j]);
		}
	}
}

void PointRendererD3D11::draw(const PointDrawParamsD3D* params, ID3D11Buffer* positions, ID3D11Buffer* colors, ID3D11Buffer* indices, int numParticles, int offset)
{
	using namespace DirectX;

	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::PointShaderConst constBuf;
			RenderParamsUtilD3D::calcPointConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::PointShaderConst));
			deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	deviceContext->VSSetShader(m_pointVs.Get(), nullptr, 0u);
	deviceContext->GSSetShader(m_pointGs.Get(), nullptr, 0u);
	deviceContext->PSSetShader(m_pointPs.Get(), nullptr, 0u);

	if (params->shadowMap)
	{
		ShadowMapD3D11* shadowMap = (ShadowMapD3D11*)params->shadowMap;
		if (params->renderStage == POINT_DRAW_SHADOW)
		{
			ID3D11ShaderResourceView* srvs[1] = { nullptr };
			deviceContext->PSSetShaderResources(0, 1, srvs);
			ID3D11SamplerState* samps[1] = { shadowMap->m_linearSampler.Get() };
			deviceContext->PSSetSamplers(0, 1, samps);
			deviceContext->PSSetShader(m_pointShadowPs.Get(), nullptr, 0u);
		}
		else
		{
			ID3D11ShaderResourceView* srvs[1] = { shadowMap->m_depthSrv.Get() };
			deviceContext->PSSetShaderResources(0, 1, srvs);
			ID3D11SamplerState* samps[1] = { shadowMap->m_linearSampler.Get() };
			deviceContext->PSSetSamplers(0, 1, samps);
		}
	}

	deviceContext->IASetInputLayout(m_inputLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->GSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	ID3D11Buffer* vertexBuffers[3] = 
	{
		positions,
		colors,	// will be interpreted as density
		colors,	// will be interpreted as phase
	};

	unsigned int vertexBufferStrides[3] =
	{
		sizeof(float4),
		sizeof(float),
		sizeof(int)
	};

	unsigned int vertexBufferOffsets[3] = { 0 };

	deviceContext->IASetVertexBuffers(0, 3, vertexBuffers, vertexBufferStrides, vertexBufferOffsets);
	deviceContext->IASetIndexBuffer(indices, DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerState[params->renderMode][params->cullMode].Get());
	}
	
	deviceContext->DrawIndexed(numParticles, offset, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}
