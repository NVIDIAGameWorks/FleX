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

#include "../d3d/renderParamsD3D.h"

#include <DirectXMath.h>
#include <wrl.h>
using namespace Microsoft::WRL;


struct PointRendererD3D11
{
	void init(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	
	void draw(const PointDrawParamsD3D* params, ID3D11Buffer* positions, ID3D11Buffer* colors, ID3D11Buffer* indices, int numParticles, int offset);

	PointRendererD3D11():
		m_device(nullptr),
		m_deviceContext(nullptr)
	{}

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;

	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11VertexShader> m_pointVs;
	ComPtr<ID3D11GeometryShader> m_pointGs;
	ComPtr<ID3D11PixelShader> m_pointPs;
	ComPtr<ID3D11PixelShader> m_pointShadowPs;
	ComPtr<ID3D11Buffer> m_constantBuffer;

	// Assumes right handed
	ComPtr<ID3D11RasterizerState> m_rasterizerState[NUM_POINT_RENDER_MODES][NUM_POINT_CULL_MODES];
};
