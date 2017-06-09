/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include <DirectXMath.h>

#include "shadowMapD3D11.h"
#include "../include/NvFlex.h"
#include "../d3d/renderParamsD3D.h"
#include "shaders.h"

#include <wrl.h>
using namespace Microsoft::WRL;

// vertex buffers for diffuse particles
struct DiffuseRenderBuffersD3D11
{
	DiffuseRenderBuffersD3D11() :
		m_positionsBuf(nullptr),
		m_velocitiesBuf(nullptr)
	{
		m_numParticles = 0;
	}

	~DiffuseRenderBuffersD3D11()
	{
		if (m_numParticles > 0)
		{
			NvFlexUnregisterD3DBuffer(m_positionsBuf);
			NvFlexUnregisterD3DBuffer(m_velocitiesBuf);
		}
	}

	int m_numParticles;

	ComPtr<ID3D11Buffer> m_positions;
	ComPtr<ID3D11Buffer> m_velocities;
	
	NvFlexBuffer* m_positionsBuf;
	NvFlexBuffer* m_velocitiesBuf;
};

struct DiffuseRendererD3D11
{
	void init(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	
	void draw(const DiffuseDrawParamsD3D* params, ID3D11Buffer* positions, ID3D11Buffer* velocities, int numParticles);

	DiffuseRendererD3D11():
		m_device(nullptr),
		m_deviceContext(nullptr)
	{}

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;

	ComPtr<ID3D11InputLayout> m_inputLayout;

	ComPtr<ID3D11VertexShader> m_diffuseVs;
	ComPtr<ID3D11GeometryShader> m_diffuseGs;
	ComPtr<ID3D11PixelShader> m_diffusePs;

	ComPtr<ID3D11Buffer> m_constantBuffer;
	ComPtr<ID3D11RasterizerState> m_rasterizerState;

	ComPtr<ID3D11BlendState> m_blendState;
	ComPtr<ID3D11DepthStencilState> m_depthStencilState;
};
