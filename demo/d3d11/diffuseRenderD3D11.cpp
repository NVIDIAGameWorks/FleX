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

#include "diffuseRenderD3D11.h"

#include "../d3d/shaders/diffuseVS.hlsl.h"
#include "../d3d/shaders/diffuseGS.hlsl.h"
#include "../d3d/shaders/diffusePS.hlsl.h"

#include "../d3d/shaderCommonD3D.h"

void DiffuseRendererD3D11::init(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_device = device;
	m_deviceContext = context;

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 2, g_diffuseVS, sizeof(g_diffuseVS), &m_inputLayout);
	}

	// create the shaders
	m_device->CreateVertexShader(g_diffuseVS, sizeof(g_diffuseVS), nullptr, &m_diffuseVs);
	m_device->CreateGeometryShader(g_diffuseGS, sizeof(g_diffuseGS), nullptr, &m_diffuseGs);
	m_device->CreatePixelShader(g_diffusePS, sizeof(g_diffusePS), nullptr, &m_diffusePs);

	{
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_device->CreateBlendState(&blendDesc, &m_blendState);
	}

	{
		D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
		depthStateDesc.DepthEnable = TRUE;	
		depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		device->CreateDepthStencilState(&depthStateDesc, &m_depthStencilState);
	}

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(Hlsl::DiffuseShaderConst); // 64 * sizeof(float);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, nullptr, &m_constantBuffer);
	}

	// create the rastersizer state
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.FrontCounterClockwise = TRUE;	// This is non-default
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.f;
		desc.SlopeScaledDepthBias = 0.f;
		desc.DepthClipEnable = TRUE;
		desc.ScissorEnable = FALSE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;

		m_device->CreateRasterizerState(&desc, &m_rasterizerState);
	}
}

void DiffuseRendererD3D11::draw(const DiffuseDrawParamsD3D* params, ID3D11Buffer* positions, ID3D11Buffer* velocities, int numParticles)
{
	using namespace DirectX;
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = { 0 };
		if (SUCCEEDED(deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::DiffuseShaderConst constBuf;
			RenderParamsUtilD3D::calcDiffuseConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::DiffuseShaderConst));
			deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	deviceContext->VSSetShader(m_diffuseVs.Get(), nullptr, 0u);
	deviceContext->GSSetShader(m_diffuseGs.Get(), nullptr, 0u);
	deviceContext->PSSetShader(m_diffusePs.Get(), nullptr, 0u);

	if (params->shadowMap)
	{
		ShadowMapD3D11* shadowMap = (ShadowMapD3D11*)params->shadowMap;
		
		ID3D11ShaderResourceView* srvs[1] = { shadowMap->m_depthSrv.Get() };
		deviceContext->PSSetShaderResources(0, 1, srvs);
		ID3D11SamplerState* samps[1] = { shadowMap->m_linearSampler.Get() };
		deviceContext->PSSetSamplers(0, 1, samps);
	}

	deviceContext->IASetInputLayout(m_inputLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->GSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	ID3D11Buffer* vertexBuffers[2] = 
	{
		positions,
		velocities
	};

	unsigned int vertexBufferStrides[2] =
	{
		sizeof(float4),
		sizeof(float4),
	};

	unsigned int vertexBufferOffsets[2] = { 0 };

	deviceContext->IASetVertexBuffers(0, 2, vertexBuffers, vertexBufferStrides, vertexBufferOffsets);
	deviceContext->IASetIndexBuffer(NV_NULL, DXGI_FORMAT_R32_UINT, 0u);

	deviceContext->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);
	deviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerState.Get());
	}
	
	deviceContext->Draw(numParticles, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}

	deviceContext->GSSetShader(nullptr, nullptr, 0u);
	deviceContext->OMSetDepthStencilState(nullptr, 0u);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}
