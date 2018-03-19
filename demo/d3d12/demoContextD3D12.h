/*
* Copyright (c) 2008-2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef DEMO_CONTEXT_D3D12_H
#define DEMO_CONTEXT_D3D12_H

#include <memory>
#include <NvCoDx12RenderTarget.h>

#include "meshRenderer.h"
#include "renderStateD3D12.h"
#include "appD3D12Ctx.h"

// SDL
#include <SDL.h>
#include <SDL_video.h>

#include "shaders.h"
#include "demoContext.h"

#define NOMINMAX
#include <d3d12.h>

namespace FlexSample {  

struct FlexBuffer
{
	inline FlexBuffer():m_buffer(nullptr) {}
	inline operator NvFlexBuffer* () const { return m_buffer; }
	~FlexBuffer() { if (m_buffer) { NvFlexUnregisterD3DBuffer(m_buffer); } }

	NvFlexBuffer* m_buffer;
};

struct FluidRenderBuffersD3D12 
{
	FluidRenderBuffersD3D12(int numParticles = 0) 
	{
		m_numParticles = numParticles;
		{
			const D3D12_VERTEX_BUFFER_VIEW nullView = {};
			m_positionsView = nullView;
			m_densitiesView = nullView;
			m_fluidView = nullView;
			for (int i = 0; i < _countof(m_anisotropiesViewArr); i++)
			{
				m_anisotropiesViewArr[i] = nullView;
			}
		}
		{
			D3D12_INDEX_BUFFER_VIEW nullView = {};
			m_indicesView = nullView; 
		}
	}

	int m_numParticles;
	
	D3D12_VERTEX_BUFFER_VIEW m_positionsView;
	D3D12_VERTEX_BUFFER_VIEW m_densitiesView;
	D3D12_VERTEX_BUFFER_VIEW m_anisotropiesViewArr[3];
	D3D12_INDEX_BUFFER_VIEW m_indicesView;

	D3D12_VERTEX_BUFFER_VIEW m_fluidView; // to be removed

	// wrapper buffers that allow Flex to write directly to VBOs
	FlexBuffer m_positionsBuf;
	FlexBuffer m_densitiesBuf;
	FlexBuffer m_anisotropiesBufArr[3];
	FlexBuffer m_indicesBuf;
};

// vertex buffers for diffuse particles
struct DiffuseRenderBuffersD3D12 
{
	DiffuseRenderBuffersD3D12(int numParticles = 0)
	{
		m_numParticles = numParticles;
		{
			const D3D12_VERTEX_BUFFER_VIEW nullView = {};
			m_positionsView = nullView;
			m_velocitiesView = nullView;
		}
		{
			D3D12_INDEX_BUFFER_VIEW nullView = {};
			m_indicesView = nullView;
		}
	}

	int m_numParticles;

	D3D12_VERTEX_BUFFER_VIEW m_positionsView;
	D3D12_VERTEX_BUFFER_VIEW m_velocitiesView;
	D3D12_INDEX_BUFFER_VIEW m_indicesView;

	FlexBuffer m_indicesBuf;
	FlexBuffer m_positionsBuf;
	FlexBuffer m_velocitiesBuf;
};

struct DemoContextD3D12: public DemoContext
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS_BASE(DemoContextD3D12)
public:
	enum
	{
		NUM_COMPOSITE_SRVS = 3,				
		MAX_DEBUG_LINE_SIZE = 1024 * 4,		// Will flush if exceeds this			
	};

	// DemoContext Impl
	virtual bool initialize(const RenderInitOptions& options) override;
	virtual void startFrame(Vec4 colorIn) override;
	virtual void endFrame() override;
	virtual void presentFrame(bool fullsync) override;

	virtual void readFrame(int* backbuffer, int width, int height) override;

	virtual void getViewRay(int x, int y, Vec3& origin, Vec3& dir) override;
	virtual void setView(Matrix44 view, Matrix44 projection) override;
	virtual void renderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug) override;

	virtual void drawMesh(const Mesh* m, Vec3 color) override;
	virtual void drawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth) override;
	virtual void drawRope(Vec4* positions, int* indices, int numIndices, float radius, int color) override;
	virtual void drawPlane(const Vec4& p, bool color) override;
	virtual void drawPlanes(Vec4* planes, int n, float bias) override;
	virtual void drawPoints(FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowTex, bool showDensity) override;

	virtual void graphicsTimerBegin() override;
	virtual void graphicsTimerEnd() override;

	virtual float rendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq) override;

	virtual void bindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float bias, Vec4 fogColor) override;
	virtual void unbindSolidShader() override;

	virtual ShadowMap* shadowCreate() override;
	virtual void shadowDestroy(ShadowMap* map) override;
	virtual void shadowBegin(ShadowMap* map) override;
	virtual void shadowEnd() override;

	virtual FluidRenderer* createFluidRenderer(uint32_t width, uint32_t height) override;
	virtual void destroyFluidRenderer(FluidRenderer* renderer) override;

	virtual FluidRenderBuffers* createFluidRenderBuffers(int numParticles, bool enableInterop) override;
	virtual void updateFluidRenderBuffers(FluidRenderBuffers* buffers, NvFlexSolver* flex, bool anisotropy, bool density) override;
	virtual void updateFluidRenderBuffers(FluidRenderBuffers* buffers, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices) override;
	virtual void destroyFluidRenderBuffers(FluidRenderBuffers* buffers) override;

	virtual GpuMesh* createGpuMesh(const Mesh* m) override;
	virtual void destroyGpuMesh(GpuMesh* mesh) override;
	virtual void drawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color) override;
	virtual void drawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color) override;

	virtual DiffuseRenderBuffers* createDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop) override;
	virtual void drawDiffuse(FluidRenderer* render, const DiffuseRenderBuffers* buffers, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front) override;
	virtual void destroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers) override;
	virtual void updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles) override;
	virtual void updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, NvFlexSolver* solver) override;
	virtual int getNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers) override;

	virtual void beginLines() override;
	virtual void drawLine(const Vec3& p, const Vec3& q, const Vec4& color) override;
	virtual void endLines() override;

	virtual void onSizeChanged(int width, int height, bool minimized) override;

	virtual void startGpuWork() override;
	virtual void endGpuWork() override;
	virtual void flushGraphicsAndWait() override;

	virtual void setFillMode(bool wire) override;
	virtual void setCullMode(bool enabled) override;

	virtual void getRenderDevice(void** device, void** context) override;
	virtual void drawImguiGraph() override;
	virtual void* getGraphicsCommandQueue() override;

		/// Get the render context
	inline AppGraphCtxD3D12* getRenderContext() const { return m_renderContext; }
	
	DemoContextD3D12();
	~DemoContextD3D12();

	int _initRenderResources(const RenderInitOptions& options);
	int _initFluidRenderTargets();
	
	void _flushDebugLines();

	MeshDrawParamsD3D m_meshDrawParams;

	Matrix44 m_view;
	Matrix44 m_proj;

	// NOTE! These are allocated such that they are in order. This is required because on compositePS.hlsl, I need s0, and s1 to be linear, and then shadowMap samplers.
	int m_linearSamplerIndex;				///< Index to a linear sample on the m_samplerDescriptorHeap
	int m_shadowMapLinearSamplerIndex;		///< Index to shadow map depth comparator sampler descriptor in m_samplerDescriptorHeap
	
	int m_fluidPointDepthSrvIndex;			///< Index into srv heap that holds srv for the m_flexMeshPipeline
	int m_fluidCompositeSrvBaseIndex;		///< We have a set of NUM_COMPOSITE_SRVS for every back buffer there is								

	AppGraphCtx* m_appGraphCtx;
	AppGraphCtxD3D12* m_renderContext;

	std::unique_ptr<MeshRenderer> m_meshRenderer;
	
	// Render pipelines
	std::unique_ptr<RenderPipeline> m_meshPipeline;
	std::unique_ptr<RenderPipeline> m_pointPipeline;
	std::unique_ptr<RenderPipeline> m_fluidThicknessPipeline;
	std::unique_ptr<RenderPipeline> m_fluidPointPipeline;
	std::unique_ptr<RenderPipeline> m_fluidSmoothPipeline;
	std::unique_ptr<RenderPipeline> m_fluidCompositePipeline;
	std::unique_ptr<RenderPipeline> m_diffusePointPipeline;
	std::unique_ptr<RenderPipeline> m_linePipeline;

	std::unique_ptr<NvCo::Dx12RenderTarget> m_fluidThicknessRenderTarget;
	std::unique_ptr<NvCo::Dx12RenderTarget> m_fluidPointRenderTarget;
	std::unique_ptr<NvCo::Dx12RenderTarget> m_fluidSmoothRenderTarget;
	std::unique_ptr<NvCo::Dx12RenderTarget> m_fluidResolvedTarget;					///< The main render target resolved, such that it can be sampled from

	std::unique_ptr<RenderMesh> m_screenQuadMesh;

	std::unique_ptr<NvCo::Dx12RenderTarget> m_shadowMap;

	NvCo::Dx12RenderTarget* m_currentShadowMap;							///< The current read from shadow buffer
	NvCo::Dx12RenderTarget* m_targetShadowMap;							///< The shadow map bound to render to (ie for write with Begin/EndShadow

	std::wstring m_executablePath;
	std::wstring m_shadersPath;

	RenderStateManagerD3D12* m_renderStateManager;
	RenderStateD3D12 m_renderState;

	SDL_Window* m_window;
	HWND m_hwnd;

	int m_msaaSamples;

	// Synchronization and timing for async compute benchmarking
	ID3D12Fence*     m_graphicsCompleteFence;
	HANDLE			 m_graphicsCompleteEvent;
	UINT64			 m_graphicsCompleteFenceValue;
	ID3D12QueryHeap* m_queryHeap;
	ID3D12Resource*  m_queryResults;

	// Staging buffer
	ID3D12Resource* m_bufferStage;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_footprint;

	bool m_inLineDraw;
	std::vector<LineData::Vertex> m_debugLineVertices;
};

} // namespace FlexSample

#endif // DEMO_CONTEXT_D3D12_H
