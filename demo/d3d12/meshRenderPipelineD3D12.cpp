#define NOMINMAX

#include <NvCoDx12HelperUtil.h>
#include <external/D3D12/include/d3dx12.h>

#include "pipelineUtilD3D12.h"
#include "meshRenderer.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

// this
#include "meshRenderPipelineD3D12.h"

// Shaders
#include "../d3d/shaders/meshVS.hlsl.h"
#include "../d3d/shaders/meshPS.hlsl.h"
#include "../d3d/shaders/meshShadowPS.hlsl.h"

namespace FlexSample {

// Make async compute benchmark shader have a unique name
namespace AsyncComputeBench
{
#include "../d3d/shaders/meshAsyncComputeBenchPS.hlsl.h"
}

MeshRenderPipelineD3D12::MeshRenderPipelineD3D12():
	Parent(PRIMITIVE_TRIANGLE),
	m_shadowMapLinearSamplerIndex(-1)
{
}

/* static */MeshRenderPipelineD3D12::PipelineStateType MeshRenderPipelineD3D12::getPipelineStateType(MeshDrawStage stage, MeshRenderMode mode, MeshCullMode cull)
{
	switch (stage)
	{
		case MESH_DRAW_NULL:
		case MESH_DRAW_REFLECTION:
		case MESH_DRAW_LIGHT:
		{
			if (mode == MESH_RENDER_WIREFRAME)
			{
				return PIPELINE_STATE_LIGHT_WIREFRAME;
			}

			switch (cull)
			{
				case MESH_CULL_BACK:		return PIPELINE_STATE_LIGHT_SOLID_CULL_BACK;
				case MESH_CULL_FRONT:		return PIPELINE_STATE_LIGHT_SOLID_CULL_FRONT;
				default:
				case MESH_CULL_NONE:		return PIPELINE_STATE_LIGHT_SOLID_CULL_NONE;
			}
		}
		case MESH_DRAW_SHADOW:		
		{
			switch (cull)
			{
				case MESH_CULL_BACK:	return PIPELINE_STATE_SHADOW_CULL_BACK;
				default:
				case MESH_CULL_NONE:	return PIPELINE_STATE_SHADOW_CULL_NONE;
			}
		}
	}

	printf("Unhandled option!");
	return PIPELINE_STATE_LIGHT_SOLID_CULL_BACK;
}

static D3D12_FILL_MODE _getFillMode(MeshRenderPipelineD3D12::PipelineStateType type)
{
	switch (type)
	{
		default:	return D3D12_FILL_MODE_SOLID;
		case MeshRenderPipelineD3D12::PIPELINE_STATE_LIGHT_WIREFRAME:	return D3D12_FILL_MODE_WIREFRAME;
	}
}

static D3D12_CULL_MODE _getCullMode(MeshRenderPipelineD3D12::PipelineStateType type)
{
	switch (type)
	{
		case MeshRenderPipelineD3D12::PIPELINE_STATE_COUNT_OF: break;

		case MeshRenderPipelineD3D12::PIPELINE_STATE_SHADOW_CULL_BACK:			return D3D12_CULL_MODE_BACK;

		case MeshRenderPipelineD3D12::PIPELINE_STATE_LIGHT_SOLID_CULL_FRONT:	return D3D12_CULL_MODE_FRONT;
		case MeshRenderPipelineD3D12::PIPELINE_STATE_LIGHT_SOLID_CULL_BACK:	return D3D12_CULL_MODE_BACK;

		case MeshRenderPipelineD3D12::PIPELINE_STATE_SHADOW_CULL_NONE:
		case MeshRenderPipelineD3D12::PIPELINE_STATE_LIGHT_WIREFRAME:
		case MeshRenderPipelineD3D12::PIPELINE_STATE_LIGHT_SOLID_CULL_NONE:	return D3D12_CULL_MODE_NONE;
	}

	printf("Unhandled option!");
	return D3D12_CULL_MODE_NONE;
}

static void _initRasterizerDesc(MeshRenderPipelineD3D12::PipelineStateType type, FrontWindingType winding, D3D12_RASTERIZER_DESC& desc)
{
	PipelineUtilD3D::initRasterizerDesc(winding, desc);

	desc.FillMode = _getFillMode(type);
	desc.CullMode = _getCullMode(type);
}

static void _initPipelineStateDesc(MeshRenderPipelineD3D12::PipelineStateType type, NvCo::Dx12RenderTarget* shadowMap, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	PipelineUtilD3D::initTargetFormat((shadowMap && MeshRenderPipelineD3D12::isShadow(type)) ? shadowMap : nullptr, renderContext, psoDesc);

	psoDesc.InputLayout.NumElements = _countof(MeshRendererD3D12::MeshInputElementDescs);
	psoDesc.InputLayout.pInputElementDescs = MeshRendererD3D12::MeshInputElementDescs;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

int MeshRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath, FrontWindingType winding, int shadowMapLinearSamplerIndex, NvCo::Dx12RenderTarget* shadowMap, bool asyncComputeBenchmark)
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
			psoDesc.VS = Dx12Blob(g_meshVS);
			psoDesc.PS = asyncComputeBenchmark ? Dx12Blob(AsyncComputeBench::g_meshPS) : Dx12Blob(g_meshPS);;

