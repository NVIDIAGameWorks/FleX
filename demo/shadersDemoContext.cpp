// to fix min max windows macros
#define NOMINMAX

// SDL
//#include "../external/SDL2-2.0.4/include/SDL_syswm.h"
#include "../external/SDL2-2.0.4/include/SDL.h"

// This
#include "shadersDemoContext.h"

// Stubs for unsupported renderers
//#if PLATFORM_LINUX
#ifdef __linux__
DemoContext* CreateDemoContextD3D11() { assert(0); return 0; };
DemoContext* CreateDemoContextD3D12() { assert(0); return 0; };
#else
#if FLEX_DX
DemoContext* CreateDemoContextOGL() { assert(0); return 0; };
#endif
#endif

extern DemoContext* CreateDemoContextOGL();
extern DemoContext* CreateDemoContextD3D11();
extern DemoContext* CreateDemoContextD3D12();

DemoContext* s_context = NULL;

void CreateDemoContext(int type)
{
	DemoContext* context = 0;
#ifndef ANDROID
	switch (type)
	{
	case 0: context = CreateDemoContextOGL(); break;
	case 1: context = CreateDemoContextD3D11(); break;
	case 2: context = CreateDemoContextD3D12(); break;
	default: assert(0);
	}
#endif
	s_context = context;
}

DemoContext* GetDemoContext()
{
	return s_context;
}

void ReshapeRender(SDL_Window* window)
{
	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	s_context->onSizeChanged(width, height, false);
}

void DestroyRender()
{
	if (s_context)
	{
		delete s_context;
		s_context = nullptr;
	}
}

void StartFrame(Vec4 colorIn) { s_context->startFrame(colorIn); }
void EndFrame() { s_context->endFrame(); }
void PresentFrame(bool fullSync) { s_context->presentFrame(fullSync); }

void FlushGraphicsAndWait() { s_context->flushGraphicsAndWait(); }

void ReadFrame(int* backbuffer, int width, int height) { s_context->readFrame(backbuffer, width, height); }

void GetViewRay(int x, int y, Vec3& origin, Vec3& dir) { return s_context->getViewRay(x, y, origin, dir); }

void SetFillMode(bool wire) { s_context->setFillMode(wire); }

void SetCullMode(bool enabled) { s_context->setCullMode(enabled); }

void SetView(Matrix44 view, Matrix44 projection) { return s_context->setView(view, projection); }

FluidRenderer* CreateFluidRenderer(uint32_t width, uint32_t height) { return s_context->createFluidRenderer(width, height); }

void DestroyFluidRenderer(FluidRenderer* renderer) { return s_context->destroyFluidRenderer(renderer); }

FluidRenderBuffers* CreateFluidRenderBuffers(int numParticles, bool enableInterop) { return s_context->createFluidRenderBuffers(numParticles, enableInterop); }

void UpdateFluidRenderBuffers(FluidRenderBuffers* buffers, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices)
{
	s_context->updateFluidRenderBuffers(buffers, particles, densities, anisotropy1, anisotropy2, anisotropy3, numParticles, indices, numIndices);
}

void UpdateFluidRenderBuffers(FluidRenderBuffers* buffers, NvFlexSolver* flex, bool anisotropy, bool density) { return s_context->updateFluidRenderBuffers(buffers, flex, anisotropy, density); }

void DestroyFluidRenderBuffers(FluidRenderBuffers* buffers) { return s_context->destroyFluidRenderBuffers(buffers); }

ShadowMap* ShadowCreate() { return s_context->shadowCreate(); }

void ShadowDestroy(ShadowMap* map) { s_context->shadowDestroy(map); }

void ShadowBegin(ShadowMap* map) { s_context->shadowBegin(map); }

void ShadowEnd() { s_context->shadowEnd(); }


void BindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float bias, Vec4 fogColor)
{
	s_context->bindSolidShader(lightPos, lightTarget, lightTransform, shadowMap, bias, fogColor);
}

