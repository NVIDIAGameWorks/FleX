#include "core/maths.h"
#include "core/extrude.h"

#include "shaders.h"

#include "meshRenderD3D11.h"
#include "pointRenderD3D11.h"
#include "fluidRenderD3D11.h"
#include "diffuseRenderD3D11.h"
#include "debugLineRenderD3D11.h"

#include "shadowMapD3D11.h"
#include "renderTargetD3D11.h"

#include "imguiGraphD3D11.h"

#include "appD3D11Ctx.h"

#include "demoContext.h"
#include "../d3d/loader.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <cstdlib>

// this
#include "demoContextD3D11.h"

namespace
{
// COM object release utilities
template <class T>
void inline COMRelease(T& t)
{
	if (t) t->Release();
	t = nullptr;
}

template <class T>
void inline COMRelease(T& t, UINT arraySize)
{
	for (UINT i = 0; i < arraySize; i++)
	{
		if (t[i]) t[i]->Release();
		t[i] = nullptr;
	}
}
}

extern Colour g_colors[];
static const int kShadowResolution = 2048;

DemoContext* CreateDemoContextD3D11()
{
	return new DemoContextD3D11;
}

DemoContextD3D11::DemoContextD3D11()
{
	m_appGraphCtx = nullptr;
	m_appGraphCtxD3D11 = nullptr;
	memset(&m_meshDrawParams, 0, sizeof(m_meshDrawParams));

	m_spotMin = 0.5f;
	m_spotMax = 1.0f;
	m_shadowBias = 0.075f;

	m_shadowMap = nullptr;
	m_immediateMesh = nullptr;

	m_window = nullptr;

	m_msaaSamples = 1;

	m_renderTimerDisjoint = nullptr;
	m_renderTimerBegin = nullptr;
	m_renderTimerEnd = nullptr;
	m_renderCompletionFence = nullptr;

	m_compositeBlendState = nullptr;

	m_fluidResolvedTarget = nullptr;
	m_fluidResolvedTargetSRV = nullptr;
	m_fluidResolvedStage = nullptr;

	m_debugLineRender = new DebugLineRenderD3D11;
	m_meshRenderer = new MeshRendererD3D11;
	m_pointRenderer = new PointRendererD3D11;
	m_diffuseRenderer = new DiffuseRendererD3D11;
}

// convert an OpenGL style projection matrix to D3D (clip z range [0, 1])
Matrix44 ConvertToD3DProjection(const Matrix44& proj)
{
	Matrix44 scale = Matrix44::kIdentity;
	scale.columns[2][2] = 0.5f;

	Matrix44 bias = Matrix44::kIdentity;
	bias.columns[3][2] = 1.0f;	

	return scale*bias*proj;
}

#define checkDxErrors(err)  __checkDxErrors (err, __FILE__, __LINE__)

inline void __checkDxErrors(HRESULT err, const char *file, const int line)
{
	if (FAILED(err))
	{
		char* lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		fprintf(stdout, "DX Error = %04d \"%s\" from file <%s>, line %i.\n",
			err, lpMsgBuf, file, line);

		exit(EXIT_FAILURE);
	}
}

DemoContextD3D11::~DemoContextD3D11()
{
	imguiGraphDestroy();
	
	COMRelease(m_renderTimerBegin);
	COMRelease(m_renderTimerEnd);
	COMRelease(m_renderTimerDisjoint);
	COMRelease(m_renderCompletionFence);

	COMRelease(m_compositeBlendState);

	COMRelease(m_fluidResolvedTarget);
	COMRelease(m_fluidResolvedTargetSRV);
	COMRelease(m_fluidResolvedStage);

	delete m_immediateMesh;
	delete m_debugLineRender;
	delete m_meshRenderer;
	delete m_pointRenderer;
	delete m_diffuseRenderer;

	// do this first, since it flushes all GPU work
	AppGraphCtxReleaseRenderTarget(m_appGraphCtx);
	AppGraphCtxRelease(m_appGraphCtx);
}

