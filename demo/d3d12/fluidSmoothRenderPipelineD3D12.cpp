#define NOMINMAX

#include <NvCoDx12HelperUtil.h>
#include <external/D3D12/include/d3dx12.h>

#include "meshRenderer.h"
#include "pipelineUtilD3D12.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

// this
#include "fluidSmoothRenderPipelineD3D12.h"

// Shaders
#include "../d3d/shaders/passThroughVS.hlsl.h"
#include "../d3d/shaders/blurDepthPS.hlsl.h"

namespace FlexSample {

FluidSmoothRenderPipelineD3D12::FluidSmoothRenderPipelineD3D12(int fluidPointDepthSrvIndex):
	Parent(PRIMITIVE_TRIANGLE),
	m_fluidPointDepthSrvIndex(fluidPointDepthSrvIndex)
{
}

static void _initPipelineStateDesc(FluidSmoothRenderPipelineD3D12::PipelineStateType type, NvCo::Dx12RenderTarget* renderTarget, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	PipelineUtilD3D::initTargetFormat(renderTarget, nullptr, psoDesc);

	psoDesc.InputLayout.NumElements = _countof(MeshRendererD3D12::MeshInputElementDescs);
	psoDesc.InputLayout.pInputElementDescs = MeshRendererD3D12::MeshInputElementDescs;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

int FluidSmoothRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath, NvCo::Dx12RenderTarget* renderTarget)
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
			CD3DX12_ROOT_PARAMETER params[3];
			
			UINT rootIndex = 0;
			{
				D3D12_ROOT_PARAMETER& param = params[rootIndex++];
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				param.Descriptor.ShaderRegister = 0u;
				param.Descriptor.RegisterSpace = 0u;
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}

			const int numSrvs = 2;
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
			desc.NumStaticSamplers = 0u;
			desc.pStaticSamplers = nullptr;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ComPtr<ID3DBlob> sigBlob;
			NV_RETURN_ON_FAIL(Dx12HelperUtil::serializeRootSigniture(desc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob));
			NV_RETURN_ON_FAIL(device->CreateRootSignature(0u, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&signiture)));
		}

		{
			psoDesc.VS = Dx12Blob(g_passThroughVS);
			psoDesc.PS = Dx12Blob(g_blurDepthPS);

			NV_RETURN_ON_FAIL(_initPipelineState(state, renderTarget, PIPELINE_STATE_NORMAL, signiture.Get(), psoDesc));
		}
	}

	return NV_OK;
}

int FluidSmoothRenderPipelineD3D12::_initPipelineState(const RenderStateD3D12& state, NvCo::Dx12RenderTarget* renderTarget, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	ID3D12Device* device = state.m_device;

	PipelineUtilD3D::initRasterizerDesc(FRONT_WINDING_CLOCKWISE, psoDesc.RasterizerState);
	_initPipelineStateDesc(pipeType, renderTarget, state.m_renderContext, psoDesc);

	psoDesc.pRootSignature = signiture;

	PipelineStateD3D12& pipeState = m_states[pipeType];
	pipeState.m_rootSignature = signiture;
	
	NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
	return NV_OK;
}

int FluidSmoothRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
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
		RenderParamsUtilD3D::calcFluidConstantBuffer(params, constBuf);
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

	NvCo::Dx12RenderTarget* sourceMap = (NvCo::Dx12RenderTarget*)params.shadowMap;

	ID3D12DescriptorHeap* heaps[] = { state.m_srvCbvUavDescriptorHeap->getHeap(), state.m_samplerDescriptorHeap->getHeap() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// Bind the srvs
	commandList->SetGraphicsRootDescriptorTable(1, state.m_srvCbvUavDescriptorHeap->getGpuHandle(m_fluidPointDepthSrvIndex));
	// Bind the samplers
	commandList->SetGraphicsRootDescriptorTable(2, state.m_samplerDescriptorHeap->getGpuHandle(params.m_sampleDescriptorBase));
	return NV_OK;
}

int FluidSmoothRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	return MeshRendererD3D12::defaultDraw(allocIn, sizeOfAlloc, platformState);
}

} // namespace FlexSample