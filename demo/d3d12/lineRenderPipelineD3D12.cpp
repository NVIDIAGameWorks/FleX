#define NOMINMAX

#include <NvCoDx12HelperUtil.h>
#include <external/D3D12/include/d3dx12.h>

#include "pipelineUtilD3D12.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

// this
#include "lineRenderPipelineD3D12.h"

// Shaders
#include "../d3d/shaders/debugLineVS.hlsl.h"
#include "../d3d/shaders/debugLinePS.hlsl.h"

namespace FlexSample {

static const D3D12_INPUT_ELEMENT_DESC InputElementDescs[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(Vec3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

LineRenderPipelineD3D12::LineRenderPipelineD3D12():
	Parent(PRIMITIVE_LINE)
{
}

int LineRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath, NvCo::Dx12RenderTarget* shadowMap)
{
	using namespace NvCo;
	ID3D12Device* device = state.m_device;

	// create the pipeline state object
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		PipelineUtilD3D::initSolidBlendDesc(psoDesc.BlendState);
		PipelineUtilD3D::initRasterizerDesc(FRONT_WINDING_CLOCKWISE, psoDesc.RasterizerState);

		{
			PipelineUtilD3D::initTargetFormat(nullptr, state.m_renderContext, psoDesc);

			psoDesc.InputLayout.NumElements = _countof(InputElementDescs);
			psoDesc.InputLayout.pInputElementDescs = InputElementDescs;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		}

		//psoDesc.RasterizerState.MultisampleEnable = TRUE;
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		
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

		psoDesc.VS = Dx12Blob(g_debugLineVS);
		psoDesc.PS = Dx12Blob(g_debugLinePS);
		psoDesc.pRootSignature = signiture.Get();

		{			
			PipelineStateD3D12& pipeState = m_states[PIPELINE_STATE_NORMAL];
			pipeState.m_rootSignature = signiture.Get();
			NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
		}
		{
			// Setup for shadow
			PipelineUtilD3D::initTargetFormat(shadowMap, nullptr, psoDesc);

			PipelineStateD3D12& pipeState = m_states[PIPELINE_STATE_SHADOW];
			pipeState.m_rootSignature = signiture.Get();
			// In D3D12, debug will warn that render target is not bounded and writes to RTV are discarded. This is the correct behavior as what we want is the depth buffer.
			NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
		}
	}

	return NV_OK;
}

int LineRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	LineDrawParams& params = *(LineDrawParams*)paramsIn;

	// Set up constant buffer
	NvCo::Dx12CircularResourceHeap::Cursor cursor = state.m_constantHeap->newConstantBuffer(params);
	if (!cursor.isValid())
	{
		return NV_FAIL;
	}
	
	PipelineStateD3D12& pipeState = (params.m_drawStage == LINE_DRAW_NORMAL) ?  m_states[PIPELINE_STATE_NORMAL] : m_states[PIPELINE_STATE_SHADOW];
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

int LineRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	return MeshRendererD3D12::defaultDraw(allocIn, sizeOfAlloc, platformState);
}

} // namespace FlexSample