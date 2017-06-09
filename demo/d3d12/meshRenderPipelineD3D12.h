/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef MESH_RENDER_PIPELINE_D3D12_H
#define MESH_RENDER_PIPELINE_D3D12_H

#include <DirectXMath.h>
#include "renderStateD3D12.h"
#include "../d3d/shaderCommonD3D.h"
#include "meshRenderer.h"
#include "../d3d/renderParamsD3D.h"

#include <NvCoDx12RenderTarget.h>

namespace FlexSample {
using namespace nvidia;

struct MeshRenderPipelineD3D12: public RenderPipeline
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS(MeshRenderPipelineD3D12, RenderPipeline);
public:
	typedef RenderPipeline Parent;

	enum PipelineStateType
	{
		PIPELINE_STATE_SHADOW_CULL_BACK,			// Is cull back 
		PIPELINE_STATE_SHADOW_CULL_NONE,			
		PIPELINE_STATE_LIGHT_WIREFRAME,				// No culling
		PIPELINE_STATE_LIGHT_SOLID_CULL_FRONT,
		PIPELINE_STATE_LIGHT_SOLID_CULL_BACK,
		PIPELINE_STATE_LIGHT_SOLID_CULL_NONE,
		PIPELINE_STATE_COUNT_OF,
	};

	MeshRenderPipelineD3D12();
	
		/// Initialize
	int initialize(const RenderStateD3D12& state, const std::wstring& shadersDir, FrontWindingType winding, int shadowMapLinearSamplerIndex, NvCo::Dx12RenderTarget* shadowMap, bool asyncComputeBenchmark);
		/// Do the binding
	virtual int bind(const void* paramsIn, const void* platformState) override;
	virtual int draw(const RenderAllocation& alloc, size_t sizeOfAlloc, const void* platformState) override;

		/// Convert into a single pipeline state type
	static PipelineStateType getPipelineStateType(MeshDrawStage stage, MeshRenderMode mode, MeshCullMode cull);
		/// true if it's a shadowing type
	static bool isShadow(PipelineStateType type) { return type == PIPELINE_STATE_SHADOW_CULL_BACK || type == PIPELINE_STATE_SHADOW_CULL_NONE; }

	protected:

	int _initPipelineState(const RenderStateD3D12& state, FrontWindingType winding, NvCo::Dx12RenderTarget* shadowMap, PipelineStateType pipeType, ID3D12RootSignature* signiture, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);

	int m_shadowMapLinearSamplerIndex;			//< The index to the linear sampler in the m_samplerHeap

	PipelineStateD3D12 m_states[PIPELINE_STATE_COUNT_OF];
};

} // namespace FlexSample

#endif