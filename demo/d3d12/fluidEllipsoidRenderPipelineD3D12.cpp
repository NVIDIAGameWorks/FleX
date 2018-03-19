#define NOMINMAX

#include <NvCoDx12HelperUtil.h>

#include <external/D3D12/include/d3dx12.h>

#include "pipelineUtilD3D12.h"
#include "meshRenderer.h"
#include "bufferD3D12.h"
#include "meshRendererD3D12.h"
#include "../d3d/shaderCommonD3D.h"

#include "../d3d/shaders/ellipsoidDepthVS.hlsl.h"
#include "../d3d/shaders/ellipsoidDepthGS.hlsl.h"
#include "../d3d/shaders/ellipsoidDepthPS.hlsl.h"

// this
#include "fluidEllipsoidRenderPipelineD3D12.h"

namespace FlexSample {

static const D3D12_INPUT_ELEMENT_DESC AnisotropicInputElementDescs[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "U", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "W", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

FluidEllipsoidRenderPipelineD3D12::FluidEllipsoidRenderPipelineD3D12():
	Parent(PRIMITIVE_POINT)
{
}

static void _initPipelineStateDesc(FluidEllipsoidRenderPipelineD3D12::PipelineStateType type, NvCo::Dx12RenderTarget* renderTarget, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	PipelineUtilD3D::initTargetFormat(renderTarget, nullptr, psoDesc);

	psoDesc.InputLayout.NumElements = _countof(AnisotropicInputElementDescs);
	psoDesc.InputLayout.pInputElementDescs = AnisotropicInputElementDescs;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
}

int FluidEllipsoidRenderPipelineD3D12::initialize(const RenderStateD3D12& state, const std::wstring& shadersPath, NvCo::Dx12RenderTarget* renderTarget)
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
			psoDesc.VS = Dx12Blob(g_ellipsoidDepthVS);
			psoDesc.GS = Dx12Blob(g_ellipsoidDepthGS);
			psoDesc.PS = Dx12Blob(g_ellipsoidDepthPS);

			NV_RETURN_ON_FAIL(_initPipelineState(state, FRONT_WINDING_COUNTER_CLOCKWISE, renderTarget, PIPELINE_NORMAL, signiture.Get(), psoDesc));
		}
	}

	return NV_OK;
}

int FluidEllipsoidRenderPipelineD3D12::_initPipelineState(const RenderStateD3D12& state, FrontWindingType winding, NvCo::Dx12RenderTarget* renderTarget, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	ID3D12Device* device = state.m_device;

	PipelineUtilD3D::initRasterizerDesc(winding, psoDesc.RasterizerState);

	_initPipelineStateDesc(pipeType, renderTarget, psoDesc);

	psoDesc.pRootSignature = signiture;

	PipelineStateD3D12& pipeState = m_states[pipeType];
	pipeState.m_rootSignature = signiture;
	
	NV_RETURN_ON_FAIL(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState.m_pipelineState)));
	return NV_OK;
}

int FluidEllipsoidRenderPipelineD3D12::bind(const void* paramsIn, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	const FluidDrawParamsD3D& params = *(FluidDrawParamsD3D*)paramsIn;

	PipelineStateType pipeType = getPipelineStateType(params.renderStage, params.renderMode, params.cullMode);
	PipelineStateD3D12& pipeState = m_states[pipeType];
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

	ID3D12DescriptorHeap* heaps[] = { nullptr };
	commandList->SetDescriptorHeaps(0, heaps);

	return NV_OK;
}

int FluidEllipsoidRenderPipelineD3D12::draw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	typedef PointRenderAllocationD3D12 Alloc;
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;

	assert(sizeof(Alloc) == sizeOfAlloc);
	const Alloc& alloc = static_cast<const Alloc&>(allocIn);

	assert(allocIn.m_numPrimitives >= 0);

	ID3D12GraphicsCommandList* commandList = state.m_commandList;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[4] = 
	{
		alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION],
		alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1],
		alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY2],
		alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY3],
	};

	commandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);
	commandList->IASetIndexBuffer(&alloc.m_indexBufferView);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	commandList->DrawIndexedInstanced((UINT)allocIn.m_numPrimitives, 1, (UINT)allocIn.m_offset, 0, 0);
	return NV_OK;
}

} // namespace FlexSample