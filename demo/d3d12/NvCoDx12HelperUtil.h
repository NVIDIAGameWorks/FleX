/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_CO_DX12_HELPER_UTIL_H
#define NV_CO_DX12_HELPER_UTIL_H

#include <NvResult.h>
#include <NvCoDxFormatUtil.h>
#include <NvCoDxDebugUtil.h>

#define NOMINMAX
#include <d3d12.h>
#include <stdio.h>
#include <wrl.h>

using namespace Microsoft::WRL;

/** \addtogroup common
@{
*/

namespace nvidia {
namespace Common { 

struct Dx12HelperUtil
{
		/// Get a Shader resource view from a resourceDesc
		/// If pixelFormat is UNKNOWN, will use the pixelFormat in the resourceDesc
	static void calcSrvDesc(const D3D12_RESOURCE_DESC& resourceDesc, DXGI_FORMAT pixelFormat, D3D12_SHADER_RESOURCE_VIEW_DESC& descOut);

		/// Get a Shader resource view from a resource
		/// If pixelFormat is UNKNOWN, will use the pixelFormat in the resourceDesc
	static void calcSrvDesc(ID3D12Resource* resource, DXGI_FORMAT pixelFormat, D3D12_SHADER_RESOURCE_VIEW_DESC& descOut);

		/// Serializes the root signiture. If any errors occur will write to the log
	static int serializeRootSigniture(const D3D12_ROOT_SIGNATURE_DESC& desc, D3D_ROOT_SIGNATURE_VERSION signitureVersion, ComPtr<ID3DBlob>& sigBlobOut);

		/// Reports to the output debug window any live objects, using debug interface. 
		/// Only works on systems that have "Dxgidebug.dll"
	static void reportLiveObjects();
};

} // namespace Common
} // namespace nvidia

/** @} */

#endif // NV_CO_DX12_DEBUG_UTIL_H
