/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once


struct DemoContextOGL : public DemoContext
{
public:

	// DemoContext Impl
	virtual bool initialize(const RenderInitOptions& options);
	virtual void startFrame(Vec4 colorIn);
	virtual void endFrame();
	virtual void presentFrame(bool fullsync);
	virtual void readFrame(int* buffer, int width, int height);
	virtual void getViewRay(int x, int y, Vec3& origin, Vec3& dir);
	virtual void setView(Matrix44 view, Matrix44 projection);
	virtual void renderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug);
	virtual void drawMesh(const Mesh* m, Vec3 color);
	virtual void drawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth);
	virtual void drawRope(Vec4* positions, int* indices, int numIndices, float radius, int color);
	virtual void drawPlane(const Vec4& p, bool color);
	virtual void drawPlanes(Vec4* planes, int n, float bias);
	virtual void drawPoints(FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowTex, bool showDensity);
	virtual void graphicsTimerBegin();
	virtual void graphicsTimerEnd();
	virtual float rendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq);
	virtual void bindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float bias, Vec4 fogColor);
	virtual void unbindSolidShader();
	virtual ShadowMap* shadowCreate();
	virtual void shadowDestroy(ShadowMap* map);
	virtual void shadowBegin(ShadowMap* map);
	virtual void shadowEnd();
	virtual FluidRenderer* createFluidRenderer(uint32_t width, uint32_t height);
	virtual void destroyFluidRenderer(FluidRenderer* renderer);
	virtual FluidRenderBuffers* createFluidRenderBuffers(int numParticles, bool enableInterop);
	virtual void updateFluidRenderBuffers(FluidRenderBuffers* buffers, NvFlexSolver* flex, bool anisotropy, bool density);
	virtual void updateFluidRenderBuffers(FluidRenderBuffers* buffers, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices);
	virtual void destroyFluidRenderBuffers(FluidRenderBuffers* buffers);
	virtual GpuMesh* createGpuMesh(const Mesh* m);
	virtual void destroyGpuMesh(GpuMesh* mesh);
	virtual void drawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color);
	virtual void drawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color);
	virtual DiffuseRenderBuffers* createDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop);
	virtual void destroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers);
	virtual void updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles);
	virtual void updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, NvFlexSolver* solver);
	virtual void drawDiffuse(FluidRenderer* render, const DiffuseRenderBuffers* buffers, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front);
	virtual int getNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers);
	virtual void beginLines();
	virtual void drawLine(const Vec3& p, const Vec3& q, const Vec4& color);
	virtual void endLines();
	virtual void onSizeChanged(int width, int height, bool minimized);
	virtual void startGpuWork();
	virtual void endGpuWork();
	virtual void flushGraphicsAndWait();
	virtual void setFillMode(bool wire);
	virtual void setCullMode(bool enabled);
	virtual void drawImguiGraph();
	virtual void* getGraphicsCommandQueue();
	virtual void getRenderDevice(void** device, void** context);
};
