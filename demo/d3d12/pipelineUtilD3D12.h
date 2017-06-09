/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef PIPELINE_UTIL_D3D12_H
#define PIPELINE_UTIL_D3D12_H

#define NOMINMAX
#include <dxgi.h>
#include <d3d12.h>

#include "meshRenderer.h"
#include <NvCoDx12RenderTarget.h>

namespace FlexSample {
using namespace nvidia;

struct PipelineUtilD3D
{
		/// Default initializes rasterizer
	static void initRasterizerDesc(FrontWindingType winding, D3D12_RASTERIZER_DESC& desc);
		/// Initialize default blend desc for solid rendering
	static void initSolidBlendDesc(D3D12_BLEND_DESC& desc);
		/// Set on psoDesc the format/samples and set up other default state.
		/// If renderTarget is NV_NULL then the format that's set on the Dx12RenderInterface/RenderContext is used
	static void initTargetFormat(Common::Dx12RenderTarget* renderTarget, AppGraphCtxD3D12* renderContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
};

} // namespace FlexSample

#endif