bool DemoContextD3D11::initialize(const RenderInitOptions& options)
{
	{
		// Load external modules
		loadModules(APP_CONTEXT_D3D11);
	}

	// must always have at least one sample
	m_msaaSamples = Max(1, options.numMsaaSamples);
	// create app graph context
	m_appGraphCtx = AppGraphCtxCreate(0);
	m_appGraphCtxD3D11 = cast_to_AppGraphCtxD3D11(m_appGraphCtx);

	AppGraphCtxUpdateSize(m_appGraphCtx, options.window, options.fullscreen, m_msaaSamples);
	_onWindowSizeChanged(m_appGraphCtxD3D11->m_winW, m_appGraphCtxD3D11->m_winH, false);

	//AppGraphCtxInitRenderTarget(m_appGraphCtx, options.window, options.fullscreen, m_msaaSamples);
	//gScene = getScene(0);

	AppGraphColor clearVal = { 0.0f, 0.0f, 0.0f, 1.0f };
	AppGraphCtxFrameStart(m_appGraphCtx, clearVal);

	ID3D11Device* device = m_appGraphCtxD3D11->m_device;
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;

	{
		// create imgui, connect to app graph context
		ImguiGraphDescD3D11 desc;
		desc.device = device;
		desc.deviceContext = deviceContext;
		desc.winW = m_appGraphCtxD3D11->m_winW;
		desc.winH = m_appGraphCtxD3D11->m_winH;

		// Use Dx11 context
		const int defaultFontHeight = (options.defaultFontHeight <= 0) ? 15 : options.defaultFontHeight;

		if (!imguiGraphInit("../../data/DroidSans.ttf", float(defaultFontHeight), (ImguiGraphDesc*)&desc))
		{
			return false;
		}
	}

	AppGraphCtxFramePresent(m_appGraphCtx, true);
	
	m_pointRenderer->init(device, deviceContext);
	m_meshRenderer->init(device, deviceContext, options.asyncComputeBenchmark);
	m_debugLineRender->init(device, deviceContext);
	m_diffuseRenderer->init(device, deviceContext);

	{
		// create blend state - used for composite phase of water rendering
		D3D11_BLEND_DESC blendStateDesc = {};
		blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
		blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

		if (FAILED(device->CreateBlendState(&blendStateDesc, &m_compositeBlendState)))
		{
			return false;
		}
	}

	// create a mesh for immediate mode rendering
	m_immediateMesh = new GpuMeshD3D11(device, deviceContext);

	m_window = options.window;
	
	D3D11_QUERY_DESC tdesc;
	ZeroMemory(&tdesc, sizeof(tdesc));
	tdesc.Query = D3D11_QUERY_TIMESTAMP;
	device->CreateQuery(&tdesc, &m_renderTimerBegin);
	device->CreateQuery(&tdesc, &m_renderTimerEnd);

	tdesc.Query = D3D11_QUERY_EVENT;
	device->CreateQuery(&tdesc, &m_renderCompletionFence);

	ZeroMemory(&tdesc, sizeof(tdesc));
	tdesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	device->CreateQuery(&tdesc, &m_renderTimerDisjoint);

	return true;
}

void DemoContextD3D11::getRenderDevice(void** deviceOut, void** contextOut)
{
	*deviceOut = m_appGraphCtxD3D11->m_device;
	*contextOut = m_appGraphCtxD3D11->m_deviceContext;
}

void DemoContextD3D11::onSizeChanged(int width, int height, bool minimized)
{
	AppGraphCtxUpdateSize(m_appGraphCtx, m_window, false, m_msaaSamples);
	_onWindowSizeChanged(m_appGraphCtxD3D11->m_winW, m_appGraphCtxD3D11->m_winH, false);
}

void DemoContextD3D11::_onWindowSizeChanged(int width, int height, bool minimized)
{
	if (m_fluidResolvedTarget)
	{
		D3D11_TEXTURE2D_DESC desc;
		m_fluidResolvedTarget->GetDesc(&desc);
		if (desc.Width == width && desc.Height == height)
		{
			return;
		}

		COMRelease(m_fluidResolvedTarget);
		COMRelease(m_fluidResolvedTargetSRV);
		COMRelease(m_fluidResolvedStage);
	}

	// Recreate...
	ID3D11Device* device = m_appGraphCtxD3D11->m_device;

	// resolved texture target (for refraction / scene sampling)
	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (FAILED(device->CreateTexture2D(&texDesc, nullptr, &m_fluidResolvedTarget)))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		if (FAILED(device->CreateShaderResourceView(m_fluidResolvedTarget, &srvDesc, &m_fluidResolvedTargetSRV)))
		{
			return;
		}

		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.BindFlags = 0u;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		if (FAILED(device->CreateTexture2D(&texDesc, nullptr, &m_fluidResolvedStage)))
		{
			return;
		}
	}
}

void DemoContextD3D11::startFrame(Vec4 clear)
{
	AppGraphColor clearColor = { clear.x, clear.y, clear.z, clear.w };
	AppGraphCtxFrameStart(m_appGraphCtx, clearColor);

	MeshDrawParamsD3D& meshDrawParams = m_meshDrawParams;

	memset(&meshDrawParams, 0, sizeof(MeshDrawParamsD3D));
	meshDrawParams.renderStage = MESH_DRAW_LIGHT;
	meshDrawParams.renderMode = MESH_RENDER_SOLID;
	meshDrawParams.cullMode = MESH_CULL_BACK;
	meshDrawParams.projection = (XMMATRIX)Matrix44::kIdentity;
	meshDrawParams.view = (XMMATRIX)Matrix44::kIdentity;
	meshDrawParams.model = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f),
		DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)
	);
}

void DemoContextD3D11::_flushFrame()
{
	m_appGraphCtxD3D11->m_deviceContext->Flush();
}

void DemoContextD3D11::endFrame()
{
	_flushFrame();

	ImguiGraphDescD3D11 desc;

	desc.device = m_appGraphCtxD3D11->m_device;
	desc.deviceContext = m_appGraphCtxD3D11->m_deviceContext;
	desc.winW = m_appGraphCtxD3D11->m_winW;
	desc.winH = m_appGraphCtxD3D11->m_winH;
	
	imguiGraphUpdate((ImguiGraphDesc*)&desc);
	//m_imguiGraphContext->update(&desc);
}

void DemoContextD3D11::presentFrame(bool fullsync)
{
	AppGraphCtxFramePresent(m_appGraphCtx, fullsync);
}

