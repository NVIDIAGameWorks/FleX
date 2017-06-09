#define NOMINMAX

#include <NvCoDx12HelperUtil.h>

#include <external/D3D12/include/d3dx12.h>

#include "pipelineUtilD3D12.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

// Shaders
#include "../d3d/shaders/diffuseVS.hlsl.h"
#include "../d3d/shaders/diffuseGS.hlsl.h"
#include "../d3d/shaders/diffusePS.hlsl.h"

// this
#include "diffusePointRenderPipelineD3D12.h"

namespace FlexSample {

static const D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

DiffusePointRenderPipelineD3D12::DiffusePointRenderPipelineD3D12():
	Parent(PRIMITIVE_POINT),
	m_shadowMapLinearSamplerIndex(-1)
{
}

static D3D12_FILL_MODE _getFillMode(DiffusePointRenderPipelineD3D12::PipelineStateType type)
{
	return D3D12_FILL_MODE_SOLID;
}

static D3D12_CULL_MODE _getCullMode(DiffusePointRenderPipelineD3D12::PipelineStateType type)
{
	return D3D12_CULL_MODE_NONE;
}

static void _initRasterizerDesc(DiffusePointRenderPipelineD3D12::PipelineStateType type, D3D12_RASTERIZER_DESC& desc)
{
	PipelineUtilD3D::initRasterizerDesc(FRONT_WINDING_COUNTER_CLOCKWISE, desc);
	desc.FillMode = _getFillMode(type);
	desc.CullMode = _getCullMode(type);
}

static void _initPipelineStateDesc(DiffusePointRenderPipelineD3D12::PipelineStateType type, NvCo::Dx12RenderTarget* shadowMap, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	PipelineUtilD3D::initTargetFormat(nullptr, renderContext, psoDesc);

	// Z test, but don't write
	{
		D3D12_DEPTH_STENCIL_DESC& desc = psoDesc.DepthStencilState;
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	psoDesc.InputLayout.NumElements = _countof(InputElementDescs);
	psoDesc.InputLayout.pInputElementDescs = InputElementDescs;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
}

int DiffusePointRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath, int shadowMapLinearSamplerIndex, NvCo::Dx12RenderTarget* shadowMap)
{
	using namespace NvCo;
	ID3D12Device* device = state.m_device;

	m_shadowMapLinearSamplerIndex = shadowMapLinearSamplerIndex;

	// create the pipeline state object
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		{
			// create blend state
			D3D12_BLEND_DESC& desc = psoDesc.BlendState;
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			{
				const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
				{
					TRUE,FALSE,
					D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
					D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
					D3D12_LOGIC_OP_NOOP,
					D3D12_COLOR_WRITE_ENABLE_ALL,
				};
				for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
					desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
			}
		}


		// create the root signature
		ComPtr<ID3D12RootSignature> signiture;
		{
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			CD3DX12_ROOT_PARAMETER params[3];
			
			UINT rootIndex = 0;
			{
				D3D12_ROOT_PARAMETER& param = params[rootIndex++];
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				param.Descriptor.ShaderRegister = 0u;
				param.Descriptor.RegisterSpace = 0u;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}

			D3D12_ROOT_SIGNATURE_DESC desc;
			desc.NumParameters = rootIndex;
			desc.pParameters = params;
			desc.NumStaticSamplers = 0u;
			desc.pStaticSamplers = nullptr;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ComPtr<ID3DBlob> sigBlob;
			NV_RETURN_ON_FAIL(Dx12HelperUtil::serializeRootSigniture(desc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob));
			NV_RETURN_ON_FAIL(device->CreateRootSignature(0u, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&signiture)));
		}

		{
			psoDesc.VS = Dx12Blob(g_diffuseVS);
			psoDesc.GS = Dx12Blob(g_diffuseGS);
			psoDesc.PS = Dx12Blob(g_diffusePS);

			NV_RETURN_ON_FAIL(_initPipelineState(state, shadowMap, PIPELINE_STATE_LIGHT_SOLID, signiture.Get(), psoDesc));
		}
	}

	return NV_OK;
}

int DiffusePointRenderPipelineD3D12::_initPipelineState(const RenderStateD3D12& state, NvCo::Dx12RenderTarget* shadowMap, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	ID3D12Device* device = state.m_device;

	_initRasterizerDesc(pipeType, psoDesc.RasterizerState);
	_initPipelineStateDesc(pipeType, shadowMap, state.m_renderContext, psoDesc);

	psoDesc.pRootSignature = signiture;

	PipelineStateD3D12& pipeState = m_states[pipeType];
	pipeState.m_rootSignature = signiture;
	
	NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
	return NV_OK;
}

int DiffusePointRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	const DiffuseDrawParamsD3D& params = *(DiffuseDrawParamsD3D*)paramsIn;

	// Set up constant buffer
	NvCo::Dx12CircularResourceHeap::Cursor cursor;
	{
		Hlsl::DiffuseShaderConst constBuf;
		RenderParamsUtilD3D::calcDiffuseConstantBuffer(params, constBuf);
		cursor = state.m_constantHeap->newConstantBuffer(constBuf);
		if (!cursor.isValid())
		{
			return NV_FAIL;
		}
	}

	const PipelineStateType pipeType = PIPELINE_STATE_LIGHT_SOLID;
	PipelineStateD3D12& pipeState = m_states[pipeType];
	if (!pipeState.isValid())
	{
		return NV_FAIL;
	}

	ID3D12GraphicsCommandList* commandList = state.m_commandList;
	commandList->SetGraphicsRootSignature(pipeState.m_rootSignature.Get());
	commandList->SetPipelineState(pipeState.m_pipelineState.Get());

	D3D12_GPU_VIRTUAL_ADDRESS cbvHandle = state.m_constantHeap->getGpuHandle(cursor);
	commandList->SetGraphicsRootConstantBufferView(0, cbvHandle);

	{
		ID3D12DescriptorHeap* heaps[] = { state.m_srvCbvUavDescriptorHeap->getHeap() }; 
		commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	}
	return NV_OK;
}

int DiffusePointRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	typedef PointRenderAllocationD3D12 Alloc;
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;

	assert(sizeof(Alloc) == sizeOfAlloc);
	const Alloc& alloc = static_cast<const Alloc&>(allocIn);

	assert(alloc.m_numPrimitives >= 0);

	ID3D12GraphicsCommandList* commandList = state.m_commandList;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[] =
	{
		alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION],
		alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1],			// Actually holds velocity
	};

	commandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);
	commandList->IASetIndexBuffer(&alloc.m_indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	if (alloc.m_indexBufferView.SizeInBytes)
	{
		commandList->DrawIndexedInstanced((UINT)allocIn.m_numPrimitives, 1, 0, 0, 0);
	}
	else
	{
		commandList->DrawInstanced((UINT)allocIn.m_numPrimitives, 1, 0, 0);
	}
	return NV_OK;
}

} // namespace FlexSample