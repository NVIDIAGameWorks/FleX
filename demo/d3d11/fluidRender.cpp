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

#include "fluidRender.h"

#include "shaders/ellipsoidDepthVS.hlsl.h"
#include "shaders/ellipsoidDepthGS.hlsl.h"
#include "shaders/ellipsoidDepthPS.hlsl.h"
#include "shaders/passThroughVS.hlsl.h"
#include "shaders/blurDepthPS.hlsl.h"
#include "shaders/compositePS.hlsl.h"

#define EXCLUDE_SHADER_STRUCTS 1
#include "shaders/shaderCommon.h"

#include "renderTarget.h"
#include "shadowMap.h"

#include "shaders.h"

typedef DirectX::XMFLOAT2 float2;

using namespace DirectX;


struct PassthroughVertex
{
	float x, y;
	float u, v;
};

void FluidRenderer::Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height)
{
	mSceneWidth = width;
	mSceneHeight = height;

	mDepthTex.Init(device, width, height);
	mDepthSmoothTex.Init(device, width, height, false);

	m_device = device;
	m_deviceContext = context;
	
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
	m_device->CreateVertexShader(g_ellipsoidDepthVS, sizeof(g_ellipsoidDepthVS), nullptr, &m_ellipsoidDepthVS);
	m_device->CreateGeometryShader(g_ellipsoidDepthGS, sizeof(g_ellipsoidDepthGS), nullptr, &m_ellipsoidDepthGS);
	m_device->CreatePixelShader(g_ellipsoidDepthPS, sizeof(g_ellipsoidDepthPS), nullptr, &m_ellipsoidDepthPS);

	// create the input layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		m_device->CreateInputLayout(inputElementDescs, 2, g_passThroughVS, sizeof(g_passThroughVS), &m_passThroughLayout);
	}

	// pass through shader
	m_device->CreateVertexShader(g_passThroughVS, sizeof(g_passThroughVS), nullptr, &m_passThroughVS);

	// full screen pixel shaders
	m_device->CreatePixelShader(g_blurDepthPS, sizeof(g_blurDepthPS), nullptr, &m_blurDepthPS);
	m_device->CreatePixelShader(g_compositePS, sizeof(g_compositePS), nullptr, &m_compositePS);

	// create a constant buffer
	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = sizeof(FluidShaderConst); // 64 * sizeof(float);
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

			m_device->CreateRasterizerState(&desc, &m_rasterizerStateRH[i][j]);
		}
	}

	CreateScreenQuad();
}

void FluidRenderer::Destroy()
{
	COMRelease(m_ellipsoidDepthLayout);
	COMRelease(m_ellipsoidDepthVS);
	COMRelease(m_ellipsoidDepthGS);
	COMRelease(m_ellipsoidDepthPS);

	COMRelease(m_passThroughLayout);
	COMRelease(m_passThroughVS);

	COMRelease(m_blurDepthPS);
	COMRelease(m_compositePS);

	COMRelease(m_constantBuffer);

	for (int i = 0; i < NUM_FLUID_RENDER_MODES; i++)
		for (int j = 0; j < NUM_FLUID_CULL_MODES; j++)
			COMRelease(m_rasterizerStateRH[i][j]);

	COMRelease(m_quadVertexBuffer);
	COMRelease(m_quadIndexBuffer);
}

void FluidRenderer::CreateScreenQuad()
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


		float4 vertices[4] =
		{
			float4(-1.0f, -1.0f, 0.0f, 0.0f),
			float4(1.0f, -1.0f, 1.0f, 0.0f),
			float4(1.0f, 1.0f, 1.0f, 1.0f),
			float4(-1.0f, 1.0f, 0.0f, 1.0f),
		};

		data.pSysMem = vertices;

		m_device->CreateBuffer(&bufDesc, &data, &m_quadVertexBuffer);
	}
}

