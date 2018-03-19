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

#include "fluidRenderD3D11.h"

#include "../d3d/shaders/pointThicknessVS.hlsl.h"
#include "../d3d/shaders/pointThicknessGS.hlsl.h"
#include "../d3d/shaders/pointThicknessPS.hlsl.h"
#include "../d3d/shaders/ellipsoidDepthVS.hlsl.h"
#include "../d3d/shaders/ellipsoidDepthGS.hlsl.h"
#include "../d3d/shaders/ellipsoidDepthPS.hlsl.h"
#include "../d3d/shaders/passThroughVS.hlsl.h"
#include "../d3d/shaders/blurDepthPS.hlsl.h"
#include "../d3d/shaders/compositePS.hlsl.h"

#include "../d3d/shaderCommonD3D.h"

#include "renderTargetD3D11.h"
#include "shadowMapD3D11.h"

#include "shaders.h"

typedef DirectX::XMFLOAT2 float2;

using namespace DirectX;

struct PassthroughVertex
{
	float3 position;
	float3 normal;
	float2 texcoords;
	float4 color;
};

void FluidRendererD3D11::init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height)
{
	m_sceneWidth = width;
	m_sceneHeight = height;

	m_thicknessTexture.init(device, width, height);
	m_depthTexture.init(device, width, height);
	m_depthSmoothTexture.init(device, width, height, false);

	m_device = device;
	m_deviceContext = context;

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "U", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "W", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 4, g_pointThicknessVS, sizeof(g_pointThicknessVS), &m_pointThicknessLayout);
	}

	// create the shaders
	m_device->CreateVertexShader(g_pointThicknessVS, sizeof(g_pointThicknessVS), nullptr, &m_pointThicknessVs);
	m_device->CreateGeometryShader(g_pointThicknessGS, sizeof(g_pointThicknessGS), nullptr, &m_pointThicknessGs);
	m_device->CreatePixelShader(g_pointThicknessPS, sizeof(g_pointThicknessPS), nullptr, &m_pointThicknessPs);
	
	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "U", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "W", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 4, g_ellipsoidDepthVS, sizeof(g_ellipsoidDepthVS), &m_ellipsoidDepthLayout);
	}

	// create the shaders
	m_device->CreateVertexShader(g_ellipsoidDepthVS, sizeof(g_ellipsoidDepthVS), nullptr, &m_ellipsoidDepthVs);
	m_device->CreateGeometryShader(g_ellipsoidDepthGS, sizeof(g_ellipsoidDepthGS), nullptr, &m_ellipsoidDepthGs);
	m_device->CreatePixelShader(g_ellipsoidDepthPS, sizeof(g_ellipsoidDepthPS), nullptr, &m_ellipsoidDepthPs);

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		m_device->CreateInputLayout(inputElementDescs, 4, g_passThroughVS, sizeof(g_passThroughVS), &m_passThroughLayout);
	}

	// pass through shader
	m_device->CreateVertexShader(g_passThroughVS, sizeof(g_passThroughVS), nullptr, &m_passThroughVs);

	// full screen pixel shaders
	m_device->CreatePixelShader(g_blurDepthPS, sizeof(g_blurDepthPS), nullptr, &m_blurDepthPs);
	m_device->CreatePixelShader(g_compositePS, sizeof(g_compositePS), nullptr, &m_compositePs);

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(Hlsl::FluidShaderConst); // 64 * sizeof(float);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, nullptr, &m_constantBuffer);
	}

	// create the rastersizer state
	for (int i = 0; i < NUM_FLUID_RENDER_MODES; i++)
	{
		for (int j = 0; j < NUM_FLUID_CULL_MODES; j++)
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

	_createScreenQuad();
}