void DemoContextD3D11::readFrame(int* buffer, int width, int height)
{
	auto deviceContext = m_appGraphCtxD3D11->m_deviceContext;
	if (m_msaaSamples > 1)
	{
		deviceContext->ResolveSubresource(m_fluidResolvedTarget, 0, m_appGraphCtxD3D11->m_backBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		deviceContext->CopyResource(m_fluidResolvedStage, m_fluidResolvedTarget);
	}
	else
	{
		deviceContext->CopyResource(m_fluidResolvedStage, m_appGraphCtxD3D11->m_backBuffer);
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	deviceContext->Map(m_fluidResolvedStage, 0u, D3D11_MAP_READ, 0, &mapped);
	// y-coordinate is flipped for DirectX
	for (int i = 0; i < height; i++)
	{
		memcpy(buffer + (width * i), ((int *)mapped.pData) + (width * (height - i)), width * sizeof(int));
	}
	deviceContext->Unmap(m_fluidResolvedStage, 0u);
}

void DemoContextD3D11::getViewRay(int x, int y, Vec3& origin, Vec3& dir)
{
	using namespace DirectX;

	XMVECTOR nearVector = XMVector3Unproject(XMVectorSet(float(x), float(m_appGraphCtxD3D11->m_winH-y), 0.0f, 0.0f), 0.0f, 0.0f, (float)m_appGraphCtxD3D11->m_winW, (float)m_appGraphCtxD3D11->m_winH, 0.0f, 1.0f, (XMMATRIX)m_proj, XMMatrixIdentity(), (XMMATRIX)m_view);
	XMVECTOR farVector = XMVector3Unproject(XMVectorSet(float(x), float(m_appGraphCtxD3D11->m_winH-y), 1.0f, 0.0f), 0.0f, 0.0f, (float)m_appGraphCtxD3D11->m_winW, (float)m_appGraphCtxD3D11->m_winH, 0.0f, 1.0f, (XMMATRIX)m_proj, XMMatrixIdentity(), (XMMATRIX)m_view);

	origin = Vec3(XMVectorGetX(nearVector), XMVectorGetY(nearVector), XMVectorGetZ(nearVector));
	XMVECTOR tmp = farVector - nearVector;
	dir = Normalize(Vec3(XMVectorGetX(tmp), XMVectorGetY(tmp), XMVectorGetZ(tmp)));
}

void DemoContextD3D11::setFillMode(bool wire)
{
	m_meshDrawParams.renderMode = wire ? MESH_RENDER_WIREFRAME : MESH_RENDER_SOLID;
}

void DemoContextD3D11::setCullMode(bool enabled)
{
	m_meshDrawParams.cullMode = enabled ? MESH_CULL_BACK : MESH_CULL_NONE;
}

void DemoContextD3D11::setView(Matrix44 view, Matrix44 projection)
{
	Matrix44 vp = projection*view;

	m_meshDrawParams.model = (XMMATRIX)Matrix44::kIdentity;
	m_meshDrawParams.view = (XMMATRIX)view;
	m_meshDrawParams.projection = (XMMATRIX)(ConvertToD3DProjection(projection));

	m_view = view;
	m_proj = ConvertToD3DProjection(projection);
}

FluidRenderer* DemoContextD3D11::createFluidRenderer(uint32_t width, uint32_t height)
{
	FluidRendererD3D11* renderer = new(_aligned_malloc(sizeof(FluidRendererD3D11), 16)) FluidRendererD3D11;
	renderer->init(m_appGraphCtxD3D11->m_device, m_appGraphCtxD3D11->m_deviceContext, width, height);
	return (FluidRenderer*)renderer;
}

void DemoContextD3D11::destroyFluidRenderer(FluidRenderer* rendererIn)
{
	FluidRendererD3D11* renderer = (FluidRendererD3D11*)rendererIn;
	renderer->~FluidRendererD3D11();
	_aligned_free(renderer);
}

FluidRenderBuffers* DemoContextD3D11::createFluidRenderBuffers(int numParticles, bool enableInterop)
{
	FluidRenderBuffersD3D11* buffers = new FluidRenderBuffersD3D11;
	buffers->m_numParticles = numParticles;

	ID3D11Device* device = m_appGraphCtxD3D11->m_device;

	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = numParticles*sizeof(Vec4);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.StructureByteStride = 0;

		device->CreateBuffer(&bufDesc, NULL, &buffers->m_positions);
	
		device->CreateBuffer(&bufDesc, NULL, &buffers->m_anisotropiesArr[0]);
		device->CreateBuffer(&bufDesc, NULL, &buffers->m_anisotropiesArr[1]);
		device->CreateBuffer(&bufDesc, NULL, &buffers->m_anisotropiesArr[2]);

		bufDesc.ByteWidth = numParticles*sizeof(float);
		device->CreateBuffer(&bufDesc, NULL, &buffers->m_densities);
	}

	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = numParticles * sizeof(int);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.StructureByteStride = 0;

		device->CreateBuffer(&bufDesc, NULL, &buffers->m_indices);
	}

	if (enableInterop)
	{
		extern NvFlexLibrary* g_flexLib;

		buffers->m_positionsBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_positions.Get(), numParticles, sizeof(Vec4));
		buffers->m_densitiesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_densities.Get(), numParticles, sizeof(float));
		buffers->m_indicesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_indices.Get(), numParticles, sizeof(int));

		buffers->m_anisotropiesBufArr[0] = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_anisotropiesArr[0].Get(), numParticles, sizeof(Vec4));
		buffers->m_anisotropiesBufArr[1] = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_anisotropiesArr[1].Get(), numParticles, sizeof(Vec4));
		buffers->m_anisotropiesBufArr[2] = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_anisotropiesArr[2].Get(), numParticles, sizeof(Vec4));
	}
	
	return reinterpret_cast<FluidRenderBuffers*>(buffers);
}

