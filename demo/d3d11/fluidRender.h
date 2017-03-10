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

#include <d3d11.h>

#include "renderTarget.h"
#include "shadowMap.h"

struct FluidRenderBuffers;

enum FluidRenderMode
{
	FLUID_RENDER_WIREFRAME,
	FLUID_RENDER_SOLID,
	NUM_FLUID_RENDER_MODES
};

enum FluidCullMode
{
	FLUID_CULL_NONE,
	FLUID_CULL_FRONT,
	FLUID_CULL_BACK,
	NUM_FLUID_CULL_MODES
};

enum FluidDrawStage
{
	FLUID_DRAW_NULL,
	FLUID_DRAW_SHADOW,
	FLUID_DRAW_REFLECTION,
	FLUID_DRAW_LIGHT
};

typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;

struct FluidDrawParams
{
	FluidRenderMode renderMode;
	FluidCullMode cullMode;
	FluidDrawStage renderStage;

	int offset;
	int n;

	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX model;

	float4 invTexScale;

	float3 invViewport;
	float3 invProjection;

	float blurRadiusWorld;
	float blurScale;
	float blurFalloff;
	int debug;

	float3 lightPos;
	float3 lightDir;
	DirectX::XMMATRIX lightTransform;

	float4 color;
	float4 clipPosToEye;

	float spotMin;
	float spotMax;
	float ior;

	ShadowMap* shadowMap;
};


struct FluidRenderer
{
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;

	ID3D11InputLayout* m_ellipsoidDepthLayout = nullptr;
	ID3D11VertexShader* m_ellipsoidDepthVS = nullptr;
	ID3D11GeometryShader* m_ellipsoidDepthGS = nullptr;
	ID3D11PixelShader* m_ellipsoidDepthPS = nullptr;

	ID3D11InputLayout* m_passThroughLayout = nullptr;
	ID3D11VertexShader* m_passThroughVS = nullptr;

	ID3D11PixelShader* m_blurDepthPS = nullptr;
	ID3D11PixelShader* m_compositePS = nullptr;

	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11RasterizerState* m_rasterizerStateRH[NUM_FLUID_RENDER_MODES][NUM_FLUID_CULL_MODES];

	ID3D11Buffer* m_quadVertexBuffer;
	ID3D11Buffer* m_quadIndexBuffer;

	RenderTarget mDepthTex;
	RenderTarget mDepthSmoothTex;
	RenderTarget mThicknessTex;

	int mSceneWidth;
	int mSceneHeight;

	void Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
	void Destroy();

	void DrawEllipsoids(const FluidDrawParams* params, const FluidRenderBuffers* buffers);
	void DrawBlurDepth(const FluidDrawParams* params);
	void DrawComposite(const FluidDrawParams* params, ID3D11ShaderResourceView* sceneMap);

	void CreateScreenQuad();
};


