/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "renderParamsD3D.h"

/* static */void RenderParamsUtilD3D::calcShadowParams(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, float shadowBias, ShadowParamsD3D* paramsOut)
{
	paramsOut->lightTransform = (DirectX::XMMATRIX&)(convertGLToD3DProjection(lightTransform));
	paramsOut->lightPos = (float3&)lightPos;
	paramsOut->lightDir = (float3&)Normalize(lightTarget - lightPos);
	paramsOut->bias = shadowBias;

	const Vec4 taps[] =
	{
		Vec2(-0.326212f,-0.40581f), Vec2(-0.840144f,-0.07358f),
		Vec2(-0.695914f,0.457137f), Vec2(-0.203345f,0.620716f),
		Vec2(0.96234f,-0.194983f), Vec2(0.473434f,-0.480026f),
		Vec2(0.519456f,0.767022f), Vec2(0.185461f,-0.893124f),
		Vec2(0.507431f,0.064425f), Vec2(0.89642f,0.412458f),
		Vec2(-0.32194f,-0.932615f), Vec2(-0.791559f,-0.59771f)
	};
	memcpy(paramsOut->shadowTaps, taps, sizeof(taps));
}

Matrix44 RenderParamsUtilD3D::convertGLToD3DProjection(const Matrix44& proj)
{
	Matrix44 scale = Matrix44::kIdentity;
	scale.columns[2][2] = 0.5f;

	Matrix44 bias = Matrix44::kIdentity;
	bias.columns[3][2] = 1.0f;

	return scale*bias*proj;
}

/* static */void RenderParamsUtilD3D::calcMeshConstantBuffer(const MeshDrawParamsD3D& params, Hlsl::MeshShaderConst& constBuf)
{
	constBuf.modelViewProjection = (float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params.model, params.view), params.projection));

	constBuf.modelView = (float4x4&)XMMatrixMultiply(params.model, params.view);

	constBuf.objectTransform = (float4x4&)params.objectTransform;
	constBuf.lightTransform = (float4x4&)params.lightTransform;

	constBuf.clipPlane = params.clipPlane;
	constBuf.fogColor = params.fogColor;
	constBuf.color = params.color;
	constBuf.secondaryColor = params.secondaryColor;

	constBuf.lightPos = params.lightPos;
	constBuf.lightDir = params.lightDir;

	constBuf.bias = params.bias;
	constBuf.expand = params.expand;
	constBuf.spotMin = params.spotMin;
	constBuf.spotMax = params.spotMax;

	constBuf.grid = params.grid;
	constBuf.tex = params.tex;
	constBuf.colorArray = params.colorArray;

	memcpy(constBuf.shadowTaps, params.shadowTaps, sizeof(constBuf.shadowTaps));
}

/* static */void RenderParamsUtilD3D::calcFluidConstantBuffer(const FluidDrawParamsD3D& params, Hlsl::FluidShaderConst& constBuf)
{
	constBuf.modelViewProjection = (Hlsl::float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params.model, params.view), params.projection));
	constBuf.modelView = (Hlsl::float4x4&)XMMatrixMultiply(params.model, params.view);
	constBuf.projection = (Hlsl::float4x4&)params.projection;
	constBuf.inverseModelView = (Hlsl::float4x4&)XMMatrixInverse(nullptr, XMMatrixMultiply(params.model, params.view));
	constBuf.inverseProjection = (Hlsl::float4x4&)XMMatrixInverse(nullptr, params.projection);

	//constBuf.invTexScale = invTexScale;
	//constBuf.invProjection = invProjection;
	constBuf.invViewport = params.invViewport;

	constBuf.blurRadiusWorld = params.blurRadiusWorld;
	constBuf.blurScale = params.blurScale;
	constBuf.blurFalloff = params.blurFalloff;
	constBuf.debug = params.debug;

	constBuf.pointRadius = params.pointRadius;
}

