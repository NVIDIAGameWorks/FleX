/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

enum AppGraphCtxType
{
	APP_CONTEXT_D3D11 = 1,
	APP_CONTEXT_D3D12 = 2
};

void loadModules(AppGraphCtxType type);
void unloadModules();

extern void loadImgui(AppGraphCtxType type);
extern void unloadImgui();

extern void loadAppGraphCtx(AppGraphCtxType type);
extern void unloadAppGraphCtx();
