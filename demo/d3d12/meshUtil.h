#ifndef MESH_UTIL_H
#define MESH_UTIL_H

// Needed for mesh.h
#pragma warning( disable : 4458 )

#include "meshRenderer.h"

#include <core/mesh.h>

typedef ::Vec4 FlexVec4;
typedef ::Vec3 FlexVec3;
typedef ::Vec2 FlexVec2;

namespace FlexSample {
//using namespace nvidia;

/* Tools/types to implify use of the flex 'Mesh' types */
struct MeshUtil
{
	static RenderMesh* createRenderMesh(MeshRenderer* renderer, const Mesh& mesh);
};

} // namespace FlexSample

#endif