void DemoContextD3D11::updateFluidRenderBuffers(FluidRenderBuffers* buffersIn, NvFlexSolver* solver, bool anisotropy, bool density)
{
	FluidRenderBuffersD3D11& buffers = *reinterpret_cast<FluidRenderBuffersD3D11*>(buffersIn);
	if (!anisotropy)
	{
		// regular particles
		NvFlexGetParticles(solver, buffers.m_positionsBuf, NULL);
	}
	else
	{
		// fluid buffers
		NvFlexGetSmoothParticles(solver, buffers.m_positionsBuf, NULL);
		NvFlexGetAnisotropy(solver, buffers.m_anisotropiesBufArr[0], buffers.m_anisotropiesBufArr[1], buffers.m_anisotropiesBufArr[2], NULL);
	}

	if (density)
	{
		NvFlexGetDensities(solver, buffers.m_densitiesBuf, NULL);
	}
	else
	{
		NvFlexGetPhases(solver, buffers.m_densitiesBuf, NULL);
	}

	NvFlexGetActive(solver, buffers.m_indicesBuf, NULL);
}

void DemoContextD3D11::updateFluidRenderBuffers(FluidRenderBuffers* buffersIn, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices)
{
	FluidRenderBuffersD3D11& buffers = *reinterpret_cast<FluidRenderBuffersD3D11*>(buffersIn);
	D3D11_MAPPED_SUBRESOURCE res;
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;

	// vertices
	if (particles)
	{
		deviceContext->Map(buffers.m_positions.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, particles, sizeof(Vec4)*numParticles);
		deviceContext->Unmap(buffers.m_positions.Get(), 0);
	}

	Vec4*const anisotropies[3] = 
	{
		anisotropy1,
		anisotropy2,
		anisotropy3,
	};

	for (int i = 0; i < 3; i++)
	{
		const Vec4* anisotropy = anisotropies[i];
		if (anisotropy)
		{
			deviceContext->Map(buffers.m_anisotropiesArr[i].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			memcpy(res.pData, anisotropy, sizeof(Vec4) * numParticles);
			deviceContext->Unmap(buffers.m_anisotropiesArr[i].Get(), 0);
		}
	}

	if (densities)
	{
		deviceContext->Map(buffers.m_densities.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, densities, sizeof(float)*numParticles);
		deviceContext->Unmap(buffers.m_densities.Get(), 0);
	}

	// indices
	if (indices)
	{
		deviceContext->Map(buffers.m_indices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, indices, sizeof(int)*numIndices);
		deviceContext->Unmap(buffers.m_indices.Get(), 0);
	}
}

void DemoContextD3D11::destroyFluidRenderBuffers(FluidRenderBuffers* buffers)
{
	delete reinterpret_cast<FluidRenderBuffersD3D11*>(buffers);
}

ShadowMap* DemoContextD3D11::shadowCreate()
{
	ShadowMapD3D11* shadowMap = new(_aligned_malloc(sizeof(ShadowMapD3D11), 16)) ShadowMapD3D11;
	shadowMap->init(m_appGraphCtxD3D11->m_device, kShadowResolution);
	return (ShadowMap*)shadowMap;
}

void DemoContextD3D11::shadowDestroy(ShadowMap* map)
{
	ShadowMapD3D11* shadowMap = (ShadowMapD3D11*)map;
	shadowMap->~ShadowMapD3D11();
	_aligned_free(shadowMap);
}

void DemoContextD3D11::shadowBegin(ShadowMap* map)
{
	ShadowMapD3D11* shadowMap = (ShadowMapD3D11*)map;
	shadowMap->bindAndClear(m_appGraphCtxD3D11->m_deviceContext);

	m_meshDrawParams.renderStage = MESH_DRAW_SHADOW;
	m_shadowMap = shadowMap;
}

void DemoContextD3D11::shadowEnd()
{
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;
	
	// reset to main frame buffer
	deviceContext->RSSetViewports(1, &m_appGraphCtxD3D11->m_viewport);
	deviceContext->OMSetRenderTargets(1, &m_appGraphCtxD3D11->m_rtv, m_appGraphCtxD3D11->m_dsv);
	deviceContext->OMSetDepthStencilState(m_appGraphCtxD3D11->m_depthState, 0u);
	deviceContext->ClearDepthStencilView(m_appGraphCtxD3D11->m_dsv, D3D11_CLEAR_DEPTH, 1.0, 0);

	m_meshDrawParams.renderStage = MESH_DRAW_NULL;
}

void DemoContextD3D11::bindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float bias, Vec4 fogColor)
{
	m_meshDrawParams.renderStage = MESH_DRAW_LIGHT;

	m_meshDrawParams.grid = 0;
	m_meshDrawParams.spotMin = m_spotMin;
	m_meshDrawParams.spotMax = m_spotMax;
	m_meshDrawParams.fogColor = (float4&)fogColor;

	m_meshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;

	ShadowParamsD3D shadow;
	RenderParamsUtilD3D::calcShadowParams(lightPos, lightTarget, lightTransform, m_shadowBias, &shadow);

	m_meshDrawParams.lightTransform = shadow.lightTransform;
	m_meshDrawParams.lightDir = shadow.lightDir;
	m_meshDrawParams.lightPos = shadow.lightPos;
	m_meshDrawParams.bias = shadow.bias;
	memcpy(m_meshDrawParams.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));

	m_shadowMap = (ShadowMapD3D11*)shadowMap;
}