void UnbindSolidShader() { s_context->unbindSolidShader(); }

void DrawMesh(const Mesh* m, Vec3 color) { s_context->drawMesh(m, color); }

void DrawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth)
{
	s_context->drawCloth(positions, normals, uvs, indices, numTris, numPositions, colorIndex, expand, twosided, smooth);
}

void DrawRope(Vec4* positions, int* indices, int numIndices, float radius, int color)
{
	s_context->drawRope(positions, indices, numIndices, radius, color);
}

void DrawPlane(const Vec4& p, bool color) { s_context->drawPlane(p, color); }

void DrawPlanes(Vec4* planes, int n, float bias) { s_context->drawPlanes(planes, n, bias); }

GpuMesh* CreateGpuMesh(const Mesh* m)
{
	return m ? s_context->createGpuMesh(m) : nullptr;
}

void DestroyGpuMesh(GpuMesh* m) { s_context->destroyGpuMesh(m); }

void DrawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color) { s_context->drawGpuMesh(m, xform, color); }

void DrawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color) { s_context->drawGpuMeshInstances(m, xforms, n, color); }

void DrawPoints(FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, bool showDensity)
{
	s_context->drawPoints(buffers, n, offset, radius, screenWidth, screenAspect, fov, lightPos, lightTarget, lightTransform, shadowTex, showDensity);
}

void RenderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers* buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug)
{
	s_context->renderEllipsoids(renderer, buffers, n, offset, radius, screenWidth, screenAspect, fov, lightPos, lightTarget, lightTransform, shadowMap, color, blur, ior, debug);
}

DiffuseRenderBuffers* CreateDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop) { return s_context->createDiffuseRenderBuffers(numDiffuseParticles, enableInterop); }

void DestroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers) { return s_context->destroyDiffuseRenderBuffers(buffers); }

void UpdateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles)
{ 
	s_context->updateDiffuseRenderBuffers(buffers, diffusePositions, diffuseVelocities, numDiffuseParticles); 
}

void UpdateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, NvFlexSolver* solver) { return s_context->updateDiffuseRenderBuffers(buffers, solver); }

int GetNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers) { return s_context->getNumDiffuseRenderParticles(buffers); }

void RenderDiffuse(FluidRenderer* render, DiffuseRenderBuffers* buffers, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front)
{
	s_context->drawDiffuse(render, buffers, n, radius, screenWidth, screenAspect, fov, color, lightPos, lightTarget, lightTransform, shadowMap, motionBlur, inscatter, outscatter, shadowEnabled, front);
}

void BeginLines() {	s_context->beginLines(); }

void DrawLine(const Vec3& p, const Vec3& q, const Vec4& color) { s_context->drawLine(p, q, color); }

void EndLines() { s_context->endLines(); }

void BeginPoints(float size) {}
void DrawPoint(const Vec3& p, const Vec4& color) {}
void EndPoints() {}

float RendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq)
{
	return s_context->rendererGetDeviceTimestamps(begin, end, freq);
}

void* GetGraphicsCommandQueue() { return s_context->getGraphicsCommandQueue(); }
void GraphicsTimerBegin() { s_context->graphicsTimerBegin(); }
void GraphicsTimerEnd() { s_context->graphicsTimerEnd(); }

void StartGpuWork() { s_context->startGpuWork(); }
void EndGpuWork() { s_context->endGpuWork(); }

void DrawImguiGraph() { return s_context->drawImguiGraph(); }

void GetRenderDevice(void** deviceOut, void** contextOut) { return s_context->getRenderDevice(deviceOut, contextOut); }

void InitRender(const RenderInitOptions& options)
{
	if (!s_context)
	{
		assert(false && "A context has not been set with SetDemoContext!");
		return;
	}
	if (!s_context->initialize(options))
	{
		assert(!"Unable to initialize context!");
		delete s_context;
		s_context = nullptr;
		return;
	}
}

