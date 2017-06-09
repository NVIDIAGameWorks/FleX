/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#include "NvCoDxFormatUtil.h"

#include <stdio.h>

namespace nvidia {
namespace Common {

/* static */DXGI_FORMAT DxFormatUtil::calcResourceFormat(UsageType usage, int usageFlags, DXGI_FORMAT format)
{
	(void)usage;
	if (usageFlags)
	{
		switch (format)
		{
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_D32_FLOAT:			return DXGI_FORMAT_R32_TYPELESS;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:	return DXGI_FORMAT_R24G8_TYPELESS;
			default: break;
		}
		return format;
	}
	return format;
}

/* static */DXGI_FORMAT DxFormatUtil::calcFormat(UsageType usage, DXGI_FORMAT format)
{
	switch (usage)
	{
		case USAGE_COUNT_OF:
		case USAGE_UNKNOWN: 
		{
			return DXGI_FORMAT_UNKNOWN;
		}
		case USAGE_DEPTH_STENCIL:
		{
			switch (format)
			{
				case DXGI_FORMAT_D32_FLOAT:
				case DXGI_FORMAT_R32_TYPELESS:			return DXGI_FORMAT_D32_FLOAT;
				case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:	return DXGI_FORMAT_D24_UNORM_S8_UINT;
				case DXGI_FORMAT_R24G8_TYPELESS:		return DXGI_FORMAT_D24_UNORM_S8_UINT;
				default: break;
			}
			return format;
		}
		case USAGE_TARGET:
		{
			switch (format)
			{
				case DXGI_FORMAT_D32_FLOAT:
				case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_UNKNOWN;
				case DXGI_FORMAT_R32_TYPELESS:		return DXGI_FORMAT_R32_FLOAT;
				default: break;
			}
			return format;
		}
		case USAGE_SRV:
		{
			switch (format)
			{
				case DXGI_FORMAT_D32_FLOAT:
				case DXGI_FORMAT_R32_TYPELESS:			return DXGI_FORMAT_R32_FLOAT;
				case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:	return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				default: break;
			}

			return format;
		}
	}

	printf("Not reachable");
	return DXGI_FORMAT_UNKNOWN;
}

bool DxFormatUtil::isTypeless(DXGI_FORMAT format)
{
	switch (format)
	{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_R24G8_TYPELESS: 
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC7_TYPELESS:
		{
			return true;
		}
		default: break;
	}
	return false;
}

/* static */int DxFormatUtil::getNumColorChannelBits(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 32;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 16;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			return 10;

		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return 8;

		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
			return 5;

		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 4;

		default:
			return 0;
	}
}


} // namespace Common 
} // namespace nvidia 