void DemoContextD3D11::unbindSolidShader()
{
	m_meshDrawParams.renderStage = MESH_DRAW_NULL;	
}

void DemoContextD3D11::graphicsTimerBegin()
{
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;
	deviceContext->Begin(m_renderTimerDisjoint);
	deviceContext->End(m_renderTimerBegin); // yes End.
}

void DemoContextD3D11::graphicsTimerEnd()
{
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;
	deviceContext->End(m_renderTimerEnd);
	deviceContext->End(m_renderTimerDisjoint);
	m_timersSet = true;
}

float DemoContextD3D11::rendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq)
{
	float renderTime = 0.0f;

	if (m_timersSet)
	{
		ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
		uint64_t renderBegin = 0;
		uint64_t renderEnd = 0;

		while (S_OK != deviceContext->GetData(m_renderTimerDisjoint, &tsDisjoint, sizeof(tsDisjoint), 0));
		while (S_OK != deviceContext->GetData(m_renderTimerBegin, &renderBegin, sizeof(UINT64), 0));
		while (S_OK != deviceContext->GetData(m_renderTimerEnd, &renderEnd, sizeof(UINT64), 0));

		float renderTime = float(renderEnd - renderBegin) / float(tsDisjoint.Frequency);

		if (begin) *begin = renderBegin;
		if (end) *end = renderEnd;
		if (freq) *freq = tsDisjoint.Frequency;
	}

	return renderTime;
}

void DemoContextD3D11::drawMesh(const Mesh* m, Vec3 color)
{
	if (m)
	{
		if (m->m_colours.size())
		{
			m_meshDrawParams.colorArray = 1;
			m_immediateMesh->updateData((Vec3*)&m->m_positions[0], &m->m_normals[0], NULL, (Vec4*)&m->m_colours[0], (int*)&m->m_indices[0], m->GetNumVertices(), int(m->GetNumFaces()));
		}
		else
		{
			m_meshDrawParams.colorArray = 0;
			m_immediateMesh->updateData((Vec3*)&m->m_positions[0], &m->m_normals[0], NULL, NULL, (int*)&m->m_indices[0], m->GetNumVertices(), int(m->GetNumFaces()));
		}

		m_meshDrawParams.color = (float4&)color;
		m_meshDrawParams.secondaryColor = (float4&)color;
		m_meshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;
		m_meshDrawParams.shadowMap = (ShadowMapD3D*)m_shadowMap;

		m_meshRenderer->draw(m_immediateMesh, &m_meshDrawParams);

		if (m->m_colours.size())
			m_meshDrawParams.colorArray = 0;

	}
}

void DemoContextD3D11::drawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth)
{
	if (!numTris)
		return;

	m_immediateMesh->updateData(positions, normals, NULL, NULL, indices, numPositions, numTris);
	
	if (twosided)
		SetCullMode(false);

	m_meshDrawParams.bias = 0.0f;
	m_meshDrawParams.expand = expand;

	m_meshDrawParams.color = (const float4&)(g_colors[colorIndex + 1] * 1.5f);
	m_meshDrawParams.secondaryColor = (const float4&)(g_colors[colorIndex] * 1.5f);
	m_meshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;
	m_meshDrawParams.shadowMap = (ShadowMapD3D*)m_shadowMap;

	m_meshRenderer->draw(m_immediateMesh, &m_meshDrawParams);

	if (twosided)
		setCullMode(true);

	m_meshDrawParams.bias = m_shadowBias;
	m_meshDrawParams.expand = 0.0f;
}

void DemoContextD3D11::drawRope(Vec4* positions, int* indices, int numIndices, float radius, int color)
{
	if (numIndices < 2)
		return;

	std::vector<Vec3> vertices;
	std::vector<Vec3> normals;
	std::vector<int> triangles;

	// flatten curve
	std::vector<Vec3> curve(numIndices);
	for (int i = 0; i < numIndices; ++i)
		curve[i] = Vec3(positions[indices[i]]);

	const int resolution = 8;
	const int smoothing = 3;

	vertices.reserve(resolution*numIndices*smoothing);
	normals.reserve(resolution*numIndices*smoothing);
	triangles.reserve(numIndices*resolution * 6 * smoothing);

	Extrude(&curve[0], int(curve.size()), vertices, normals, triangles, radius, resolution, smoothing);

	m_immediateMesh->updateData(&vertices[0], &normals[0], NULL, NULL, &triangles[0], int(vertices.size()), int(triangles.size())/3);

	setCullMode(false);

	m_meshDrawParams.color = (const float4&)(g_colors[color % 8] * 1.5f);
	m_meshDrawParams.secondaryColor = (const float4&)(g_colors[color % 8] * 1.5f);
	m_meshDrawParams.objectTransform = (const float4x4&)Matrix44::kIdentity;
	m_meshDrawParams.shadowMap = (ShadowMapD3D*)m_shadowMap;

	m_meshRenderer->draw(m_immediateMesh, &m_meshDrawParams);

	setCullMode(true);
}

