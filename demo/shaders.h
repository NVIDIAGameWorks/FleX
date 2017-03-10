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

#define STRINGIFY(A) #A

#include "../core/maths.h"
#include "../core/mesh.h"

#include "../include/NvFlex.h"


#if FLEX_DX

#include <d3d11.h>
typedef ID3D11Buffer* VertexBuffer;
typedef ID3D11Buffer* IndexBuffer;

void GetRenderDevice(ID3D11Device** device, ID3D11DeviceContext** context);

#else

typedef unsigned int VertexBuffer;
typedef unsigned int IndexBuffer;
typedef unsigned int Texture;

#endif

struct SDL_Window;

void InitRender(SDL_Window* window, bool fullscreen, int msaa);
void DestroyRender();
void ReshapeRender(SDL_Window* window);

void StartFrame(Vec4 clearColor);
void EndFrame();

// set to true to enable vsync
void PresentFrame(bool fullsync);

void GetViewRay(int x, int y, Vec3& origin, Vec3& dir);

// read back pixel values
void ReadFrame(int* backbuffer, int width, int height);

void SetView(Matrix44 view, Matrix44 proj);
void SetFillMode(bool wireframe);
void SetCullMode(bool enabled);

// debug draw methods
void BeginLines();
void DrawLine(const Vec3& p, const Vec3& q, const Vec4& color);
void EndLines();

// shadowing
struct ShadowMap;
ShadowMap* ShadowCreate();
void ShadowDestroy(ShadowMap* map);
void ShadowBegin(ShadowMap* map);
void ShadowEnd();

// primitive draw methods
void DrawPlanes(Vec4* planes, int n, float bias);
void DrawPoints(VertexBuffer positions, VertexBuffer color, IndexBuffer indices, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, bool showDensity);
void DrawMesh(const Mesh*, Vec3 color);
void DrawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex=3, float expand=0.0f, bool twosided=true, bool smooth=true);
void DrawBuffer(float* buffer, Vec3 camPos, Vec3 lightPos);
void DrawRope(Vec4* positions, int* indices, int numIndices, float radius, int color);

struct GpuMesh;

GpuMesh* CreateGpuMesh(const Mesh* m);
void DestroyGpuMesh(GpuMesh* m);
void DrawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color);
void DrawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color);

// main lighting shader
void BindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, float bias, Vec4 fogColor);
void UnbindSolidShader();

// new fluid renderer
struct FluidRenderer;

// owns render targets and shaders associated with fluid rendering
FluidRenderer* CreateFluidRenderer(uint32_t width, uint32_t height);
void DestroyFluidRenderer(FluidRenderer*);

struct FluidRenderBuffers
{
	VertexBuffer mPositionVBO;
	VertexBuffer mDensityVBO;
	VertexBuffer mAnisotropyVBO[3];
	IndexBuffer mIndices;

	VertexBuffer mFluidVBO; // to be removed

	// wrapper buffers that allow Flex to write directly to VBOs
	NvFlexBuffer* mPositionBuf;
	NvFlexBuffer* mDensitiesBuf;	
	NvFlexBuffer* mAnisotropyBuf[3];
	NvFlexBuffer* mIndicesBuf;

	int mNumFluidParticles;
};

FluidRenderBuffers CreateFluidRenderBuffers(int numParticles, bool enableInterop);
void DestroyFluidRenderBuffers(FluidRenderBuffers buffers);

// update fluid particle buffers from a FlexSovler
void UpdateFluidRenderBuffers(FluidRenderBuffers buffers, NvFlexSolver* flex, bool anisotropy, bool density);

// update fluid particle buffers from host memory
void UpdateFluidRenderBuffers(FluidRenderBuffers buffers, 
	Vec4* particles, 
	float* densities, 
	Vec4* anisotropy1, 
	Vec4* anisotropy2, 
	Vec4* anisotropy3, 
	int numParticles, 
	int* indices, 
	int numIndices);

// vertex buffers for diffuse particles
struct DiffuseRenderBuffers
{
	VertexBuffer mDiffusePositionVBO;
	VertexBuffer mDiffuseVelocityVBO;
	IndexBuffer mDiffuseIndicesIBO;

	NvFlexBuffer* mDiffuseIndicesBuf;
	NvFlexBuffer* mDiffusePositionsBuf;
	NvFlexBuffer* mDiffuseVelocitiesBuf;

	int mNumDiffuseParticles;
};

// owns diffuse particle vertex buffers
DiffuseRenderBuffers CreateDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop);
void DestroyDiffuseRenderBuffers(DiffuseRenderBuffers buffers);

// update diffuse particle vertex buffers from a NvFlexSolver
void UpdateDiffuseRenderBuffers(DiffuseRenderBuffers buffers, NvFlexSolver* solver);

// update diffuse particle vertex buffers from host memory
void UpdateDiffuseRenderBuffers(DiffuseRenderBuffers buffers,
	Vec4* diffusePositions,
	Vec4* diffuseVelocities,
	int* diffuseIndices,
	int numDiffuseParticles);

// screen space fluid rendering
void RenderEllipsoids(FluidRenderer* render, FluidRenderBuffers buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, Vec4 color, float blur, float ior, bool debug);
void RenderDiffuse(FluidRenderer* render, DiffuseRenderBuffers buffer, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, float motionBlur,  float inscatter, float outscatter, bool shadow, bool front);

// UI rendering
void imguiGraphDraw();
void imguiGraphInit(const char* fontpath);