
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

#include "appD3D11Ctx.h"

#include "core/maths.h"

struct ShadowMap;

struct GpuMesh
{
	ID3D11Buffer* positionBuffer;
	ID3D11Buffer* normalBuffer;
	ID3D11Buffer* texcoordBuffer;
	ID3D11Buffer* colorBuffer;

	ID3D11Buffer* indexBuffer;

	GpuMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext) 
		: positionBuffer(NULL)
		, normalBuffer(NULL)
		, texcoordBuffer(NULL)
		, colorBuffer(NULL)
		, indexBuffer(NULL)		
		, mDevice(device)
		, mDeviceContext(deviceContext)
		, mNumFaces(0)
		, mNumVertices(0)
		{}

	~GpuMesh()
	{
		Release();
	}

	void Release()
	{
		COMRelease(positionBuffer);
		COMRelease(normalBuffer);
		COMRelease(texcoordBuffer);
		COMRelease(colorBuffer);
		COMRelease(indexBuffer);
	}

	void Resize(int numVertices, int numFaces)
	{
		Release();

		{		
			// vertex buffers
			D3D11_BUFFER_DESC bufDesc;
			bufDesc.ByteWidth = sizeof(Vec3)*numVertices;
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufDesc.MiscFlags = 0;
		
			mDevice->CreateBuffer(&bufDesc, NULL, &positionBuffer);
			mDevice->CreateBuffer(&bufDesc, NULL, &normalBuffer);

			bufDesc.ByteWidth = sizeof(Vec2)*numVertices;
			mDevice->CreateBuffer(&bufDesc, NULL, &texcoordBuffer);

			bufDesc.ByteWidth = sizeof(Vec4)*numVertices;
			mDevice->CreateBuffer(&bufDesc, NULL, &colorBuffer);
		}

		{
			// index buffer
			D3D11_BUFFER_DESC bufDesc;
			bufDesc.ByteWidth = sizeof(int)*numFaces*3;
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufDesc.MiscFlags = 0;

			mDevice->CreateBuffer(&bufDesc, NULL, &indexBuffer);
		}

		mMaxVertices = numVertices;
		mMaxFaces = numFaces;
	}

	void UpdateData(const Vec3* positions, const Vec3* normals, const Vec2* texcoords, const Vec4* colors, const int* indices, int numVertices, int numFaces)
	{
		if (numVertices > mMaxVertices || numFaces > mMaxFaces)
		{
			Resize(numVertices, numFaces);
		}

		D3D11_MAPPED_SUBRESOURCE res;

		// vertices
		if (positions)
		{
			mDeviceContext->Map(positionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, positions, sizeof(Vec3)*numVertices);
			mDeviceContext->Unmap(positionBuffer, 0);
		}

		if (normals)
		{
			mDeviceContext->Map(normalBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, normals, sizeof(Vec3)*numVertices);
			mDeviceContext->Unmap(normalBuffer, 0);
		}

		if (texcoords)
		{
			mDeviceContext->Map(texcoordBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, texcoords, sizeof(Vec2)*numVertices);
			mDeviceContext->Unmap(texcoordBuffer, 0);
		}

		if (colors)
		{
			mDeviceContext->Map(colorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, colors, sizeof(Vec4)*numVertices);
			mDeviceContext->Unmap(colorBuffer, 0);
		}
	
		// indices
		if (indices)
		{
			mDeviceContext->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, indices, sizeof(int)*numFaces*3);
			mDeviceContext->Unmap(indexBuffer, 0);
		}

		mNumVertices = numVertices;
		mNumFaces = numFaces;
	}

	// duplicate method to handle conversion from vec4 to vec3 positions / normals
	void UpdateData(const Vec4* positions, const Vec4* normals, const Vec2* texcoords, const Vec4* colors, const int* indices, int numVertices, int numFaces)
	{
		if (numVertices > mMaxVertices || numFaces > mMaxFaces)
		{
			Resize(numVertices, numFaces);
		}

		D3D11_MAPPED_SUBRESOURCE res;

		// vertices
		if (positions)
		{
			mDeviceContext->Map(positionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			for (int i=0; i < numVertices; ++i)
				((Vec3*)res.pData)[i] = Vec3(positions[i]);
			mDeviceContext->Unmap(positionBuffer, 0);
		}

		if (normals)
		{
			mDeviceContext->Map(normalBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			for (int i=0; i < numVertices; ++i)
				((Vec3*)res.pData)[i] = Vec3(normals[i]);
			mDeviceContext->Unmap(normalBuffer, 0);
		}

		if (texcoords)
		{
			mDeviceContext->Map(texcoordBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, texcoords, sizeof(Vec2)*numVertices);
			mDeviceContext->Unmap(texcoordBuffer, 0);
		}

		if (colors)
		{
			mDeviceContext->Map(colorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, colors, sizeof(Vec4)*numVertices);
			mDeviceContext->Unmap(colorBuffer, 0);
		}
	
		// indices
		if (indices)
		{
			mDeviceContext->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, indices, sizeof(int)*numFaces*3);
			mDeviceContext->Unmap(indexBuffer, 0);
		}

		mNumVertices = numVertices;
		mNumFaces = numFaces;
	}

	int mNumVertices = 0;
	int mNumFaces = 0;

	int mMaxVertices = 0;
	int mMaxFaces = 0;

	ID3D11Device* mDevice;
	ID3D11DeviceContext* mDeviceContext;
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

typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;

struct MeshDrawParams
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
	ShadowMap* shadowMap;
};

struct MeshRenderer
{
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;

	ID3D11InputLayout* m_inputLayout = nullptr;
	ID3D11VertexShader* m_meshVS = nullptr;
	ID3D11PixelShader* m_meshPS = nullptr;
	ID3D11PixelShader* m_meshPS_Shadow = nullptr;
	ID3D11Buffer* m_constantBuffer = nullptr;
	ID3D11RasterizerState* m_rasterizerStateRH[NUM_MESH_RENDER_MODES][NUM_MESH_CULL_MODES];

	~MeshRenderer()
	{
		Destroy();
	}

	void Init(ID3D11Device* device, ID3D11DeviceContext* context);
	void Destroy();

	void Draw(const GpuMesh* mesh, const MeshDrawParams* params);	
	
};