void DemoContextD3D11::drawPlane(const Vec4& p, bool color)
{
	std::vector<Vec3> vertices;
	std::vector<Vec3> normals;
	std::vector<int> indices;
	
	Vec3 u, v;
	BasisFromVector(Vec3(p.x, p.y, p.z), &u, &v);

	Vec3 c = Vec3(p.x, p.y, p.z)*-p.w;

	m_meshDrawParams.shadowMap = (ShadowMapD3D*)m_shadowMap;

	if (color)
		m_meshDrawParams.color = (const float4&)(p * 0.5f + Vec4(0.5f, 0.5f, 0.5f, 0.5f));

	const float kSize = 200.0f;
	const int kGrid = 3;

	// draw a grid of quads, otherwise z precision suffers
	for (int x = -kGrid; x <= kGrid; ++x)
	{
		for (int y = -kGrid; y <= kGrid; ++y)
		{
			Vec3 coff = c + u*float(x)*kSize*2.0f + v*float(y)*kSize*2.0f;

			int indexStart = int(vertices.size());

			vertices.push_back(Vec3(coff + u*kSize + v*kSize));
			vertices.push_back(Vec3(coff - u*kSize + v*kSize));
			vertices.push_back(Vec3(coff - u*kSize - v*kSize));
			vertices.push_back(Vec3(coff + u*kSize - v*kSize));

			normals.push_back(Vec3(p.x, p.y, p.z));
			normals.push_back(Vec3(p.x, p.y, p.z));
			normals.push_back(Vec3(p.x, p.y, p.z));
			normals.push_back(Vec3(p.x, p.y, p.z));


			indices.push_back(indexStart+0);
			indices.push_back(indexStart+1);
			indices.push_back(indexStart+2);

			indices.push_back(indexStart+2);
			indices.push_back(indexStart+3);
			indices.push_back(indexStart+0);
		}
	}

	m_immediateMesh->updateData(&vertices[0], &normals[0], NULL, NULL, &indices[0], int(vertices.size()), int(indices.size())/3);
	m_meshRenderer->draw(m_immediateMesh, &m_meshDrawParams);
}

void DemoContextD3D11::drawPlanes(Vec4* planes, int n, float bias)
{
	m_meshDrawParams.color = (float4&)Vec4(0.9f, 0.9f, 0.9f, 1.0f);

	m_meshDrawParams.bias = 0.0f;
	m_meshDrawParams.grid = 1;
	m_meshDrawParams.expand = 0;

	for (int i = 0; i < n; ++i)
	{
		Vec4 p = planes[i];
		p.w -= bias;

		drawPlane(p, false);
	}

	m_meshDrawParams.grid = 0;
	m_meshDrawParams.bias = m_shadowBias;
		
}

GpuMesh* DemoContextD3D11::createGpuMesh(const Mesh* m)
{
	GpuMeshD3D11* mesh = new GpuMeshD3D11(m_appGraphCtxD3D11->m_device, m_appGraphCtxD3D11->m_deviceContext);
	mesh->updateData((Vec3*)&m->m_positions[0], &m->m_normals[0], NULL, NULL, (int*)&m->m_indices[0], m->GetNumVertices(), int(m->GetNumFaces()));
	return (GpuMesh*)mesh;
}

void DemoContextD3D11::destroyGpuMesh(GpuMesh* m)
{
	delete reinterpret_cast<GpuMeshD3D11*>(m);
}

void DemoContextD3D11::drawGpuMesh(GpuMesh* meshIn, const Matrix44& xform, const Vec3& color)
{
	if (meshIn)
	{
		GpuMeshD3D11* mesh = (GpuMeshD3D11*)meshIn;

		MeshDrawParamsD3D params = m_meshDrawParams;

		params.color = (float4&)color;
		params.secondaryColor = (float4&)color;
		params.objectTransform = (float4x4&)xform;
		params.shadowMap = (ShadowMapD3D*)m_shadowMap;

		m_meshRenderer->draw(mesh, &params);
	}
}

void DemoContextD3D11::drawGpuMeshInstances(GpuMesh* meshIn, const Matrix44* xforms, int n, const Vec3& color)
{
	if (meshIn)
	{
		GpuMeshD3D11* mesh = (GpuMeshD3D11*)meshIn;

		m_meshDrawParams.color = (float4&)color;
		m_meshDrawParams.secondaryColor = (float4&)color;
		m_meshDrawParams.shadowMap = (ShadowMapD3D*)m_shadowMap;

		// copy params
		MeshDrawParamsD3D params(m_meshDrawParams);

		for (int i = 0; i < n; ++i)
		{
			params.objectTransform = (const float4x4&)xforms[i];
			m_meshRenderer->draw(mesh, &params);
		}
	}
}

void DemoContextD3D11::drawPoints(FluidRenderBuffers* buffersIn, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, bool showDensity)
{
	FluidRenderBuffersD3D11& buffers = *reinterpret_cast<FluidRenderBuffersD3D11*>(buffersIn);
	if (n == 0)
		return;
	
	PointDrawParamsD3D params;

	params.renderMode = POINT_RENDER_SOLID;
	params.cullMode = POINT_CULL_BACK;
	params.model = (const XMMATRIX&)Matrix44::kIdentity;
	params.view = (const XMMATRIX&)m_view;
	params.projection = (const XMMATRIX&)m_proj;

	params.pointRadius = radius;
	params.pointScale = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.spotMin = m_spotMin;
	params.spotMax = m_spotMax;

	int mode = 0;
	if (showDensity)
		mode = 1;
	if (shadowTex == 0)
		mode = 2;
	params.mode = mode;

	for (int i = 0; i < 8; i++)
		params.colors[i] = *((const float4*)&g_colors[i].r);

	// set shadow parameters
	ShadowParamsD3D shadow;
	RenderParamsUtilD3D::calcShadowParams(lightPos, lightTarget, lightTransform, m_shadowBias, &shadow);
	params.lightTransform = shadow.lightTransform;
	params.lightDir = shadow.lightDir;
	params.lightPos = shadow.lightPos;
	memcpy(params.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));

	if (m_meshDrawParams.renderStage == MESH_DRAW_SHADOW)
	{
		params.renderStage = POINT_DRAW_SHADOW;
		params.mode = 2;
	}
	else
	{
		params.renderStage = POINT_DRAW_LIGHT;
	}
	
	params.shadowMap = (ShadowMapD3D*)m_shadowMap;

	m_pointRenderer->draw(&params, buffers.m_positions.Get(), buffers.m_densities.Get(), buffers.m_indices.Get(), n, offset);
}

