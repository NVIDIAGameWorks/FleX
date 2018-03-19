/*
* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

// Library stuff...
#define GFSDK_Aftermath_PFN typedef GFSDK_Aftermath_Result

#ifdef EXPORTS
#define GFSDK_Aftermath_DLLSPEC __declspec(dllexport) 
#else
#define GFSDK_Aftermath_DLLSPEC
#endif

#define GFSDK_Aftermath_API extern "C" GFSDK_Aftermath_DLLSPEC GFSDK_Aftermath_Result
