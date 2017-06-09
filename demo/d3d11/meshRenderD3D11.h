
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
#include "../d3d/renderParamsD3D.h"

#include <wrl.h>
using namespace Microsoft::WRL;

struct GpuMeshD3D11
{
	GpuMeshD3D11(ID3D11Device* device, ID3D11DeviceContext* deviceContext) ;

	void resize(int numVertices, int numFaces);
	
	void updateData(const Vec3* positions, const Vec3* normals, const Vec2* texcoords, const Vec4* colors, const int* indices, int numVertices, int numFaces);
		// duplicate method to handle conversion from vec4 to vec3 positions / normals
	void updateData(const Vec4* positions, const Vec4* normals, const Vec2* texcoords, const Vec4* colors, const int* indices, int numVertices, int numFaces);
	
	ComPtr<ID3D11Buffer> m_positionBuffer;
	ComPtr<ID3D11Buffer> m_normalBuffer;
	ComPtr<ID3D11Buffer> m_texcoordBuffer;
	ComPtr<ID3D11Buffer> m_colorBuffer;

	ComPtr<ID3D11Buffer> m_indexBuffer;

	int m_numVertices;
	int m_numFaces;

	int m_maxVertices;
	int m_maxFaces;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;
};


struct MeshRendererD3D11
{
	void init(ID3D11Device* device, ID3D11DeviceContext* context, bool asyncComputeBenchmark);
	void draw(const GpuMeshD3D11* mesh, const MeshDrawParamsD3D* params);	

	MeshRendererD3D11():
		m_device(nullptr),
		m_deviceContext(nullptr)
	{}

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;

	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11VertexShader> m_meshVs;
	ComPtr<ID3D11PixelShader> m_meshPs;
	ComPtr<ID3D11PixelShader> m_meshShadowPs;
	ComPtr<ID3D11Buffer> m_constantBuffer;
	ComPtr<ID3D11RasterizerState> m_rasterizerState[NUM_MESH_RENDER_MODES][NUM_MESH_CULL_MODES];
};



