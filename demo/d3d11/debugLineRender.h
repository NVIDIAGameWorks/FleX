#pragma once


#include "shaders/debugLineVS.hlsl.h"
#include "shaders/debugLinePS.hlsl.h"


struct DebugVertex
{
	Vec3 position;
	Vec4 color;
};

struct DebugLineRender
{

	void Init(ID3D11Device* d, ID3D11DeviceContext* c) 
	{
		device = d;
		context = c;

		// create the rasterizer state
		{
			D3D11_RASTERIZER_DESC desc = {};
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = TRUE;	// This is non-default
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.f;
			desc.SlopeScaledDepthBias = 0.f;
			desc.DepthClipEnable = TRUE;
			desc.ScissorEnable = FALSE;		// This is non-default
			desc.MultisampleEnable = TRUE;
			desc.AntialiasedLineEnable = FALSE;			

			device->CreateRasterizerState(&desc, &rasterizerState);
		}

		{
			D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
			depthStateDesc.DepthEnable = FALSE;	// disable depth test
			depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			device->CreateDepthStencilState(&depthStateDesc, &depthStencilState);
		}

		// create the input layout
		{
			D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			device->CreateInputLayout(inputElementDescs, 2, g_debugLineVS, sizeof(g_debugLineVS), &inputLayout);
		}

		// create the blend state
		{
			D3D11_BLEND_DESC blendDesc = {};

			blendDesc.AlphaToCoverageEnable = false;
			blendDesc.IndependentBlendEnable = false;
			blendDesc.RenderTarget[0].BlendEnable = false;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			device->CreateBlendState(&blendDesc, &blendState);
		}

		// create the shaders
		device->CreateVertexShader(g_debugLineVS, sizeof(g_debugLineVS), nullptr, &vertexShader);
		device->CreatePixelShader(g_debugLinePS, sizeof(g_debugLinePS), nullptr, &pixelShader);

		// create a constant buffer
		{
			D3D11_BUFFER_DESC bufDesc;
			bufDesc.ByteWidth = sizeof(Matrix44);
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufDesc.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;
			bufDesc.MiscFlags = 0;

			device->CreateBuffer(&bufDesc, NULL, &constantBuffer);
		}
	}

	void Destroy()
	{
		COMRelease(depthStencilState);
		COMRelease(rasterizerState);
		COMRelease(inputLayout);
		COMRelease(blendState);
		COMRelease(vertexShader);
		COMRelease(pixelShader);
		COMRelease(constantBuffer);
		COMRelease(vertexBuffer);
	}

	void AddLine(const Vec3& p, const Vec3& q, const Vec4& color)
	{
		DebugVertex v = { p, color };
		DebugVertex w = { q, color };

		queued.push_back(v);
		queued.push_back(w);
	}

	void FlushLines(const Matrix44& viewProj)
	{
		if (queued.empty())
			return;

		// recreate vertex buffer if not big enough for queued lines
		if (vertexBufferSize < int(queued.size()))
		{
			if (vertexBuffer)
				vertexBuffer->Release();

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage            = D3D11_USAGE_DYNAMIC;
			bufferDesc.ByteWidth        = UINT(sizeof(DebugVertex)*queued.size());
			bufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags        = 0;

			device->CreateBuffer(&bufferDesc, 0, &vertexBuffer);				

			vertexBufferSize = int(queued.size());
		}
		
		// update vertex buffer
		D3D11_MAPPED_SUBRESOURCE res;
		context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, &queued[0], sizeof(DebugVertex)*queued.size());
		context->Unmap(vertexBuffer, 0);
		
		// update constant buffer
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource = {};
			if (S_OK == context->Map(constantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))
			{
				memcpy(mappedResource.pData, &viewProj, sizeof(viewProj));

				context->Unmap(constantBuffer, 0u);
			}
		}

		// configure for line renderering
		context->VSSetShader(vertexShader, nullptr, 0u);
		context->GSSetShader(nullptr, nullptr, 0u);
		context->PSSetShader(pixelShader, nullptr, 0u);

		context->IASetInputLayout(inputLayout);
		context->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);

		context->VSSetConstantBuffers(0, 1, &constantBuffer);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		context->RSSetState(rasterizerState);

		UINT vertexStride = sizeof(DebugVertex);
		UINT offset = 0u;
		context->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &offset);


		// kick
		context->Draw(UINT(queued.size()), 0);

		// empty queue
		queued.resize(0);

	}


	std::vector<DebugVertex> queued;

	ID3D11Buffer* vertexBuffer = nullptr;
	int vertexBufferSize = 0;

	ID3D11DepthStencilState* depthStencilState = nullptr;
	ID3D11RasterizerState* rasterizerState = nullptr;
	ID3D11InputLayout* inputLayout = nullptr;
	ID3D11BlendState* blendState = nullptr;
	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;
	ID3D11Buffer* constantBuffer = nullptr;

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;

};