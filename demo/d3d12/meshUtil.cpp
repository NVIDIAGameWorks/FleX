/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "meshUtil.h"

namespace FlexSample {

/* static */RenderMesh* MeshUtil::createRenderMesh(MeshRenderer* renderer, const Mesh& mesh)
{
	int numFaces = mesh.GetNumFaces();
	int numVertices = mesh.GetNumVertices();

	MeshData data;
	data.colors = (mesh.m_colours.size() > 0) ? (const Vec4*)&mesh.m_colours[0] : nullptr;
	data.positions = (mesh.m_positions.size() > 0) ? (const Vec3*)&mesh.m_positions[0] : nullptr;
	data.normals = (mesh.m_normals.size() > 0) ? (const Vec3*)&mesh.m_normals[0] : nullptr;
	data.indices = (mesh.m_indices.size() > 0) ? (const uint32_t*)&mesh.m_indices[0] : nullptr;
	data.texcoords = (mesh.m_texcoords[0].size() > 0) ? (const Vec2*)&mesh.m_texcoords[0] : nullptr;

	data.numFaces = numFaces;
	data.numVertices = numVertices;

	return renderer->createMesh(data);
}

} // namespace FlexSample