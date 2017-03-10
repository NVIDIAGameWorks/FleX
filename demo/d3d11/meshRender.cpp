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

#include "meshRender.h"
#include "appD3d11Ctx.h"


#include "shaders/meshVS.hlsl.h"
#include "shaders/meshPS.hlsl.h"
#include "shaders/meshShadowPS.hlsl.h"

#define EXCLUDE_SHADER_STRUCTS 1
#include "shaders/shaderCommon.h"

#include "shadowMap.h"


void MeshRenderer::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
	for (int i = 0; i < NUM_MESH_RENDER_MODES; i++)
		for (int j = 0; j < NUM_MESH_CULL_MODES; j++)
			m_rasterizerStateRH[i][j] = nullptr;

	m_device = device;
	m_deviceContext = context;

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 4, g_meshVS, sizeof(g_meshVS), &m_inputLayout);
	}

	// create the shaders
	m_device->CreateVertexShader(g_meshVS, sizeof(g_meshVS), nullptr, &m_meshVS);
	m_device->CreatePixelShader(g_meshPS, sizeof(g_meshPS), nullptr, &m_meshPS);
	m_device->CreatePixelShader(g_meshPS_Shadow, sizeof(g_meshPS_Shadow), nullptr, &m_meshPS_Shadow);

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(MeshShaderConst);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, nullptr, &m_constantBuffer);
	}

	// create the rastersizer state
	for (int i = 0; i < NUM_MESH_RENDER_MODES; i++)
	{
		for (int j = 0; j < NUM_MESH_CULL_MODES; j++)

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

void MeshRenderer::Destroy()
{
	COMRelease(m_inputLayout);
	COMRelease(m_meshVS);
	COMRelease(m_meshPS);
	COMRelease(m_meshPS_Shadow);
	COMRelease(m_constantBuffer);

	for (int i = 0; i < NUM_MESH_RENDER_MODES; i++)
		for (int j = 0; j < NUM_MESH_CULL_MODES; j++)
			COMRelease(m_rasterizerStateRH[i][j]);
}


void MeshRenderer::Draw(const GpuMesh* mesh, const MeshDrawParams* params)
{
	using namespace DirectX;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == m_deviceContext->Map(m_constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			MeshShaderConst cParams;

			cParams.modelviewprojection = (float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params->model, params->view), params->projection));

			cParams.modelview = (float4x4&)XMMatrixMultiply(params->model, params->view);

			cParams.objectTransform = (float4x4&)params->objectTransform;
			cParams.lightTransform = (float4x4&)params->lightTransform;

			cParams.clipPlane = params->clipPlane;
			cParams.fogColor = params->fogColor;
			cParams.color = params->color;
			cParams.secondaryColor = params->secondaryColor;

			cParams.lightPos = params->lightPos;
			cParams.lightDir = params->lightDir;

			cParams.bias = params->bias;
			cParams.expand = params->expand;
			cParams.spotMin = params->spotMin;
			cParams.spotMax = params->spotMax;

			cParams.grid = params->grid;
			cParams.tex = params->tex;
			cParams.colorArray = params->colorArray;

			memcpy(cParams.shadowTaps, params->shadowTaps, sizeof(cParams.shadowTaps));

			memcpy(mappedResource.pData, &cParams, sizeof(MeshShaderConst));

			m_deviceContext->Unmap(m_constantBuffer, 0u);
		}
	}

	m_deviceContext->VSSetShader(m_meshVS, nullptr, 0u);
	m_deviceContext->GSSetShader(nullptr, nullptr, 0u);

	switch (params->renderStage)
	{
	case MESH_DRAW_SHADOW:
	{
		m_deviceContext->PSSetShader(m_meshPS_Shadow, nullptr, 0u);
		break;
	}
	case MESH_DRAW_LIGHT:
	{
		m_deviceContext->PSSetShader(m_meshPS, nullptr, 0u);

		ShadowMap* shadowMap = (ShadowMap*)params->shadowMap;
		ID3D11ShaderResourceView* ppSRV[1] = { shadowMap->m_depthSrv.Get() };
		m_deviceContext->PSSetShaderResources(0, 1, ppSRV);
		ID3D11SamplerState* ppSS[1] = { shadowMap->m_linearSampler.Get() };
		m_deviceContext->PSSetSamplers(0, 1, ppSS);
		break;
	}
	default:
		assert(false);
		break;
	}

	m_deviceContext->IASetInputLayout(m_inputLayout);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	m_deviceContext->PSSetConstantBuffers(0, 1, &m_constantBuffer);

	ID3D11Buffer* vertexBuffers[4] =
	{
		mesh->positionBuffer,
		mesh->normalBuffer,
		mesh->texcoordBuffer,
		mesh->colorBuffer,
	};

	unsigned int vertexBufferStrides[4] =
	{
		sizeof(Vec3),
		sizeof(Vec3),
		sizeof(Vec2),
		sizeof(Vec4)
	};

	unsigned int vertexBufferOffsets[4] = { 0, 0, 0, 0 };

	m_deviceContext->IASetVertexBuffers(0, 4, vertexBuffers, vertexBufferStrides, vertexBufferOffsets);
	m_deviceContext->IASetIndexBuffer(mesh->indexBuffer, DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		m_deviceContext->RSSetState(m_rasterizerStateRH[params->renderMode][params->cullMode]);
	}
	
	m_deviceContext->DrawIndexed((UINT)mesh->mNumFaces*3, 0, 0);

	if (depthSign < 0.f)
	{
		m_deviceContext->RSSetState(nullptr);
	}	
}
