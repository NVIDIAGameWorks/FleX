/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef LINE_RENDER_PIPELINE_D3D12_H
#define LINE_RENDER_PIPELINE_D3D12_H

#include <DirectXMath.h>
#include "renderStateD3D12.h"
#include "meshRenderer.h"

#include <NvCoDx12RenderTarget.h>

#include <string>

namespace FlexSample {
using namespace nvidia;

enum LineDrawStage
{
	LINE_DRAW_NORMAL,
	LINE_DRAW_SHADOW,
};

struct LineDrawParams
{
	Hlsl::float4x4 m_modelWorldProjection;	/// Transforms point from world-space to clip space
	LineDrawStage m_drawStage;
};

struct LineRenderPipelineD3D12: public RenderPipeline
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS(LineRenderPipelineD3D12, RenderPipeline);
public:
	typedef RenderPipeline Parent;

	enum PipelineStateType
	{		
		PIPELINE_STATE_NORMAL,				
		PIPELINE_STATE_SHADOW,
		PIPELINE_STATE_COUNT_OF,
	};

	LineRenderPipelineD3D12();
	
		/// Initialize
	int initialize(const RenderStateD3D12& state, const std::wstring& shadersDir, NvCo::Dx12RenderTarget* shadowMap);
		/// Do the binding
	virtual int bind(const void* paramsIn, const void* platformState) override;
	virtual int draw(const RenderAllocation& alloc, size_t sizeOfAlloc, const void* platformState) override;

	protected:

	PipelineStateD3D12 m_states[PIPELINE_STATE_COUNT_OF];
};

} // namespace FlexSample

#endif