#if 0
extern Mesh* g_mesh;
void DrawShapes();
#endif

void DemoContextD3D11::renderEllipsoids(FluidRenderer* rendererIn, FluidRenderBuffers* buffersIn, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug)
{
	FluidRenderBuffersD3D11& buffers = *reinterpret_cast<FluidRenderBuffersD3D11*>(buffersIn);
	FluidRendererD3D11& renderer = *(FluidRendererD3D11*)rendererIn;
	if (n == 0)
		return;

	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;

	FluidDrawParamsD3D params;

	params.renderMode = FLUID_RENDER_SOLID;
	params.cullMode = FLUID_CULL_NONE;// FLUID_CULL_BACK;
	params.model = (const XMMATRIX&)Matrix44::kIdentity;
	params.view = (const XMMATRIX&)m_view;
	params.projection = (XMMATRIX&)m_proj;

	params.offset = offset;
	params.n = n;
	params.renderStage = FLUID_DRAW_LIGHT;

	const float viewHeight = tanf(fov / 2.0f);
	params.invViewport = float3(1.0f / screenWidth, screenAspect / screenWidth, 1.0f);
	params.invProjection = float3(screenAspect * viewHeight, viewHeight, 1.0f);

	// make sprites larger to get smoother thickness texture
	const float thicknessScale = 4.0f;
	params.pointRadius = thicknessScale * radius;

	params.shadowMap = (ShadowMapD3D*)m_shadowMap;

	renderer.m_thicknessTexture.bindAndClear(m_appGraphCtxD3D11->m_deviceContext);
#if 0
	// This seems redundant.
	{
		m_meshDrawParams.renderStage = MESH_DRAW_LIGHT;
		m_meshDrawParams.cullMode = MESH_CULL_NONE;

		if (g_mesh)
			DrawMesh(g_mesh, Vec3(1.0f));

		DrawShapes();

		m_meshDrawParams.renderStage = MESH_DRAW_NULL;
		m_meshDrawParams.cullMode = MESH_CULL_BACK;
	}
#endif
	renderer.drawThickness(&params, &buffers);

	renderer.m_depthTexture.bindAndClear(m_appGraphCtxD3D11->m_deviceContext);
	renderer.drawEllipsoids(&params, &buffers);

	//---------------------------------------------------------------
	// build smooth depth

	renderer.m_depthSmoothTexture.bindAndClear(deviceContext);

	params.blurRadiusWorld = radius * 0.5f;
	params.blurScale = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.invTexScale = float4(1.0f / screenAspect, 1.0f, 0.0f, 0.0f);
	params.blurFalloff = blur;
	params.debug = debug;

	renderer.drawBlurDepth(&params);

	//---------------------------------------------------------------
	// composite

	{
		deviceContext->RSSetViewports(1, &m_appGraphCtxD3D11->m_viewport);
		deviceContext->RSSetScissorRects(0, nullptr);

		deviceContext->OMSetRenderTargets(1, &m_appGraphCtxD3D11->m_rtv, m_appGraphCtxD3D11->m_dsv);
		deviceContext->OMSetDepthStencilState(m_appGraphCtxD3D11->m_depthState, 0u);

		float blendFactor[4] = { 1.0, 1.0, 1.0, 1.0 };
		deviceContext->OMSetBlendState(m_compositeBlendState, blendFactor, 0xffff);
	}

	params.invTexScale = (float4&)Vec2(1.0f / screenWidth, screenAspect / screenWidth);
	params.clipPosToEye = (float4&)Vec2(tanf(fov*0.5f)*screenAspect, tanf(fov*0.5f));
	params.color = (float4&)color;
	params.ior = ior;
	params.spotMin = m_spotMin;
	params.spotMax = m_spotMax;
	params.debug = debug;

	params.lightPos = (const float3&)lightPos;
	params.lightDir = (const float3&)-Normalize(lightTarget - lightPos);
	params.lightTransform = (const XMMATRIX&)(ConvertToD3DProjection(lightTransform));

	// Resolve MS back buffer/copy
	if (m_msaaSamples > 1)
	{
		deviceContext->ResolveSubresource(m_fluidResolvedTarget, 0, m_appGraphCtxD3D11->m_backBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}
	else
	{
		deviceContext->CopyResource(m_fluidResolvedTarget, m_appGraphCtxD3D11->m_backBuffer);
	}

	renderer.drawComposite(&params, m_fluidResolvedTargetSRV);

	deviceContext->OMSetBlendState(nullptr, 0, 0xffff);
}

DiffuseRenderBuffers* DemoContextD3D11::createDiffuseRenderBuffers(int numParticles, bool& enableInterop)
{
	ID3D11Device* device = m_appGraphCtxD3D11->m_device;
	DiffuseRenderBuffersD3D11* buffers = new DiffuseRenderBuffersD3D11;

	buffers->m_numParticles = numParticles;
	if (numParticles > 0)
	{
		{
			D3D11_BUFFER_DESC bufDesc;
			bufDesc.ByteWidth = numParticles * sizeof(Vec4);
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufDesc.MiscFlags = 0;
			if (enableInterop)
			{
				bufDesc.CPUAccessFlags = 0;
				bufDesc.Usage = D3D11_USAGE_DEFAULT;
				bufDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
			}

			device->CreateBuffer(&bufDesc, NULL, buffers->m_positions.ReleaseAndGetAddressOf());
			device->CreateBuffer(&bufDesc, NULL, buffers->m_velocities.ReleaseAndGetAddressOf()); 
		}

		if (enableInterop)
		{
			extern NvFlexLibrary* g_flexLib;

			buffers->m_positionsBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_positions.Get(), numParticles, sizeof(Vec4));
			buffers->m_velocitiesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers->m_velocities.Get(), numParticles, sizeof(Vec4));

			if (buffers->m_positionsBuf		== nullptr ||
				buffers->m_velocitiesBuf	== nullptr)
				enableInterop = false;
		}
	}

	return reinterpret_cast<DiffuseRenderBuffers*>(buffers);
}

