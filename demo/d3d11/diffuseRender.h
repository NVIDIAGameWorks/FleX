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

#include "shadowMap.h"


typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;

struct DiffuseDrawParams
{
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX model;

	float diffuseRadius;  // point size in world space
	float diffuseScale;   // scale to calculate size in pixels
	float spotMin;
	float spotMax;
	float motionScale;

	DirectX::XMMATRIX lightTransform;
	float3 lightPos;
	float3 lightDir;

	float4 color;

	float4 shadowTaps[12];
	ShadowMap* shadowMap;

	int mode;
};


struct DiffuseRenderer
{
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;

	ID3D11InputLayout* m_inputLayout = nullptr;

	ID3D11VertexShader* m_diffuseVS = nullptr;
	ID3D11GeometryShader* m_diffuseGS = nullptr;
	ID3D11PixelShader* m_diffusePS = nullptr;

	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11RasterizerState* m_rasterizerState = nullptr;

	ID3D11BlendState* m_blendState = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;

	void Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	void Destroy();


	void Draw(const DiffuseDrawParams* params, ID3D11Buffer* positions, ID3D11Buffer* velocities, ID3D11Buffer* indices, int numParticles);
};
