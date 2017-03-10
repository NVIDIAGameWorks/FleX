// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2013-2017 NVIDIA Corporation. All rights reserved.

#pragma once

// disable some warnings
#if _WIN32
#pragma warning(disable: 4267)  // conversion from 'size_t' to 'int', possible loss of data
#endif

class Scene
{
public:

	Scene(const char* name) : mName(name) {}
	
	virtual void Initialize() = 0;
	virtual void PostInitialize() {}
	
	// update any buffers (all guaranteed to be mapped here)
	virtual void Update() {}	

	// send any changes to flex (all buffers guaranteed to be unmapped here)
	virtual void Sync() {}
	
	virtual void Draw(int pass) {}
	virtual void KeyDown(int key) {}
	virtual void DoGui() {}
	virtual void CenterCamera() {}

	virtual Matrix44 GetBasis() { return Matrix44::kIdentity; }	

	virtual const char* GetName() { return mName; }

	const char* mName;
};


#include "scenes/adhesion.h"
#include "scenes/armadilloshower.h"
#include "scenes/bananas.h"
#include "scenes/bouyancy.h"
#include "scenes/bunnybath.h"
#include "scenes/ccdfluid.h"
#include "scenes/clothlayers.h"
#include "scenes/dambreak.h"
#include "scenes/darts.h"
#include "scenes/debris.h"
#include "scenes/deformables.h"
#include "scenes/envcloth.h"
#include "scenes/flag.h"
#include "scenes/fluidblock.h"
#include "scenes/fluidclothcoupling.h"
#include "scenes/forcefield.h"
#include "scenes/frictionmoving.h"
#include "scenes/frictionramp.h"
#include "scenes/gamemesh.h"
#include "scenes/googun.h"
#include "scenes/granularpile.h"
#include "scenes/granularshape.h"
#include "scenes/inflatable.h"
#include "scenes/initialoverlap.h"
#include "scenes/lighthouse.h"
#include "scenes/localspacecloth.h"
#include "scenes/localspacefluid.h"
#include "scenes/lowdimensionalshapes.h"
#include "scenes/melting.h"
#include "scenes/mixedpile.h"
#include "scenes/nonconvex.h"
#include "scenes/parachutingbunnies.h"
#include "scenes/pasta.h"
#include "scenes/plasticstack.h"
#include "scenes/player.h"
#include "scenes/potpourri.h"
#include "scenes/rayleightaylor.h"
#include "scenes/restitution.h"
#include "scenes/rigidfluidcoupling.h"
#include "scenes/rigidpile.h"
#include "scenes/rigidrotation.h"
#include "scenes/rockpool.h"
#include "scenes/sdfcollision.h"
#include "scenes/shapecollision.h"
#include "scenes/softbody.h"
#include "scenes/spherecloth.h"
#include "scenes/surfacetension.h"
#include "scenes/tearing.h"
#include "scenes/thinbox.h"
#include "scenes/trianglecollision.h"
#include "scenes/triggervolume.h"
#include "scenes/viscosity.h"
#include "scenes/waterballoon.h"
