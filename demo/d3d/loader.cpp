/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "loader.h"

//#include <SDL.h>

#include "imgui.h"

void loadModules(AppGraphCtxType type)
{
	loadAppGraphCtx(type);
	loadImgui(type);
}

void unloadModules()
{
	unloadImgui();
	unloadAppGraphCtx();
}