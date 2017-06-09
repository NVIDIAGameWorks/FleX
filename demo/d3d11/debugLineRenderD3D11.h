#pragma once

#include <vector>
#include "core/maths.h"

#include <wrl.h>
using namespace Microsoft::WRL;

struct DebugLineRenderD3D11
{
	struct Vertex
	{
		Vec3 position;
		Vec4 color;
	};

	void init(ID3D11Device* d, ID3D11DeviceContext* c);
	void addLine(const Vec3& p, const Vec3& q, const Vec4& color);
	void flush(const Matrix44& viewProj);

	DebugLineRenderD3D11():
		m_vertexBufferSize(0),
		m_device(nullptr),
		m_context(nullptr)
	{}

	std::vector<Vertex> m_queued;

	ComPtr<ID3D11Buffer> m_vertexBuffer;
	int m_vertexBufferSize;

	ComPtr<ID3D11DepthStencilState> m_depthStencilState;
	ComPtr<ID3D11RasterizerState> m_rasterizerState;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11BlendState> m_blendState;
	ComPtr<ID3D11VertexShader> m_vertexShader;
	ComPtr<ID3D11PixelShader> m_pixelShader;
	ComPtr<ID3D11Buffer> m_constantBuffer;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
};