void FluidRendererD3D11::_createScreenQuad()
{
	// create an index buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = 4*sizeof(UINT);
		bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.CPUAccessFlags = DXGI_CPU_ACCESS_NONE;
		bufDesc.MiscFlags = 0;

		unsigned int quad_indices[4] = { 0, 1, 3, 2 };
		
		D3D11_SUBRESOURCE_DATA data = { 0 };
		data.pSysMem = quad_indices;

		m_device->CreateBuffer(&bufDesc, &data, &m_quadIndexBuffer);
	}

	// create a vertex buffer
	{
		
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = 4*sizeof(PassthroughVertex);
		bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = DXGI_CPU_ACCESS_NONE;
		bufDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data = { 0 };


		PassthroughVertex vertices[4] =
		{
		 	{ {-1.0f, -1.0f, 0.0f},  {0, 1, 0}, {0.0f, 0.0f}, {1, 1, 1, 1}},
			{ { 1.0f, -1.0f, 0.0f},  {0, 1, 0}, {1.0f, 0.0f}, {1, 1, 1, 1}},
			{ { 1.0f,  1.0f, 0.0f},  {0, 1, 0}, {1.0f, 1.0f}, {1, 1, 1, 1}},
			{ {-1.0f,  1.0f, 0.0f},  {0, 1, 0}, {0.0f, 1.0f}, {1, 1, 1, 1}},
		};

		data.pSysMem = vertices;

		m_device->CreateBuffer(&bufDesc, &data, &m_quadVertexBuffer);
	}
}

void FluidRendererD3D11::drawThickness(const FluidDrawParamsD3D* params, const FluidRenderBuffersD3D11* buffers)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{

		D3D11_BUFFER_DESC desc;
		m_constantBuffer->GetDesc(&desc);

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::FluidShaderConst constBuf;
			RenderParamsUtilD3D::calcFluidConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::FluidShaderConst));
			deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	deviceContext->VSSetShader(m_pointThicknessVs.Get(), nullptr, 0u);
	deviceContext->GSSetShader(m_pointThicknessGs.Get(), nullptr, 0u);
	deviceContext->PSSetShader(m_pointThicknessPs.Get(), nullptr, 0u);

	deviceContext->IASetInputLayout(m_pointThicknessLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->GSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	ID3D11Buffer* vertexBuffers[4] =
	{
		buffers->m_positions.Get(),
		buffers->m_anisotropiesArr[0].Get(),
		buffers->m_anisotropiesArr[1].Get(),
		buffers->m_anisotropiesArr[2].Get()
	};

	unsigned int vertexBufferStrides[4] =
	{
		sizeof(float4),
		sizeof(float4),
		sizeof(float4),
		sizeof(float4)
	};

	unsigned int vertexBufferOffsets[4] = { 0 };

	deviceContext->IASetVertexBuffers(0, 4, vertexBuffers, vertexBufferStrides, vertexBufferOffsets);
	deviceContext->IASetIndexBuffer(buffers->m_indices.Get(), DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerState[params->renderMode][params->cullMode].Get());
	}

	deviceContext->DrawIndexed(params->n, params->offset, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}

void FluidRendererD3D11::drawEllipsoids(const FluidDrawParamsD3D* params, const FluidRenderBuffersD3D11* buffers)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{

		D3D11_BUFFER_DESC desc;
		m_constantBuffer->GetDesc(&desc);

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::FluidShaderConst constBuf;
			RenderParamsUtilD3D::calcFluidConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::FluidShaderConst));
			deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	deviceContext->VSSetShader(m_ellipsoidDepthVs.Get(), nullptr, 0u);
	deviceContext->GSSetShader(m_ellipsoidDepthGs.Get(), nullptr, 0u);
	deviceContext->PSSetShader(m_ellipsoidDepthPs.Get(), nullptr, 0u);

	deviceContext->IASetInputLayout(m_ellipsoidDepthLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->GSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	ID3D11Buffer* vertexBuffers[4] =
	{
		buffers->m_positions.Get(),
		buffers->m_anisotropiesArr[0].Get(),
		buffers->m_anisotropiesArr[1].Get(),
		buffers->m_anisotropiesArr[2].Get()
	};

	unsigned int vertexBufferStrides[4] =
	{
		sizeof(float4),
		sizeof(float4),
		sizeof(float4),
		sizeof(float4)
	};

	unsigned int vertexBufferOffsets[4] = { 0 };

	deviceContext->IASetVertexBuffers(0, 4, vertexBuffers, vertexBufferStrides, vertexBufferOffsets);
	deviceContext->IASetIndexBuffer(buffers->m_indices.Get(), DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerState[params->renderMode][params->cullMode].Get());
	}

	deviceContext->DrawIndexed(params->n, params->offset, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}

