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

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#define NV_NULL nullptr
#define NV_RETURN_ON_FAIL(x) { HRESULT _res = (x); if (FAILED(_res)) { return _res; } }

struct RenderTarget
{
public:

	RenderTarget();

	HRESULT Init(ID3D11Device* device, int width, int height, bool depthTest = true);

	void BindAndClear(ID3D11DeviceContext* context);

	XMMATRIX m_lightView;
	XMMATRIX m_lightProjection;
	XMMATRIX m_lightWorldToTex;

	ComPtr<ID3D11Texture2D> m_backTexture;
	ComPtr<ID3D11RenderTargetView> m_backRtv;
	ComPtr<ID3D11ShaderResourceView> m_backSrv;

	ComPtr<ID3D11Texture2D> m_depthTexture;
	ComPtr<ID3D11DepthStencilView> m_depthDsv;
	ComPtr<ID3D11ShaderResourceView> m_depthSrv;

	ComPtr<ID3D11SamplerState> m_linearSampler;
	ComPtr<ID3D11SamplerState> m_pointSampler;

	ComPtr<ID3D11DepthStencilState> m_depthStencilState;

	D3D11_VIEWPORT m_viewport;
};
