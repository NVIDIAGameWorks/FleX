#define NOMINMAX

#include <NvCoDx12HelperUtil.h>
#include <external/D3D12/include/d3dx12.h>

#include "pipelineUtilD3D12.h"
#include "meshRenderer.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

// Shaders
#include "../d3d/shaders/passThroughVS.hlsl.h"
#include "../d3d/shaders/compositePS.hlsl.h"

// this
#include "fluidCompositeRenderPipelineD3D12.h"

#define NOMINMAX
#include <wrl.h>
using namespace Microsoft::WRL;

namespace FlexSample {

FluidCompositeRenderPipelineD3D12::FluidCompositeRenderPipelineD3D12():
	Parent(PRIMITIVE_TRIANGLE)
{
}

static void _initPipelineStateDesc(FluidCompositeRenderPipelineD3D12::PipelineStateType type, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	PipelineUtilD3D::initTargetFormat(nullptr, renderContext, psoDesc);

	psoDesc.InputLayout.NumElements = _countof(MeshRendererD3D12::MeshInputElementDescs);
	psoDesc.InputLayout.pInputElementDescs = MeshRendererD3D12::MeshInputElementDescs;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

int FluidCompositeRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath)
{
	using namespace NvCo;
	ID3D12Device* device = state.m_device;

	// create the pipeline state object
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		PipelineUtilD3D::initSolidBlendDesc(psoDesc.BlendState);

		// create the root signature
		ComPtr<ID3D12RootSignature> signiture;
		{
			CD3DX12_DESCRIPTOR_RANGE ranges[2];
			CD3DX12_ROOT_PARAMETER params[10];
			
			int rangeIndex = 0;
			UINT rootIndex = 0;
			{
				D3D12_ROOT_PARAMETER& param = params[rootIndex++];
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				param.Descriptor.ShaderRegister = 0u;
				param.Descriptor.RegisterSpace = 0u;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}

			// depthTex, sceneTex, shadowTex
			const int numSrvs = 3;
			if (numSrvs > 0)
			{
				ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSrvs, 0);
				params[rootIndex++].InitAsDescriptorTable(1, &ranges[rangeIndex], D3D12_SHADER_VISIBILITY_PIXEL);
				rangeIndex++;
			}

			const int numSamplers = 2;
			if (numSamplers > 1)
			{
				ranges[rangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, numSamplers, 0);
				params[rootIndex++].InitAsDescriptorTable(1, &ranges[rangeIndex], D3D12_SHADER_VISIBILITY_PIXEL);
				rangeIndex++;
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
			psoDesc.VS = Dx12Blob(g_passThroughVS);
			psoDesc.PS = Dx12Blob(g_compositePS);

			NV_RETURN_ON_FAIL(_initPipelineState(state, PIPELINE_STATE_NORMAL, signiture.Get(), psoDesc));
		}
	}

	return NV_OK;
}

int FluidCompositeRenderPipelineD3D12::_initPipelineState(const RenderStateD3D12& state, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	ID3D12Device* device = state.m_device;

	PipelineUtilD3D::initRasterizerDesc(FRONT_WINDING_CLOCKWISE, psoDesc.RasterizerState);
	_initPipelineStateDesc(pipeType, state.m_renderContext, psoDesc);

	psoDesc.pRootSignature = signiture;

	PipelineStateD3D12& pipeState = m_states[pipeType];
	pipeState.m_rootSignature = signiture;
	
	NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
	return NV_OK;
}

int FluidCompositeRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	const FluidDrawParamsD3D& params = *(FluidDrawParamsD3D*)paramsIn;

	PipelineStateD3D12& pipeState = m_states[PIPELINE_STATE_NORMAL];
	if (!pipeState.isValid())
	{
		return NV_FAIL;
	}

	NvCo::Dx12CircularResourceHeap::Cursor cursor;
	{
		Hlsl::FluidShaderConst constBuf;
		RenderParamsUtilD3D::calcFluidCompositeConstantBuffer(params, constBuf);
		cursor = state.m_constantHeap->newConstantBuffer(constBuf);
		if (!cursor.isValid())
		{
			return NV_FAIL;
		}
	}

	ID3D12GraphicsCommandList* commandList = state.m_commandList;

	commandList->SetGraphicsRootSignature(pipeState.m_rootSignature.Get());
	commandList->SetPipelineState(pipeState.m_pipelineState.Get());

	D3D12_GPU_VIRTUAL_ADDRESS cbvHandle = state.m_constantHeap->getGpuHandle(cursor);
	commandList->SetGraphicsRootConstantBufferView(0, cbvHandle);

	ID3D12DescriptorHeap* heaps[] = { state.m_srvCbvUavDescriptorHeap->getHeap(), state.m_samplerDescriptorHeap->getHeap() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// Bind the srvs
	commandList->SetGraphicsRootDescriptorTable(1, state.m_srvCbvUavDescriptorHeap->getGpuHandle(params.m_srvDescriptorBase));
	// Bind the samplers
	commandList->SetGraphicsRootDescriptorTable(2, state.m_samplerDescriptorHeap->getGpuHandle(params.m_sampleDescriptorBase));
	return NV_OK;
}

int FluidCompositeRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	return MeshRendererD3D12::defaultDraw(allocIn, sizeOfAlloc, platformState);
}

} // namespace FlexSample