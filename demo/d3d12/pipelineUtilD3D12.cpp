#include "pipelineUtilD3D12.h"

namespace NvCo = nvidia::Common;

namespace FlexSample {

void PipelineUtilD3D::initRasterizerDesc(FrontWindingType winding, D3D12_RASTERIZER_DESC& desc)
{
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.CullMode = D3D12_CULL_MODE_NONE;

	desc.FrontCounterClockwise = (winding == FRONT_WINDING_COUNTER_CLOCKWISE);

	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void PipelineUtilD3D::initSolidBlendDesc(D3D12_BLEND_DESC& desc)
{
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;
	{
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}
}

void PipelineUtilD3D::initTargetFormat(NvCo::Dx12RenderTarget* renderTarget, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;

	// Normally single render target
	psoDesc.NumRenderTargets = 1;

	if (renderTarget)
	{
		psoDesc.RTVFormats[0] = renderTarget->getTargetFormat(NvCo::Dx12RenderTarget::BUFFER_TARGET);
		psoDesc.DSVFormat = renderTarget->getTargetFormat(NvCo::Dx12RenderTarget::BUFFER_DEPTH_STENCIL);
		psoDesc.NumRenderTargets = renderTarget->getNumRenderTargets();
		psoDesc.SampleDesc.Count = 1;	
	}
	else
	{
		const NvCo::Dx12TargetInfo& targetInfo = renderContext->m_targetInfo;

		psoDesc.RTVFormats[0] = targetInfo.m_renderTargetFormats[0]; 
		psoDesc.DSVFormat = targetInfo.m_depthStencilFormat; 
		psoDesc.SampleDesc.Count = targetInfo.m_numSamples; 
	}

	// If no depth buffer, disable 
	if (psoDesc.DSVFormat == DXGI_FORMAT_UNKNOWN)
	{
		psoDesc.DepthStencilState.DepthEnable = FALSE;
	}
}

} // namespace FlexSample
