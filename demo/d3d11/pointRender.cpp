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

#include "pointRender.h"

#include "shaders/pointVS.hlsl.h"
#include "shaders/pointGS.hlsl.h"
#include "shaders/pointPS.hlsl.h"

#define EXCLUDE_SHADER_STRUCTS 1
#include "shaders/shaderCommon.h"


void PointRenderer::Init(ID3D11Device* device, ID3D11DeviceContext* context)
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
	m_device->CreateVertexShader(g_pointVS, sizeof(g_pointVS), nullptr, &m_pointVS);
	m_device->CreateGeometryShader(g_pointGS, sizeof(g_pointGS), nullptr, &m_pointGS);
	m_device->CreatePixelShader(g_pointPS, sizeof(g_pointPS), nullptr, &m_pointPS);

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(PointShaderConst); // 64 * sizeof(float);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, nullptr, &m_constantBuffer);
	}

	// create the rastersizer state
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

			m_device->CreateRasterizerState(&desc, &m_rasterizerStateRH[i][j]);
		}
	}
}

void PointRenderer::Destroy()
{
	COMRelease(m_inputLayout);
	COMRelease(m_pointVS);
	COMRelease(m_pointGS);
	COMRelease(m_pointPS);
	COMRelease(m_constantBuffer);

	for (int i = 0; i < NUM_POINT_RENDER_MODES; i++)
		for (int j = 0; j < NUM_POINT_CULL_MODES; j++)
			COMRelease(m_rasterizerStateRH[i][j]);
}



void PointRenderer::Draw(const PointDrawParams* params, ID3D11Buffer* positions, ID3D11Buffer* colors, ID3D11Buffer* indices, int numParticles, int offset)
{
	using namespace DirectX;

	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == deviceContext->Map(m_constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			PointShaderConst cParams;

			cParams.modelview = (float4x4&)XMMatrixMultiply(params->model, params->view);
			cParams.projection = (float4x4&)params->projection;

			cParams.pointRadius = params->pointRadius;
			cParams.pointScale = params->pointScale;
			cParams.spotMin = params->spotMin;
			cParams.spotMax = params->spotMax;

			cParams.lightTransform = (float4x4&)params->lightTransform;
			cParams.lightPos = params->lightPos;
			cParams.lightDir = params->lightDir;

			for (int i = 0; i < 8; i++)
				cParams.colors[i] = params->colors[i];

			memcpy(cParams.shadowTaps, params->shadowTaps, sizeof(cParams.shadowTaps));

			cParams.mode = params->mode;

			memcpy(mappedResource.pData, &cParams, sizeof(PointShaderConst));

			deviceContext->Unmap(m_constantBuffer, 0u);
		}
	}

	deviceContext->VSSetShader(m_pointVS, nullptr, 0u);
	deviceContext->GSSetShader(m_pointGS, nullptr, 0u);
	deviceContext->PSSetShader(m_pointPS, nullptr, 0u);

	if (params->shadowMap)
	{
		ShadowMap* shadowMap = (ShadowMap*)params->shadowMap;
		if (params->renderStage == POINT_DRAW_SHADOW)
		{
			ID3D11ShaderResourceView* ppSRV[1] = { nullptr };
			deviceContext->PSSetShaderResources(0, 1, ppSRV);
			ID3D11SamplerState* ppSS[1] = { shadowMap->m_linearSampler.Get() };
			deviceContext->PSSetSamplers(0, 1, ppSS);
		}
		else
		{
			ID3D11ShaderResourceView* ppSRV[1] = { shadowMap->m_depthSrv.Get() };
			deviceContext->PSSetShaderResources(0, 1, ppSRV);
			ID3D11SamplerState* ppSS[1] = { shadowMap->m_linearSampler.Get() };
			deviceContext->PSSetSamplers(0, 1, ppSS);
		}
	}

	deviceContext->IASetInputLayout(m_inputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	deviceContext->GSSetConstantBuffers(0, 1, &m_constantBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &m_constantBuffer);

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
		deviceContext->RSSetState(m_rasterizerStateRH[params->renderMode][params->cullMode]);
	}
	
	deviceContext->DrawIndexed(numParticles, offset, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}
