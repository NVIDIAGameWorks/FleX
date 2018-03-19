#define NOMINMAX

#include <NvCoDx12HelperUtil.h>
#include <external/D3D12/include/d3dx12.h>

#include "pipelineUtilD3D12.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

// this
#include "pointRenderPipelineD3D12.h"

// Shaders
#include "../d3d/shaders/pointVS.hlsl.h"
#include "../d3d/shaders/pointGS.hlsl.h"
#include "../d3d/shaders/pointPS.hlsl.h"
#include "../d3d/shaders/pointShadowPS.hlsl.h"

namespace FlexSample {

PointRenderPipelineD3D12::PointRenderPipelineD3D12():
	Parent(PRIMITIVE_POINT),
	m_shadowMapLinearSamplerIndex(-1)
{
}

PointRenderPipelineD3D12::PipelineStateType PointRenderPipelineD3D12::getPipelineStateType(PointDrawStage stage, PointRenderMode mode, PointCullMode cull)
{
	switch (stage)
	{
		case POINT_DRAW_SHADOW:		return PIPELINE_STATE_SHADOW;
		default:					return PIPELINE_STATE_LIGHT_SOLID;
	}
}

static D3D12_FILL_MODE _getFillMode(PointRenderPipelineD3D12::PipelineStateType type)
{
	return D3D12_FILL_MODE_SOLID;
}

static D3D12_CULL_MODE _getCullMode(PointRenderPipelineD3D12::PipelineStateType type)
{
	return D3D12_CULL_MODE_NONE;
}

static void _initRasterizerDesc(PointRenderPipelineD3D12::PipelineStateType type, D3D12_RASTERIZER_DESC& desc)
{
	PipelineUtilD3D::initRasterizerDesc(FRONT_WINDING_COUNTER_CLOCKWISE, desc);
	desc.FillMode = _getFillMode(type);
	desc.CullMode = _getCullMode(type);
}

static void _initPipelineStateDesc(PointRenderPipelineD3D12::PipelineStateType type, NvCo::Dx12RenderTarget* shadowMap, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	PipelineUtilD3D::initTargetFormat((shadowMap && type == PointRenderPipelineD3D12::PIPELINE_STATE_SHADOW) ? shadowMap : nullptr, renderContext, psoDesc);

	psoDesc.InputLayout.NumElements = _countof(MeshRendererD3D12::PointInputElementDescs);
	psoDesc.InputLayout.pInputElementDescs = MeshRendererD3D12::PointInputElementDescs;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
}

int PointRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath, int shadowMapLinearSamplerIndex, NvCo::Dx12RenderTarget* shadowMap)
{
	using namespace NvCo;
	ID3D12Device* device = state.m_device;

	m_shadowMapLinearSamplerIndex = shadowMapLinearSamplerIndex;

	// create the pipeline state object
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		PipelineUtilD3D::initSolidBlendDesc(psoDesc.BlendState);

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

			const int numSrvs = 1;
			if (numSrvs > 0)
			{
				ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSrvs, 0);
				params[rootIndex++].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
			}
			const int numSamplers = 1;
			if (numSamplers > 0)
			{
				ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSamplers, 0);
				params[rootIndex++].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
			}

			D3D12_ROOT_SIGNATURE_DESC desc;
			desc.NumParameters = rootIndex;
			desc.pParameters = params;
			desc.NumStaticSamplers = 0;
			desc.pStaticSamplers = nullptr;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ComPtr<ID3DBlob> sigBlob;
			NV_RETURN_ON_FAIL(Dx12HelperUtil::serializeRootSigniture(desc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob));
			NV_RETURN_ON_FAIL(device->CreateRootSignature(0u, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&signiture)));
		}

		{
			psoDesc.VS = Dx12Blob(g_pointVS);
			psoDesc.GS = Dx12Blob(g_pointGS);
			psoDesc.PS = Dx12Blob(g_pointPS);

			NV_RETURN_ON_FAIL(_initPipelineState(state, shadowMap, PIPELINE_STATE_LIGHT_SOLID, signiture.Get(), psoDesc));
		}

		// Shadow rendering
		{
			D3D12_ROOT_PARAMETER params[1];
			params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			params[0].Descriptor.ShaderRegister = 0u;
			params[0].Descriptor.RegisterSpace = 0u;
			params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			D3D12_ROOT_SIGNATURE_DESC desc;
			desc.NumParameters = 1;
			desc.pParameters = params;
			desc.NumStaticSamplers = 0u;
			desc.pStaticSamplers = nullptr;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ComPtr<ID3DBlob> sigBlob;
			NV_RETURN_ON_FAIL(Dx12HelperUtil::serializeRootSigniture(desc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob));
			NV_RETURN_ON_FAIL(device->CreateRootSignature(0u, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&signiture)));
		}

		{
			psoDesc.VS = Dx12Blob(g_pointVS);
			psoDesc.GS = Dx12Blob(g_pointGS);
			psoDesc.PS = Dx12Blob(g_pointShadowPS);

			NV_RETURN_ON_FAIL(_initPipelineState(state, shadowMap, PIPELINE_STATE_SHADOW, signiture.Get(), psoDesc));
		}
	}

	return NV_OK;
}

int PointRenderPipelineD3D12::_initPipelineState(const RenderStateD3D12& state, NvCo::Dx12RenderTarget* shadowMap, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
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

int PointRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	const PointDrawParamsD3D& params = *(PointDrawParamsD3D*)paramsIn;

	// Set up constant buffer
	NvCo::Dx12CircularResourceHeap::Cursor cursor;
	{
		Hlsl::PointShaderConst constBuf;
		RenderParamsUtilD3D::calcPointConstantBuffer(params, constBuf);
		cursor = state.m_constantHeap->newConstantBuffer(constBuf);
		if (!cursor.isValid())
		{
			return NV_FAIL;
		}
	}

	const PipelineStateType pipeType = getPipelineStateType(params.renderStage, params.renderMode, params.cullMode);
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

	if (pipeType == PIPELINE_STATE_SHADOW)
	{
		ID3D12DescriptorHeap* heaps[] = { nullptr };
		commandList->SetDescriptorHeaps(0, heaps);
	}
	else
	{
		NvCo::Dx12RenderTarget* shadowMap = (NvCo::Dx12RenderTarget*)params.shadowMap;

		ID3D12DescriptorHeap* heaps[] = { state.m_srvCbvUavDescriptorHeap->getHeap(), state.m_samplerDescriptorHeap->getHeap() };
		commandList->SetDescriptorHeaps(_countof(heaps), heaps);
		// Bind the srvs
		commandList->SetGraphicsRootDescriptorTable(1, state.m_srvCbvUavDescriptorHeap->getGpuHandle(shadowMap->getSrvHeapIndex(shadowMap->getPrimaryBufferType())));
		// Bind the samplers
		commandList->SetGraphicsRootDescriptorTable(2, state.m_samplerDescriptorHeap->getGpuHandle(m_shadowMapLinearSamplerIndex));
	}

	return NV_OK;
}

int PointRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	return MeshRendererD3D12::defaultDraw(allocIn, sizeOfAlloc, platformState);
}

} // namespace FlexSample