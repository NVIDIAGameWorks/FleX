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

enum PointRenderMode
{
	POINT_RENDER_WIREFRAME,
	POINT_RENDER_SOLID,
	NUM_POINT_RENDER_MODES
};

enum PointCullMode
{
	POINT_CULL_NONE,
	POINT_CULL_FRONT,
	POINT_CULL_BACK,
	NUM_POINT_CULL_MODES
};

enum PointDrawStage
{
	POINT_DRAW_NULL,
	POINT_DRAW_SHADOW,
	POINT_DRAW_REFLECTION,
	POINT_DRAW_LIGHT
};

typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;

struct PointDrawParams
{
	PointRenderMode renderMode;
	PointCullMode cullMode;
	PointDrawStage renderStage;

	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX model;

	float pointRadius;  // point size in world space
	float pointScale;   // scale to calculate size in pixels
	float spotMin;
	float spotMax;

	DirectX::XMMATRIX lightTransform;
	float3 lightPos;
	float3 lightDir;

	float4 colors[8];

	float4 shadowTaps[12];
	ShadowMap* shadowMap;

	int mode;
};


struct PointRenderer
{
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_pointVS = nullptr;
	ID3D11GeometryShader* m_pointGS = nullptr;
	ID3D11PixelShader* m_pointPS = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11RasterizerState* m_rasterizerStateRH[NUM_POINT_RENDER_MODES][NUM_POINT_CULL_MODES];

	void Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	void Destroy();

	void Draw(const PointDrawParams* params, ID3D11Buffer* positions, ID3D11Buffer* colors, ID3D11Buffer* indices, int numParticles, int offset);
};
