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

#define NOMINMAX

#include "core/maths.h"
#include <DirectXMath.h>

#include "shaderCommonD3D.h"

struct ShadowMapD3D;

typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;

struct DiffuseDrawParamsD3D
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
	ShadowMapD3D* shadowMap;

	int mode;
};

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

struct FluidDrawParamsD3D
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

	float pointRadius;

	ShadowMapD3D* shadowMap;

	// Used on D3D12
	int m_srvDescriptorBase;			
	int m_sampleDescriptorBase;
};

enum MeshRenderMode
{
	MESH_RENDER_WIREFRAME,
	MESH_RENDER_SOLID,
	NUM_MESH_RENDER_MODES
};

enum MeshCullMode
{
	MESH_CULL_NONE,
	MESH_CULL_FRONT,
	MESH_CULL_BACK,
	NUM_MESH_CULL_MODES
};

enum MeshDrawStage
{
	MESH_DRAW_NULL,
	MESH_DRAW_SHADOW,
	MESH_DRAW_REFLECTION,
	MESH_DRAW_LIGHT
};

struct MeshDrawParamsD3D
{
	MeshRenderMode renderMode;
	MeshCullMode cullMode;
	MeshDrawStage renderStage;

	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX model;

	float4x4 objectTransform;

	DirectX::XMMATRIX lightTransform;
	float3 lightPos;
	float3 lightDir;

	float4 clipPlane;
	float4 fogColor;
	float4 color;
	float4 secondaryColor;

	float bias;
	float expand;
	float spotMin;
	float spotMax;

	int grid;
	int tex;
	int colorArray;

	float4 shadowTaps[12];
	ShadowMapD3D* shadowMap;
};

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

struct PointDrawParamsD3D
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
	ShadowMapD3D* shadowMap;

	int mode;
};

struct ShadowParamsD3D
{
	DirectX::XMMATRIX lightTransform;
	float3 lightPos;
	float3 lightDir;
	float bias;
	float4 shadowTaps[12];
};

struct RenderParamsUtilD3D
{
		/// Assumes the lightTransform is in OpenGl projection format
	static void calcShadowParams(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, float shadowBias, ShadowParamsD3D* paramsOut);

		/// Convert a projection from GL into the same projection as can be used by Direct3D
		/// D3D has clip z range [0, 1], GL -1 to 1 (as is unit cube)
	static Matrix44 convertGLToD3DProjection(const Matrix44& proj);

	static void calcMeshConstantBuffer(const MeshDrawParamsD3D& params, Hlsl::MeshShaderConst& constBufOut);

	static void calcFluidConstantBuffer(const FluidDrawParamsD3D& params, Hlsl::FluidShaderConst& constBuf);

	static void calcFluidCompositeConstantBuffer(const FluidDrawParamsD3D& params, Hlsl::FluidShaderConst& constBuf);

	static void calcDiffuseConstantBuffer(const DiffuseDrawParamsD3D& params, Hlsl::DiffuseShaderConst& constBuf);

	static void calcPointConstantBuffer(const PointDrawParamsD3D& params, Hlsl::PointShaderConst& constBuf);
};
