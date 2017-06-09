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

#include "meshRenderD3D11.h"
#include "appD3d11Ctx.h"


#include "../d3d/shaders/meshVS.hlsl.h"
#include "../d3d/shaders/meshPS.hlsl.h"
#include "../d3d/shaders/meshShadowPS.hlsl.h"

#include "../d3d/shaderCommonD3D.h"

#include "shadowMapD3D11.h"

// Make async compute benchmark shader have a unique name
namespace AsyncComputeBench
{
#include "../d3d/shaders/meshAsyncComputeBenchPS.hlsl.h"
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! GpuMeshD3D11 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

GpuMeshD3D11::GpuMeshD3D11(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
		: m_device(device)
		, m_deviceContext(deviceContext)
		, m_numFaces(0)
		, m_numVertices(0)
		, m_maxVertices(0)
		, m_maxFaces(0)
{
}

void GpuMeshD3D11::resize(int numVertices, int numFaces)
{
	{
		// vertex buffers
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(Vec3)*numVertices;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, NULL, m_positionBuffer.ReleaseAndGetAddressOf());
		m_device->CreateBuffer(&bufDesc, NULL, m_normalBuffer.ReleaseAndGetAddressOf());

		bufDesc.ByteWidth = sizeof(Vec2)*numVertices;
		m_device->CreateBuffer(&bufDesc, NULL, m_texcoordBuffer.ReleaseAndGetAddressOf());

		bufDesc.ByteWidth = sizeof(Vec4)*numVertices;
		m_device->CreateBuffer(&bufDesc, NULL, m_colorBuffer.ReleaseAndGetAddressOf());
	}

	{
		// index buffer
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(int)*numFaces * 3;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;

		m_device->CreateBuffer(&bufDesc, NULL, &m_indexBuffer);
	}

	m_maxVertices = numVertices;
	m_maxFaces = numFaces;
}

void GpuMeshD3D11::updateData(const Vec3* positions, const Vec3* normals, const Vec2* texcoords, const Vec4* colors, const int* indices, int numVertices, int numFaces)
{
	if (numVertices > m_maxVertices || numFaces > m_maxFaces)
	{
		resize(numVertices, numFaces);
	}

	D3D11_MAPPED_SUBRESOURCE res;

	// vertices
	if (positions)
	{
		m_deviceContext->Map(m_positionBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, positions, sizeof(Vec3)*numVertices);
		m_deviceContext->Unmap(m_positionBuffer.Get(), 0);
	}

	if (normals)
	{
		m_deviceContext->Map(m_normalBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, normals, sizeof(Vec3)*numVertices);
		m_deviceContext->Unmap(m_normalBuffer.Get(), 0);
	}

	if (texcoords)
	{
		m_deviceContext->Map(m_texcoordBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, texcoords, sizeof(Vec2)*numVertices);
		m_deviceContext->Unmap(m_texcoordBuffer.Get(), 0);
	}

	if (colors)
	{
		m_deviceContext->Map(m_colorBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, colors, sizeof(Vec4)*numVertices);
		m_deviceContext->Unmap(m_colorBuffer.Get(), 0);
	}

	// indices
	if (indices)
	{
		m_deviceContext->Map(m_indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, indices, sizeof(int)*numFaces * 3);
		m_deviceContext->Unmap(m_indexBuffer.Get(), 0);
	}

	m_numVertices = numVertices;
	m_numFaces = numFaces;
}

void GpuMeshD3D11::updateData(const Vec4* positions, const Vec4* normals, const Vec2* texcoords, const Vec4* colors, const int* indices, int numVertices, int numFaces)
{
	if (numVertices > m_maxVertices || numFaces > m_maxFaces)
	{
		resize(numVertices, numFaces);
	}

	D3D11_MAPPED_SUBRESOURCE res;

	// vertices
	if (positions)
	{
		m_deviceContext->Map(m_positionBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		for (int i = 0; i < numVertices; ++i)
			((Vec3*)res.pData)[i] = Vec3(positions[i]);
		m_deviceContext->Unmap(m_positionBuffer.Get(), 0);
	}

	if (normals)
	{
		m_deviceContext->Map(m_normalBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		for (int i = 0; i < numVertices; ++i)
			((Vec3*)res.pData)[i] = Vec3(normals[i]);
		m_deviceContext->Unmap(m_normalBuffer.Get(), 0);
	}

	if (texcoords)
	{
		m_deviceContext->Map(m_texcoordBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, texcoords, sizeof(Vec2)*numVertices);
		m_deviceContext->Unmap(m_texcoordBuffer.Get(), 0);
	}

	if (colors)
	{
		m_deviceContext->Map(m_colorBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, colors, sizeof(Vec4)*numVertices);
		m_deviceContext->Unmap(m_colorBuffer.Get(), 0);
	}

	// indices
	if (indices)
	{
		m_deviceContext->Map(m_indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, indices, sizeof(int)*numFaces * 3);
		m_deviceContext->Unmap(m_indexBuffer.Get(), 0);
	}

	m_numVertices = numVertices;
	m_numFaces = numFaces;
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! MeshRendererD3D11 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

void MeshRendererD3D11::init(ID3D11Device* device, ID3D11DeviceContext* context, bool asyncComputeBenchmark)
{
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

	if (asyncComputeBenchmark)
	{
		m_device->CreatePixelShader(AsyncComputeBench::g_meshPS, sizeof(AsyncComputeBench::g_meshPS), nullptr, &m_meshPs);
	}
	else
	{
		m_device->CreatePixelShader(g_meshPS, sizeof(g_meshPS), nullptr, &m_meshPs);
	}

	m_device->CreateVertexShader(g_meshVS, sizeof(g_meshVS), nullptr, &m_meshVs);
	m_device->CreatePixelShader(g_meshPS_Shadow, sizeof(g_meshPS_Shadow), nullptr, &m_meshShadowPs);

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(Hlsl::MeshShaderConst);
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

			m_device->CreateRasterizerState(&desc, &m_rasterizerState[i][j]);
		}
	}
}

void MeshRendererD3D11::draw(const GpuMeshD3D11* mesh, const MeshDrawParamsD3D* params)
{
	using namespace DirectX;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (SUCCEEDED(m_deviceContext->Map(m_constantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			Hlsl::MeshShaderConst constBuf;
			RenderParamsUtilD3D::calcMeshConstantBuffer(*params, constBuf);
			memcpy(mappedResource.pData, &constBuf, sizeof(Hlsl::MeshShaderConst));
			m_deviceContext->Unmap(m_constantBuffer.Get(), 0u);
		}
	}

	m_deviceContext->VSSetShader(m_meshVs.Get(), nullptr, 0u);
	m_deviceContext->GSSetShader(nullptr, nullptr, 0u);

	switch (params->renderStage)
	{
	case MESH_DRAW_SHADOW:
	{
		m_deviceContext->PSSetShader(m_meshShadowPs.Get(), nullptr, 0u);
		break;
	}
	case MESH_DRAW_LIGHT:
	{
		m_deviceContext->PSSetShader(m_meshPs.Get(), nullptr, 0u);

		ShadowMapD3D11* shadowMap = (ShadowMapD3D11*)params->shadowMap;
		ID3D11ShaderResourceView* srvs[1] = { shadowMap->m_depthSrv.Get() };
		m_deviceContext->PSSetShaderResources(0, 1, srvs);
		ID3D11SamplerState* sampStates[1] = { shadowMap->m_linearSampler.Get() };
		m_deviceContext->PSSetSamplers(0, 1, sampStates);
		break;
	}
	default:
		assert(false);
		break;
	}

	m_deviceContext->IASetInputLayout(m_inputLayout.Get());
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_deviceContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_deviceContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

	ID3D11Buffer* vertexBuffers[4] =
	{
		mesh->m_positionBuffer.Get(),
		mesh->m_normalBuffer.Get(),
		mesh->m_texcoordBuffer.Get(),
		mesh->m_colorBuffer.Get(),
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
	m_deviceContext->IASetIndexBuffer(mesh->m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		m_deviceContext->RSSetState(m_rasterizerState[params->renderMode][params->cullMode].Get());
	}
	
	m_deviceContext->DrawIndexed((UINT)mesh->m_numFaces*3, 0, 0);

	if (depthSign < 0.f)
	{
		m_deviceContext->RSSetState(nullptr);
	}	
}