/* static */void RenderParamsUtilD3D::calcDiffuseConstantBuffer(const DiffuseDrawParamsD3D& params, Hlsl::DiffuseShaderConst& constBuf)
{
	using namespace DirectX;

	XMMATRIX modelViewProj = XMMatrixMultiply(XMMatrixMultiply(params.model, params.view), params.projection);
	constBuf.modelViewProjection = (Hlsl::float4x4&)modelViewProj;
	XMMATRIX modelView = XMMatrixMultiply(params.model, params.view);
	constBuf.modelView = (Hlsl::float4x4&)modelView;
	constBuf.projection = (Hlsl::float4x4&)params.projection;

	constBuf.motionBlurScale = params.motionScale;
	constBuf.diffuseRadius = params.diffuseRadius;
	constBuf.diffuseScale = params.diffuseScale;
	constBuf.spotMin = params.spotMin;
	constBuf.spotMax = params.spotMax;

	constBuf.lightTransform = (Hlsl::float4x4&)params.lightTransform;
	constBuf.lightPos = params.lightPos;
	constBuf.lightDir = params.lightDir;
	constBuf.color = params.color;

	memcpy(constBuf.shadowTaps, params.shadowTaps, sizeof(constBuf.shadowTaps));
}

/* static */void RenderParamsUtilD3D::calcFluidCompositeConstantBuffer(const FluidDrawParamsD3D& params, Hlsl::FluidShaderConst& constBuf)
{
	constBuf.modelViewProjection = (Hlsl::float4x4&)(XMMatrixMultiply(XMMatrixMultiply(params.model, params.view), params.projection));
	constBuf.modelView = (Hlsl::float4x4&)XMMatrixMultiply(params.model, params.view);
	constBuf.projection = (Hlsl::float4x4&)params.projection;
	constBuf.inverseModelView = (Hlsl::float4x4&)XMMatrixInverse(nullptr, XMMatrixMultiply(params.model, params.view));
	constBuf.inverseProjection = (Hlsl::float4x4&)XMMatrixInverse(nullptr, params.projection);

	constBuf.lightTransform = (Hlsl::float4x4&)params.lightTransform;

	constBuf.invTexScale = params.invTexScale;

	//constBuf.invViewport = params.invViewport;
	//constBuf.invProjection = params.invProjection;

	constBuf.blurRadiusWorld = params.blurRadiusWorld;
	constBuf.blurScale = params.blurScale;
	constBuf.blurFalloff = params.blurFalloff;
	constBuf.debug = params.debug;

	constBuf.clipPosToEye = params.clipPosToEye;
	constBuf.color = params.color;
	constBuf.ior = params.ior;
	constBuf.spotMin = params.spotMin;
	constBuf.spotMax = params.spotMax;

	constBuf.lightPos = params.lightPos;
	constBuf.lightDir = params.lightDir;

	typedef DirectX::XMFLOAT2 float2;
	const float2 taps[] =
	{
		float2(-0.326212f,-0.40581f), float2(-0.840144f,-0.07358f),
		float2(-0.695914f,0.457137f), float2(-0.203345f,0.620716f),
		float2(0.96234f,-0.194983f), float2(0.473434f,-0.480026f),
		float2(0.519456f,0.767022f), float2(0.185461f,-0.893124f),
		float2(0.507431f,0.064425f), float2(0.89642f,0.412458f),
		float2(-0.32194f,-0.932615f), float2(-0.791559f,-0.59771f)
	};
	memcpy(constBuf.shadowTaps, taps, sizeof(taps));
}

/* static */void RenderParamsUtilD3D::calcPointConstantBuffer(const PointDrawParamsD3D& params, Hlsl::PointShaderConst& constBuf)
{
	using namespace DirectX;

	constBuf.modelView = (float4x4&)XMMatrixMultiply(params.model, params.view);
	constBuf.projection = (float4x4&)params.projection;

	constBuf.pointRadius = params.pointRadius;
	constBuf.pointScale = params.pointScale;
	constBuf.spotMin = params.spotMin;
	constBuf.spotMax = params.spotMax;

	constBuf.lightTransform = (float4x4&)params.lightTransform;
	constBuf.lightPos = params.lightPos;
	constBuf.lightDir = params.lightDir;

	for (int i = 0; i < 8; i++)
		constBuf.colors[i] = params.colors[i];

	memcpy(constBuf.shadowTaps, params.shadowTaps, sizeof(constBuf.shadowTaps));

	constBuf.mode = params.mode;
}