void FluidRenderer::DrawEllipsoids(const FluidDrawParams* params, const FluidRenderBuffers* buffers)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{

		D3D11_BUFFER_DESC desc;
		m_constantBuffer->GetDesc(&desc);

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == deviceContext->Map(m_constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			FluidShaderConst cParams;

			cParams.modelviewprojection = (float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params->model, params->view), params->projection));
			cParams.projection = (float4x4&)params->projection;
			cParams.modelview_inverse = (float4x4&)XMMatrixInverse(nullptr, XMMatrixMultiply(params->model, params->view));
			cParams.projection_inverse = (float4x4&)XMMatrixInverse(nullptr, params->projection);

			//cParams.invTexScale = invTexScale;
			//cParams.invProjection = invProjection;
			cParams.invViewport = params->invViewport;
			

			cParams.blurRadiusWorld = params->blurRadiusWorld;
			cParams.blurScale = params->blurScale;
			cParams.blurFalloff = params->blurFalloff;
			cParams.debug = params->debug;

			memcpy(mappedResource.pData, &cParams, sizeof(FluidShaderConst));

			deviceContext->Unmap(m_constantBuffer, 0u);
		}
	}

	deviceContext->VSSetShader(m_ellipsoidDepthVS, nullptr, 0u);
	deviceContext->GSSetShader(m_ellipsoidDepthGS, nullptr, 0u);
	deviceContext->PSSetShader(m_ellipsoidDepthPS, nullptr, 0u);

	deviceContext->IASetInputLayout(m_ellipsoidDepthLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	deviceContext->GSSetConstantBuffers(0, 1, &m_constantBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &m_constantBuffer);

	ID3D11Buffer* vertexBuffers[4] = 
	{
		buffers->mPositionVBO,
		buffers->mAnisotropyVBO[0],
		buffers->mAnisotropyVBO[1],
		buffers->mAnisotropyVBO[2]
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
	deviceContext->IASetIndexBuffer(buffers->mIndices, DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerStateRH[params->renderMode][params->cullMode]);
	}
	
	deviceContext->DrawIndexed(params->n, params->offset, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}

void FluidRenderer::DrawBlurDepth(const FluidDrawParams* params)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == deviceContext->Map(m_constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			FluidShaderConst cParams;

			cParams.modelviewprojection = (float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params->model, params->view), params->projection));
			cParams.projection = (float4x4&)params->projection;
			cParams.modelview_inverse = (float4x4&)XMMatrixInverse(nullptr, XMMatrixMultiply(params->model, params->view));
			cParams.projection_inverse = (float4x4&)XMMatrixInverse(nullptr, params->projection);

			//cParams.invTexScale = params->invTexScale;
			//cParams.invViewport = params->invViewport;
			//cParams.invProjection = params->invProjection;

			cParams.blurRadiusWorld = params->blurRadiusWorld;
			cParams.blurScale = params->blurScale;
			cParams.blurFalloff = params->blurFalloff;
			cParams.debug = params->debug;

			memcpy(mappedResource.pData, &cParams, sizeof(FluidShaderConst));

			deviceContext->Unmap(m_constantBuffer, 0u);
		}
	}

	deviceContext->VSSetShader(m_passThroughVS, nullptr, 0u);
	deviceContext->GSSetShader(nullptr, nullptr, 0u);
	deviceContext->PSSetShader(m_blurDepthPS, nullptr, 0u);

	ID3D11ShaderResourceView* ppSRV[1] = { mDepthTex.m_backSrv.Get() };
	deviceContext->PSSetShaderResources(0, 1, ppSRV);

	deviceContext->IASetInputLayout(m_passThroughLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &m_constantBuffer);

	UINT vertexStride = sizeof(PassthroughVertex);
	UINT offset = 0u;
	deviceContext->IASetVertexBuffers(0, 1, &m_quadVertexBuffer, &vertexStride, &offset);
	deviceContext->IASetIndexBuffer(m_quadIndexBuffer, DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerStateRH[params->renderMode][params->cullMode]);
	}

	deviceContext->DrawIndexed((UINT)4, 0, 0);

	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(nullptr);
	}
}

