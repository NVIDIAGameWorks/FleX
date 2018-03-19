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

#include "renderTargetD3D11.h"
#include "shadowMapD3D11.h"
#include "../include/NvFlex.h"
#include "../d3d/renderParamsD3D.h"
#include "shaders.h"


struct FluidRenderBuffersD3D11
{
	FluidRenderBuffersD3D11():
		m_positionsBuf(nullptr),
		m_densitiesBuf(nullptr),
		m_indicesBuf(nullptr)
	{
		for (int i = 0; i < 3; i++)
		{
			m_anisotropiesBufArr[i] = nullptr;
		}
		m_numParticles = 0;
	}
	~FluidRenderBuffersD3D11()
	{
		NvFlexUnregisterD3DBuffer(m_positionsBuf);
		NvFlexUnregisterD3DBuffer(m_densitiesBuf);
		NvFlexUnregisterD3DBuffer(m_indicesBuf);

		for (int i = 0; i < 3; i++)
		{
			NvFlexUnregisterD3DBuffer(m_anisotropiesBufArr[i]);
		}
	}

	int m_numParticles;
	ComPtr<ID3D11Buffer> m_positions;
	ComPtr<ID3D11Buffer> m_densities;
	ComPtr<ID3D11Buffer> m_anisotropiesArr[3];
	ComPtr<ID3D11Buffer> m_indices;

	ComPtr<ID3D11Buffer> m_fluid; // to be removed

	// wrapper buffers that allow Flex to write directly to VBOs
	NvFlexBuffer* m_positionsBuf;
	NvFlexBuffer* m_densitiesBuf;
	NvFlexBuffer* m_anisotropiesBufArr[3];
	NvFlexBuffer* m_indicesBuf;
};

struct FluidRendererD3D11
{	
	void init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
	
	void drawThickness(const FluidDrawParamsD3D* params, const FluidRenderBuffersD3D11* buffers);
	void drawEllipsoids(const FluidDrawParamsD3D* params, const FluidRenderBuffersD3D11* buffers);
	void drawBlurDepth(const FluidDrawParamsD3D* params);
	void drawComposite(const FluidDrawParamsD3D* params, ID3D11ShaderResourceView* sceneMap);

	FluidRendererD3D11():
		m_device(nullptr),
		m_deviceContext(nullptr)
	{}

	void _createScreenQuad();

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;

	ComPtr<ID3D11InputLayout> m_pointThicknessLayout;
	ComPtr<ID3D11VertexShader> m_pointThicknessVs;
	ComPtr<ID3D11GeometryShader> m_pointThicknessGs;
	ComPtr<ID3D11PixelShader> m_pointThicknessPs;

	ComPtr<ID3D11InputLayout> m_ellipsoidDepthLayout;
	ComPtr<ID3D11VertexShader> m_ellipsoidDepthVs;
	ComPtr<ID3D11GeometryShader> m_ellipsoidDepthGs;
	ComPtr<ID3D11PixelShader> m_ellipsoidDepthPs;

	ComPtr<ID3D11InputLayout> m_passThroughLayout;
	ComPtr<ID3D11VertexShader> m_passThroughVs;

	ComPtr<ID3D11PixelShader> m_blurDepthPs;
	ComPtr<ID3D11PixelShader> m_compositePs;

	ComPtr<ID3D11Buffer> m_constantBuffer;

	// Right handed rasterizer state
	ComPtr<ID3D11RasterizerState> m_rasterizerState[NUM_FLUID_RENDER_MODES][NUM_FLUID_CULL_MODES];

	ComPtr<ID3D11Buffer> m_quadVertexBuffer;
	ComPtr<ID3D11Buffer> m_quadIndexBuffer;

	RenderTargetD3D11 m_thicknessTexture;
	RenderTargetD3D11 m_depthTexture;
	RenderTargetD3D11 m_depthSmoothTexture;

	int m_sceneWidth;
	int m_sceneHeight;
};


