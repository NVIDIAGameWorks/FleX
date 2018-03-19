/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef FLUID_THICKNESS_RENDER_PIPELINE_D3D12_H
#define FLUID_THICKNESS_RENDER_PIPELINE_D3D12_H

#include <DirectXMath.h>
#include "RenderStateD3D12.h"
#include "../d3d/renderParamsD3D.h"
#include "meshRenderer.h"

#include <NvCoDx12RenderTarget.h>

namespace FlexHlsl {
struct FluidShaderConst;
} // namespace FlexHlsl

namespace FlexSample {
using namespace nvidia;

struct FluidThicknessRenderPipelineD3D12: public RenderPipeline
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS(FluidThicknessRenderPipelineD3D12, RenderPipeline);
public:
	typedef RenderPipeline Parent;

	enum PipelineStateType
	{
		PIPELINE_NORMAL,
		PIPELINE_STATE_COUNT_OF,
	};

	FluidThicknessRenderPipelineD3D12();
	
		/// Initialize
	int initialize(const RenderStateD3D12& state, const std::wstring& shadersDir, NvCo::Dx12RenderTarget* renderTarget);
		/// Do the binding
	virtual int bind(const void* paramsIn, const void* platformState) override;
	virtual int draw(const RenderAllocation& alloc, size_t sizeOfAlloc, const void* platformState) override;

		/// Convert into a single pipeline state type
	static PipelineStateType getPipelineStateType(FluidDrawStage stage, FluidRenderMode mode, FluidCullMode cull) { return PIPELINE_NORMAL; }
	
	protected:

	int _initPipelineState(const RenderStateD3D12& state, FrontWindingType winding, NvCo::Dx12RenderTarget* renderTarget, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);

	PipelineStateD3D12 m_states[PIPELINE_STATE_COUNT_OF];
};

} // namespace FlexSample

#endif