void FluidRenderer::DrawComposite(const FluidDrawParams* params, ID3D11ShaderResourceView* sceneMap)
{
	ID3D11DeviceContext* deviceContext = m_deviceContext;

	// update constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (S_OK == deviceContext->Map(m_constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
		{
			FluidShaderConst cParams;

			cParams.modelviewprojection = (float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params->model, params->view), params->projection));
			cParams.modelview = (float4x4&)XMMatrixMultiply(params->model, params->view);
			cParams.projection = (float4x4&)params->projection;
			cParams.modelview_inverse = (float4x4&)XMMatrixInverse(nullptr, XMMatrixMultiply(params->model, params->view));
			cParams.projection_inverse = (float4x4&)XMMatrixInverse(nullptr, params->projection);
			
			cParams.lightTransform = (float4x4&)params->lightTransform;

			cParams.invTexScale = params->invTexScale;

			//cParams.invViewport = params->invViewport;
			//cParams.invProjection = params->invProjection;

			cParams.blurRadiusWorld = params->blurRadiusWorld;
			cParams.blurScale = params->blurScale;
			cParams.blurFalloff = params->blurFalloff;
			cParams.debug = params->debug;

			cParams.clipPosToEye = params->clipPosToEye;
			cParams.color = params->color;
			cParams.ior = params->ior;
			cParams.spotMin = params->spotMin;
			cParams.spotMax = params->spotMax;

			cParams.lightPos = params->lightPos;
			cParams.lightDir = params->lightDir;

			memcpy(mappedResource.pData, &cParams, sizeof(FluidShaderConst));

			deviceContext->Unmap(m_constantBuffer, 0u);

			const float2 taps[] =
			{
				float2(-0.326212f,-0.40581f), float2(-0.840144f,-0.07358f),
				float2(-0.695914f,0.457137f), float2(-0.203345f,0.620716f),
				float2(0.96234f,-0.194983f), float2(0.473434f,-0.480026f),
				float2(0.519456f,0.767022f), float2(0.185461f,-0.893124f),
				float2(0.507431f,0.064425f), float2(0.89642f,0.412458f),
				float2(-0.32194f,-0.932615f), float2(-0.791559f,-0.59771f)
			};
			memcpy(cParams.shadowTaps, taps, sizeof(taps));
		}
	}

	deviceContext->VSSetShader(m_passThroughVS, nullptr, 0u);
	deviceContext->GSSetShader(nullptr, nullptr, 0u);
	deviceContext->PSSetShader(m_compositePS, nullptr, 0u);

	
	RenderTarget* depthMap = &mDepthSmoothTex;
	ShadowMap* shadowMap = (ShadowMap*)params->shadowMap;

	ID3D11ShaderResourceView* ppSRV[3] = 
	{ 
		depthMap->m_backSrv.Get(),
		sceneMap,
		shadowMap->m_depthSrv.Get()

	};
	deviceContext->PSSetShaderResources(0, 3, ppSRV);

	ID3D11SamplerState* ppSS[2] = 
	{ 
		depthMap->m_linearSampler.Get() ,
		shadowMap->m_linearSampler.Get()
	};
	deviceContext->PSSetSamplers(0, 2, ppSS);	


	deviceContext->IASetInputLayout(m_passThroughLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &m_constantBuffer);

	UINT vertexStride = sizeof(PassthroughVertex);
	UINT offset = 0u;
	deviceContext->IASetVertexBuffers(0, 1, &m_quadVertexBuffer, &vertexStride, &offset);
	deviceContext->IASetIndexBuffer(m_quadIndexBuffer, DXGI_FORMAT_R32_UINT, 0u);

	float depthSign = DirectX::XMVectorGetW(params->projection.r[2]);
	if (depthSign < 0.f)
	{
		deviceContext->RSSetState(m_rasterizerStateRH[params->renderMode][params->cullMode]);
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