void DemoContextD3D11::destroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers)
{
	delete reinterpret_cast<DiffuseRenderBuffersD3D11*>(buffers);
}

void DemoContextD3D11::updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffersIn, NvFlexSolver* solver)
{
	DiffuseRenderBuffersD3D11* buffers = reinterpret_cast<DiffuseRenderBuffersD3D11*>(buffersIn);
	// diffuse particles
	if (buffers->m_numParticles)
	{
		NvFlexGetDiffuseParticles(solver, buffers->m_positionsBuf, buffers->m_velocitiesBuf, nullptr);
	}
}

void DemoContextD3D11::updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffersIn, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles) 
{
	DiffuseRenderBuffersD3D11& buffers = *reinterpret_cast<DiffuseRenderBuffersD3D11*>(buffersIn);
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;

	D3D11_MAPPED_SUBRESOURCE res;

	// vertices
	if (diffusePositions)
	{
		deviceContext->Map(buffers.m_positions.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, diffusePositions, sizeof(Vec4)*numDiffuseParticles);
		deviceContext->Unmap(buffers.m_positions.Get(), 0);
	}

	if (diffuseVelocities)
	{
		deviceContext->Map(buffers.m_velocities.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, diffuseVelocities, sizeof(Vec4)*numDiffuseParticles);
		deviceContext->Unmap(buffers.m_velocities.Get(), 0);
	}
}

void DemoContextD3D11::drawDiffuse(FluidRenderer* renderIn, const DiffuseRenderBuffers* buffersIn, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front)
{
	FluidRendererD3D11* render = (FluidRendererD3D11*)renderIn;
	const DiffuseRenderBuffersD3D11& buffers = *reinterpret_cast<const DiffuseRenderBuffersD3D11*>(buffersIn);
	if (n == 0)
		return;

	DiffuseDrawParamsD3D params;

	params.model = (const XMMATRIX&)Matrix44::kIdentity;
	params.view = (const XMMATRIX&)m_view;
	params.projection = (const XMMATRIX&)m_proj;
	params.diffuseRadius = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.diffuseScale = radius;
	params.spotMin = m_spotMin;
	params.spotMax = m_spotMax;
	params.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	params.motionScale = motionBlur;

	// set shadow parameters
	ShadowParamsD3D shadow;
	RenderParamsUtilD3D::calcShadowParams(lightPos, lightTarget, lightTransform, m_shadowBias, &shadow);
	params.lightTransform = shadow.lightTransform;
	params.lightDir = shadow.lightDir;
	params.lightPos = shadow.lightPos;
	params.shadowMap = (ShadowMapD3D*)m_shadowMap;

	memcpy(params.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));
	
	m_diffuseRenderer->draw(&params, buffers.m_positions.Get(), buffers.m_velocities.Get(), n);

	// reset depth stencil state
	m_appGraphCtxD3D11->m_deviceContext->OMSetDepthStencilState(m_appGraphCtxD3D11->m_depthState, 0u);
}

int DemoContextD3D11::getNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers)
{
	return reinterpret_cast<DiffuseRenderBuffersD3D11*>(buffers)->m_numParticles;
}

void DemoContextD3D11::beginLines() 
{
}

void DemoContextD3D11::drawLine(const Vec3& p, const Vec3& q, const Vec4& color) 
{
	m_debugLineRender->addLine(p, q, color);
}

void DemoContextD3D11::endLines() 
{
	// draw
	Matrix44 projectionViewWorld = ((Matrix44&)(m_meshDrawParams.projection))*((Matrix44&)(m_meshDrawParams.view));

	m_debugLineRender->flush(projectionViewWorld);
}

void DemoContextD3D11::startGpuWork() {}
void DemoContextD3D11::endGpuWork() {}

void DemoContextD3D11::flushGraphicsAndWait() 
{
	ID3D11DeviceContext* deviceContext = m_appGraphCtxD3D11->m_deviceContext;

	deviceContext->End(m_renderCompletionFence);
	while (S_OK != deviceContext->GetData(m_renderCompletionFence, 0, 0, 0));
}

void* DemoContextD3D11::getGraphicsCommandQueue() { return NULL; }

void DemoContextD3D11::drawImguiGraph() 
{ 
	imguiGraphDraw();
}