			NV_RETURN_ON_FAIL(_initPipelineState(state, winding, shadowMap, PIPELINE_STATE_LIGHT_SOLID_CULL_FRONT, signiture.Get(), psoDesc));
			NV_RETURN_ON_FAIL(_initPipelineState(state, winding, shadowMap, PIPELINE_STATE_LIGHT_SOLID_CULL_BACK, signiture.Get(), psoDesc));
			NV_RETURN_ON_FAIL(_initPipelineState(state, winding, shadowMap, PIPELINE_STATE_LIGHT_SOLID_CULL_NONE, signiture.Get(), psoDesc));
			NV_RETURN_ON_FAIL(_initPipelineState(state, winding, shadowMap, PIPELINE_STATE_LIGHT_WIREFRAME, signiture.Get(), psoDesc));
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
			desc.NumStaticSamplers = 0;
			desc.pStaticSamplers = nullptr;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ComPtr<ID3DBlob> sigBlob;
			NV_RETURN_ON_FAIL(Dx12HelperUtil::serializeRootSigniture(desc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob));
			NV_RETURN_ON_FAIL(device->CreateRootSignature(0u, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&signiture)));
		}

		{
			psoDesc.VS = Dx12Blob(g_meshVS);
			psoDesc.PS = Dx12Blob(g_meshPS_Shadow);
			
			NV_RETURN_ON_FAIL(_initPipelineState(state, winding, shadowMap, PIPELINE_STATE_SHADOW_CULL_BACK, signiture.Get(), psoDesc));
			NV_RETURN_ON_FAIL(_initPipelineState(state, winding, shadowMap, PIPELINE_STATE_SHADOW_CULL_NONE, signiture.Get(), psoDesc));
		}
	}

	return NV_OK;
}

int MeshRenderPipelineD3D12::_initPipelineState(const RenderStateD3D12& state, FrontWindingType winding, NvCo::Dx12RenderTarget* shadowMap, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	ID3D12Device* device = state.m_device;

	_initRasterizerDesc(pipeType, winding, psoDesc.RasterizerState);
	_initPipelineStateDesc(pipeType, shadowMap, state.m_renderContext, psoDesc);

	psoDesc.pRootSignature = signiture;

	PipelineStateD3D12& pipeState = m_states[pipeType];
	pipeState.m_rootSignature = signiture;
	
	NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
	return NV_OK;
}

int MeshRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	const MeshDrawParamsD3D& params = *(MeshDrawParamsD3D*)paramsIn;

	NvCo::Dx12CircularResourceHeap::Cursor cursor;
	{
		Hlsl::MeshShaderConst constBuf;
		RenderParamsUtilD3D::calcMeshConstantBuffer(params, constBuf);
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

	if (isShadow(pipeType))
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

int MeshRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	return MeshRendererD3D12::defaultDraw(allocIn, sizeOfAlloc, platformState);
}

} // namespace FlexSample