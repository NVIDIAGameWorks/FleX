/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef DEMO_CONTEXT_H
#define DEMO_CONTEXT_H

#include "shaders.h"

//#include "../d3d/imguiGraph.h"

class DemoContext
{
	public:
		/// Returns false if couldn't start up
	virtual bool initialize(const RenderInitOptions& options) = 0;
	
	virtual void startFrame(Vec4 colorIn) = 0;
	virtual void endFrame() = 0;
	virtual void presentFrame(bool fullsync) = 0;
	
	virtual void readFrame(int* backBuffer, int width, int height) {}

	virtual void getViewRay(int x, int y, Vec3& origin, Vec3& dir) = 0;
	virtual void setView(Matrix44 view, Matrix44 projection) = 0;
	virtual void renderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug) = 0;
	
	virtual void drawMesh(const Mesh* m, Vec3 color) = 0;
	virtual void drawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth) = 0;
	virtual void drawRope(Vec4* positions, int* indices, int numIndices, float radius, int color) = 0;
	virtual void drawPlane(const Vec4& p, bool color) = 0;
	virtual void drawPlanes(Vec4* planes, int n, float bias) = 0;
	
	virtual void graphicsTimerBegin() = 0;
	virtual void graphicsTimerEnd() = 0;
	
	virtual float rendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq) = 0;

	virtual void bindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float bias, Vec4 fogColor) = 0;
	virtual void unbindSolidShader() = 0;

	virtual ShadowMap* shadowCreate() = 0;
	virtual void shadowDestroy(ShadowMap* map) = 0;
	virtual void shadowBegin(ShadowMap* map) = 0;
	virtual void shadowEnd() = 0;

	virtual FluidRenderer* createFluidRenderer(uint32_t width, uint32_t height) = 0;
	virtual void destroyFluidRenderer(FluidRenderer* renderer) = 0;

	virtual FluidRenderBuffers* createFluidRenderBuffers(int numParticles, bool enableInterop) = 0;
	virtual void drawPoints(FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowTex, bool showDensity) = 0;
	virtual void updateFluidRenderBuffers(FluidRenderBuffers* buffers, NvFlexSolver* flex, bool anisotropy, bool density) = 0;
	virtual void updateFluidRenderBuffers(FluidRenderBuffers* buffers, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices) = 0;
	virtual void destroyFluidRenderBuffers(FluidRenderBuffers* buffers) = 0;

	virtual GpuMesh* createGpuMesh(const Mesh* m) = 0;
	virtual void destroyGpuMesh(GpuMesh* mesh) = 0;
	virtual void drawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color) = 0;
	virtual void drawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color) = 0;

	virtual DiffuseRenderBuffers* createDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop) = 0;
	virtual void destroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers) = 0;
	virtual void updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles) = 0;
	virtual void updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, NvFlexSolver* solver) = 0;
	virtual int getNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers) = 0;

	virtual void drawDiffuse(FluidRenderer* render, const DiffuseRenderBuffers* buffers, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front) = 0;

	virtual void beginLines() = 0;
	virtual void drawLine(const Vec3& p, const Vec3& q, const Vec4& color) = 0;
	virtual void endLines() = 0;

	virtual void onSizeChanged(int width, int height, bool minimized) = 0;

	virtual void startGpuWork() = 0;
	virtual void endGpuWork() = 0;
	virtual void flushGraphicsAndWait() = 0;

	virtual void setFillMode(bool wire) = 0;
	virtual void setCullMode(bool enabled) = 0;

	virtual void drawImguiGraph() = 0;
	virtual void* getGraphicsCommandQueue() { return nullptr; }
	virtual void getRenderDevice(void** device, void** context) = 0;

	virtual ~DemoContext() {}
};

#endif // DEMO_CONTEXT_H