void FluidRendererD3D11::drawBlurDepth(const FluidDrawParamsD3D* params)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::FluidShaderConst constBuf;
			RenderParamsUtilD3D::calcFluidConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::FluidShaderConst));
			deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	deviceContext->VSSetShader(m_passThroughVs.Get(), nullptr, 0u);
	deviceContext->GSSetShader(nullptr, nullptr, 0u);
	deviceContext->PSSetShader(m_blurDepthPs.Get(), nullptr, 0u);

	ID3D11ShaderResourceView* srvs[2] = { m_depthTexture.m_backSrv.Get(), m_thicknessTexture.m_backSrv.Get() };
	deviceContext->PSSetShaderResources(0, 2, srvs);

	ID3D11SamplerState* samps[1] = { m_thicknessTexture.m_linearSampler.Get() };
	deviceContext->PSSetSamplers(0, 1, samps);

	deviceContext->IASetInputLayout(m_passThroughLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	UINT vertexStride = sizeof(PassthroughVertex);
	UINT offset = 0u;
	deviceContext->IASetVertexBuffers(0, 1, m_quadVertexBuffer.GetAddressOf(), &vertexStride, &offset);
	deviceContext->IASetIndexBuffer(m_quadIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerState[params->renderMode][params->cullMode].Get());
	}

	deviceContext->DrawIndexed((UINT)4, 0, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}

void FluidRendererD3D11::drawComposite(const FluidDrawParamsD3D* params, ID3D11ShaderResourceView* sceneMap)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::FluidShaderConst constBuf;
			RenderParamsUtilD3D::calcFluidCompositeConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::FluidShaderConst));
			deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	deviceContext->VSSetShader(m_passThroughVs.Get(), nullptr, 0u);
	deviceContext->GSSetShader(nullptr, nullptr, 0u);
	deviceContext->PSSetShader(m_compositePs.Get(), nullptr, 0u);

	RenderTargetD3D11* depthMap = &m_depthSmoothTexture;
	ShadowMapD3D11* shadowMap = (ShadowMapD3D11*)params->shadowMap;

	ID3D11ShaderResourceView* srvs[3] = 
	{ 
		depthMap->m_backSrv.Get(),
		sceneMap,
		shadowMap->m_depthSrv.Get()

	};
	deviceContext->PSSetShaderResources(0, 3, srvs);

	ID3D11SamplerState* samps[2] = 
	{ 
		depthMap->m_linearSampler.Get() ,
		shadowMap->m_linearSampler.Get()
	};
	deviceContext->PSSetSamplers(0, 2, samps);

	deviceContext->IASetInputLayout(m_passThroughLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	UINT vertexStride = sizeof(PassthroughVertex);
	UINT offset = 0u;
	deviceContext->IASetVertexBuffers(0, 1, m_quadVertexBuffer.GetAddressOf(), &vertexStride, &offset);
	deviceContext->IASetIndexBuffer(m_quadIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerState[params->renderMode][params->cullMode].Get());
	}

	deviceContext->DrawIndexed((UINT)4, 0, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}

	// reset srvs
	ID3D11ShaderResourceView* zero[3] = { NULL, NULL, NULL };
	deviceContext->PSSetShaderResources(0, 3, zero);
}

