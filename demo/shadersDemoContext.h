/*
 * Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include "shaders.h"

#include "demoContext.h"

// This file implements the Shaders.h 'interface' through the DemoContext interface
void CreateDemoContext(int type);	// 0 = OpenGL, 1 = DX11, 2 = DX12
DemoContext* GetDemoContext();
