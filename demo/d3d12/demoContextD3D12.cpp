// to fix min max windows macros
#define NOMINMAX

#include "meshRendererD3D12.h"

// Pipelines

#include "meshRenderPipelineD3D12.h"
#include "pointRenderPipelineD3D12.h"
#include "fluidThicknessRenderPipelineD3D12.h"
#include "fluidEllipsoidRenderPipelineD3D12.h"
#include "fluidSmoothRenderPipelineD3D12.h"
#include "fluidCompositeRenderPipelineD3D12.h"
#include "diffusePointRenderPipelineD3D12.h"
#include "lineRenderPipelineD3D12.h"

#include "meshUtil.h"

#include <NvCoDx12RenderTarget.h>

// SDL
#include <SDL_syswm.h>

#include "shadersDemoContext.h"

// Flex
#include "core/maths.h"
#include "core/extrude.h"

#define NOMINMAX
#include <d3d12.h>
#include <d3dcompiler.h>

#include "imguiGraphD3D12.h"

#include "../d3d/loader.h"

#include "demoContextD3D12.h"

// include the Direct3D Library file
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "DXGI.lib")

using namespace DirectX;

static float gSpotMin = 0.5f;
static float gSpotMax = 1.0f;
static float gShadowBias = 0.075f;
static const int kShadowResolution = 2048;

// Global externally
extern Colour g_colors[];

#define NV_PRINT_F_U64 "%I64u"

DemoContext* CreateDemoContextD3D12()
{
	return new FlexSample::DemoContextD3D12;
}

namespace FlexSample {  

DemoContextD3D12::DemoContextD3D12()
{
	m_shadowMapLinearSamplerIndex = -1;
	m_fluidPointDepthSrvIndex = -1;
	m_currentShadowMap = nullptr;
	m_targetShadowMap = nullptr;
	
	m_inLineDraw = false;
	memset(&m_meshDrawParams, 0, sizeof(m_meshDrawParams));

	m_hwnd = nullptr;
	m_window = nullptr;

	// Allocate space for all debug vertices
	m_debugLineVertices.resize(MAX_DEBUG_LINE_SIZE);

	m_renderStateManager = new RenderStateManagerD3D12;
}

template <class T>
void inline COMRelease(T& t)
{
	if (t) t->Release();
	t = nullptr;
}

DemoContextD3D12::~DemoContextD3D12()
{
	imguiGraphDestroy();

	AppGraphCtxRelease(m_appGraphCtx);

	delete m_renderStateManager;

	COMRelease(m_graphicsCompleteFence);
	COMRelease(m_queryHeap);
	COMRelease(m_queryResults);

	COMRelease(m_bufferStage);

	// Explicitly delete these, so we can call D3D memory leak checker at bottom of this destructor
	m_meshPipeline.reset();
	m_pointPipeline.reset();
	m_fluidPointPipeline.reset();
	m_fluidSmoothPipeline.reset();
	m_fluidCompositePipeline.reset();
	m_diffusePointPipeline.reset();
	m_linePipeline.reset();
	m_fluidThicknessRenderTarget.reset();
	m_fluidPointRenderTarget.reset();
	m_fluidSmoothRenderTarget.reset();
	m_fluidResolvedTarget.reset();					
	m_screenQuadMesh.reset();
	m_shadowMap.reset();
}

bool DemoContextD3D12::initialize(const RenderInitOptions& options)
{
	// must always have at least one sample
	m_msaaSamples = Max(1, options.numMsaaSamples);

	{
		// Load external modules
		loadModules(APP_CONTEXT_D3D12);
	}

	m_appGraphCtx = AppGraphCtxCreate(0);
	m_renderContext = cast_to_AppGraphCtxD3D12(m_appGraphCtx);

	AppGraphCtxUpdateSize(m_appGraphCtx, options.window, options.fullscreen, m_msaaSamples);

	using namespace NvCo;
	// Get the hwnd
	m_hwnd = nullptr;
	m_window = options.window;
	{
		// get Windows handle to this SDL window
		SDL_SysWMinfo winInfo;
		SDL_VERSION(&winInfo.version);
		if (SDL_GetWindowWMInfo(options.window, &winInfo))
		{
			if (winInfo.subsystem == SDL_SYSWM_WINDOWS)
			{
				m_hwnd = winInfo.info.win.window;
			}
		}
	}

	{
		WCHAR buffer[_MAX_PATH];
		DWORD size = GetModuleFileNameW(nullptr, buffer, _MAX_PATH);
		if (size == 0 || size == _MAX_PATH)
		{
			// Method failed or path was truncated.
			return false;
		}
		std::wstring path;
		path += buffer;
		const size_t lastSlash = path.find_last_of(L"\\");
		if (lastSlash >= 0)
		{
			path.resize(lastSlash + 1);
		}

		m_executablePath.swap(path);
	}

	{
		m_shadersPath = m_executablePath;
		m_shadersPath += L"../../demo/d3d/shaders/";
	}
	
	int width, height;
	SDL_GetWindowSize(m_window, &width, &height);

	{
		ScopeGpuWork scope(getRenderContext());
		NV_RETURN_FALSE_ON_FAIL(_initRenderResources(options));

		{
			// create imgui, connect to app graph context
			ImguiGraphDescD3D12 desc;
			desc.device = m_renderContext->m_device;
			desc.commandList = m_renderContext->m_commandList;
			desc.lastFenceCompleted = 0;
			desc.nextFenceValue = 1;
			desc.winW = m_renderContext->m_winW;
			desc.winW = m_renderContext->m_winH;
			desc.numMSAASamples = options.numMsaaSamples;
			desc.dynamicHeapCbvSrvUav.userdata = this;
			desc.dynamicHeapCbvSrvUav.reserveDescriptors = NULL;

			int defaultFontHeight = (options.defaultFontHeight <= 0) ? 15 : options.defaultFontHeight;
			
			if (!imguiGraphInit("../../data/DroidSans.ttf", float(defaultFontHeight), (ImguiGraphDesc*)&desc))
			{
				return false;
			}
		}
	}

	return true;
}

int DemoContextD3D12::_initRenderResources(const RenderInitOptions& options)
{
	AppGraphCtxD3D12* renderContext = getRenderContext();
	ID3D12Device* device = renderContext->m_device;

	{
		// https://msdn.microsoft.com/en-us/library/windows/desktop/dn859253(v=vs.85).aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476876(v=vs.85).aspx#Overview
		D3D12_FEATURE_DATA_D3D12_OPTIONS options;
		device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
	}

	{
		// Make enough space for largest _single_ dynamic buffer allocation
		NV_RETURN_ON_FAIL(m_renderStateManager->initialize(renderContext, 16 * 1024 * 1024));
		m_renderState = m_renderStateManager->getState();
	}

	// Create the renderer
	{
		std::unique_ptr<MeshRendererD3D12> renderer(new MeshRendererD3D12);
		NV_RETURN_ON_FAIL(renderer->initialize(m_renderState));
		m_meshRenderer = std::move(renderer);
	}

	{
		// NOTE! Must be in this order, as compositePS expects s0, s1 = linear, shadow samplers, in that order
		m_linearSamplerIndex = m_renderState.m_samplerDescriptorHeap->allocate();
		m_shadowMapLinearSamplerIndex = m_renderState.m_samplerDescriptorHeap->allocate();

		{
			// linear sampler with comparator - used for shadow map sampling
			D3D12_SAMPLER_DESC desc =
			{
				D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				0.0f, 0, D3D12_COMPARISON_FUNC_LESS_EQUAL,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f,
			};
			device->CreateSampler(&desc, m_renderState.m_samplerDescriptorHeap->getCpuHandle(m_shadowMapLinearSamplerIndex));
		}
		{
			// A regular linear sampler
			D3D12_SAMPLER_DESC desc =
			{
				D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				0.0, 0, D3D12_COMPARISON_FUNC_NEVER,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f,
			};
			device->CreateSampler(&desc, m_renderState.m_samplerDescriptorHeap->getCpuHandle(m_linearSamplerIndex));
		}
	}

	// Allocate the srvs used for fluid render targets
	{
		m_fluidPointDepthSrvIndex = m_renderState.m_srvCbvUavDescriptorHeap->allocate();
		m_fluidCompositeSrvBaseIndex = m_renderState.m_srvCbvUavDescriptorHeap->allocate(NUM_COMPOSITE_SRVS);
	}

	// Create the shadow map
	{
		m_shadowMap = std::unique_ptr<NvCo::Dx12RenderTarget>(new NvCo::Dx12RenderTarget);

		NvCo::Dx12RenderTarget::Desc desc;
		desc.init(kShadowResolution, kShadowResolution);
		desc.m_targetFormat = DXGI_FORMAT_UNKNOWN;
		desc.m_depthStencilFormat = DXGI_FORMAT_R32_TYPELESS;

		// Make a small shadow map so we can configure pipeline correctly
		NV_RETURN_ON_FAIL(m_shadowMap->init(renderContext, desc));

		m_shadowMap->setDebugName(L"ShadowMap");

		if (m_shadowMap->allocateSrvView(NvCo::Dx12RenderTarget::BUFFER_DEPTH_STENCIL, device, *m_renderState.m_srvCbvUavDescriptorHeap) < 0)
		{
			printf("Unable to allocate shadow buffer srv index");

			return NV_FAIL;
		}
	}

	// Init fluid resources
	NV_RETURN_ON_FAIL(_initFluidRenderTargets());

	// Create pipelines
	{

		// Mesh
		{
			std::unique_ptr<MeshRenderPipelineD3D12> pipeline(new MeshRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, FRONT_WINDING_COUNTER_CLOCKWISE, m_shadowMapLinearSamplerIndex, m_shadowMap.get(), options.asyncComputeBenchmark));
			m_meshPipeline = std::move(pipeline);
		}
		// Point
		{
			std::unique_ptr<PointRenderPipelineD3D12> pipeline(new PointRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, m_shadowMapLinearSamplerIndex, m_shadowMap.get()));
			m_pointPipeline = std::move(pipeline);
		}
		// FluidThickness
		{
			std::unique_ptr<FluidThicknessRenderPipelineD3D12> pipeline(new FluidThicknessRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, m_fluidThicknessRenderTarget.get()));
			m_fluidThicknessPipeline = std::move(pipeline);
		}
		// FluidPoint
		{
			std::unique_ptr<FluidEllipsoidRenderPipelineD3D12> pipeline(new FluidEllipsoidRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, m_fluidPointRenderTarget.get()));
			m_fluidPointPipeline = std::move(pipeline);
		}
		// FluidSmooth
		{
			std::unique_ptr<FluidSmoothRenderPipelineD3D12> pipeline(new FluidSmoothRenderPipelineD3D12(m_fluidPointDepthSrvIndex));
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, m_fluidSmoothRenderTarget.get()));
			m_fluidSmoothPipeline = std::move(pipeline);
		}
		// FluidComposite
		{
			std::unique_ptr<FluidCompositeRenderPipelineD3D12> pipeline(new FluidCompositeRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath));
			m_fluidCompositePipeline = std::move(pipeline);
		}
		// DiffusePoint
		{
			std::unique_ptr<DiffusePointRenderPipelineD3D12> pipeline(new DiffusePointRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, m_linearSamplerIndex, m_shadowMap.get()));
			m_diffusePointPipeline = std::move(pipeline);
		}
		// Line
		{
			std::unique_ptr<LineRenderPipelineD3D12> pipeline(new LineRenderPipelineD3D12);
			NV_RETURN_ON_FAIL(pipeline->initialize(m_renderState, m_shadersPath, m_shadowMap.get()));
			m_linePipeline = std::move(pipeline);
		}
	}

	{
		// Create a passthru screen quad
		uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
		Vec3 pos[] = { { -1, -1, 0} , {1, -1, 0}, {1, 1, 0}, {-1, 1, 0} };
		Vec2 uvs[] = { { 0, 0}, {1, 0}, {1, 1}, {0, 1} };

		MeshData mesh;
		mesh.init();

		mesh.indices = indices;
		mesh.positions = pos;
		mesh.texcoords = uvs;
		mesh.numFaces = _countof(indices) / 3;
		mesh.numVertices = _countof(pos);

		m_screenQuadMesh = std::unique_ptr<FlexSample::RenderMesh>(m_meshRenderer->createMesh(mesh));
	}

	// create synchronization objects
	{
		m_graphicsCompleteEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!m_graphicsCompleteEvent)
		{
			return E_FAIL;
		}
		NV_RETURN_ON_FAIL(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_graphicsCompleteFence)));
		m_graphicsCompleteFenceValue = 1;
	}

	{
		// Query heap and results buffer

		// Create timestamp query heap and results buffer
		const int queryCount = 2;
		D3D12_QUERY_HEAP_DESC queryHeapDesc = { D3D12_QUERY_HEAP_TYPE_TIMESTAMP, queryCount, 0 /* NodeMask */ };
		NV_RETURN_ON_FAIL(device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&m_queryHeap)));

		D3D12_HEAP_PROPERTIES heapProps =
		{
			D3D12_HEAP_TYPE_READBACK,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0
		};

		D3D12_RESOURCE_DESC queryBufDesc =
		{
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0u,
			queryCount * sizeof(uint64_t),
			1u,
			1u,
			1,
			DXGI_FORMAT_UNKNOWN,
			{ 1u, 0u },
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			D3D12_RESOURCE_FLAG_NONE
		};

		NV_RETURN_ON_FAIL(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&queryBufDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_queryResults)));

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.MipLevels = 1u;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Width = renderContext->m_winW;
		texDesc.Height = renderContext->m_winH;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		texDesc.DepthOrArraySize = 1u;
		texDesc.SampleDesc.Count = 1u;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		// get footprint information
		m_footprint = {};
		UINT64 uploadHeapSize = 0u;
		device->GetCopyableFootprints(&texDesc, 0u, 1u, 0u, &m_footprint, nullptr, nullptr, &uploadHeapSize);

		D3D12_RESOURCE_DESC bufferDesc = texDesc;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0u;
		bufferDesc.Width = uploadHeapSize;
		bufferDesc.Height = 1u;
		bufferDesc.DepthOrArraySize = 1u;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		NV_RETURN_ON_FAIL(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_bufferStage)));
	}
	return NV_OK;
}

int DemoContextD3D12::_initFluidRenderTargets()
{
	AppGraphCtxD3D12* renderContext = getRenderContext();
	ID3D12Device* device = renderContext->m_device;

	// Fluid thickness
	{
		{
			std::unique_ptr<NvCo::Dx12RenderTarget> target(new NvCo::Dx12RenderTarget);
			NvCo::Dx12RenderTarget::Desc desc;
			desc.init(renderContext->m_winW, renderContext->m_winH, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_TYPELESS);
			for (int i = 0; i != 4; i++) desc.m_targetClearColor[i] = 0;

			NV_RETURN_ON_FAIL(target->init(renderContext, desc));

			target->setDebugName(L"Fluid Thickness");
			m_fluidThicknessRenderTarget = std::move(target);
		}
	}

	// Fluid point render target
	{
		{
			std::unique_ptr<NvCo::Dx12RenderTarget> target(new NvCo::Dx12RenderTarget);
			NvCo::Dx12RenderTarget::Desc desc;
			desc.init(renderContext->m_winW, renderContext->m_winH, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_TYPELESS);
			for (int i = 0; i != 4; i++) desc.m_targetClearColor[i] = 0;

			NV_RETURN_ON_FAIL(target->init(renderContext, desc));

			target->setDebugName(L"Fluid Point");
			m_fluidPointRenderTarget = std::move(target);
		}
	}

	// Fluid smooth 
	{
		std::unique_ptr<NvCo::Dx12RenderTarget> target(new NvCo::Dx12RenderTarget);
		NvCo::Dx12RenderTarget::Desc desc;
		desc.init(renderContext->m_winW, renderContext->m_winH, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN);
		for (int i = 0; i != 4; i++) desc.m_targetClearColor[i] = 0;

		NV_RETURN_ON_FAIL(target->init(renderContext, desc));

		target->setDebugName(L"Fluid Smooth");
		m_fluidSmoothRenderTarget = std::move(target);
	}

	// The resolved target for final compose
	{
		std::unique_ptr<NvCo::Dx12RenderTarget> target(new NvCo::Dx12RenderTarget);
		NvCo::Dx12RenderTarget::Desc desc;
		Vec4 clearColor = { 1, 0, 1, 1 };
		desc.m_targetClearColor = clearColor;
		desc.init(renderContext->m_winW, renderContext->m_winH, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);

		NV_RETURN_ON_FAIL(target->init(renderContext, desc));

		target->setDebugName(L"Fluid Resolved Target");
		m_fluidResolvedTarget = std::move(target);
	}

	// Init all of the srvs!!!!
	{
		NvCo::Dx12DescriptorHeap& heap = *m_renderState.m_srvCbvUavDescriptorHeap;
		// Set up the srv for accessing this buffer
		{
			m_fluidPointRenderTarget->createSrv(device, heap, NvCo::Dx12RenderTarget::BUFFER_TARGET, m_fluidPointDepthSrvIndex);
			m_fluidThicknessRenderTarget->createSrv(device, heap, NvCo::Dx12RenderTarget::BUFFER_TARGET, m_fluidPointDepthSrvIndex + 1);
		}

		{
			// 0 - is the depth texture					m_fluidSmoothRenderTarget (target)
			// 1 - is the 'composite' scene texture		m_fluidResolvedTarget (target)
			// 2 - shadow texture						m_shadowMap (depth stencil)

			m_fluidSmoothRenderTarget->createSrv(device, heap, NvCo::Dx12RenderTarget::BUFFER_TARGET, m_fluidCompositeSrvBaseIndex + 0);
			m_fluidResolvedTarget->createSrv(device, heap, NvCo::Dx12RenderTarget::BUFFER_TARGET, m_fluidCompositeSrvBaseIndex + 1);
			m_shadowMap->createSrv(device, heap, NvCo::Dx12RenderTarget::BUFFER_DEPTH_STENCIL, m_fluidCompositeSrvBaseIndex + 2);
		}
	}

	return NV_OK;
}

void DemoContextD3D12::onSizeChanged(int width, int height, bool minimized)
{
	// Free any set render targets
	m_fluidThicknessRenderTarget.reset();
	m_fluidPointRenderTarget.reset();
	m_fluidSmoothRenderTarget.reset();
	m_fluidResolvedTarget.reset();

	AppGraphCtxUpdateSize(m_appGraphCtx, m_window, false, m_msaaSamples);

	// Will need to create the render targets..
	_initFluidRenderTargets();
}

void DemoContextD3D12::startFrame(FlexVec4 colorIn)
{
	AppGraphCtxD3D12* renderContext = getRenderContext();

	// Work out what what can be recovered, as GPU no longer accessing 
	m_renderStateManager->updateCompleted();

	AppGraphColor clearColor = { colorIn.x, colorIn.y, colorIn.z, colorIn.w };
	AppGraphCtxFrameStart(cast_from_AppGraphCtxD3D12(renderContext), clearColor);

	{
		MeshDrawParamsD3D& params = m_meshDrawParams;
		memset(&params, 0, sizeof(MeshDrawParamsD3D));
		params.renderStage = MESH_DRAW_LIGHT;
		params.renderMode = MESH_RENDER_SOLID;
		params.cullMode = MESH_CULL_BACK;
		params.projection = XMMatrixIdentity();
		params.view = XMMatrixIdentity();
		params.model = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f),
			DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)
		);
	}
}

void DemoContextD3D12::endFrame()
{
	{
		ImguiGraphDescD3D12 desc;
		desc.device = m_renderContext->m_device;
		desc.commandList = m_renderContext->m_commandList;
		desc.winW = m_renderContext->m_winW;
		desc.winH = m_renderContext->m_winH;

		imguiGraphUpdate((ImguiGraphDesc*)&desc);
	}

	AppGraphCtxD3D12* renderContext = getRenderContext();

	nvidia::Common::Dx12Resource& backBuffer = renderContext->m_backBuffers[renderContext->m_renderTargetIndex];
	if (renderContext->m_numMsaaSamples > 1)
	{
		// MSAA resolve	
		nvidia::Common::Dx12Resource& renderTarget = *renderContext->m_renderTargets[renderContext->m_renderTargetIndex];
		assert(&renderTarget != &backBuffer);
		// Barriers to wait for the render target, and the backbuffer to be in correct state
		{
			nvidia::Common::Dx12BarrierSubmitter submitter(renderContext->m_commandList);
			renderTarget.transition(D3D12_RESOURCE_STATE_RESOLVE_SOURCE, submitter);
			backBuffer.transition(D3D12_RESOURCE_STATE_RESOLVE_DEST, submitter);
		}
		// Do the resolve...
		renderContext->m_commandList->ResolveSubresource(backBuffer, 0, renderTarget, 0, renderContext->m_targetInfo.m_renderTargetFormats[0]);
	}

	// copy to staging buffer
	{
		{
			nvidia::Common::Dx12BarrierSubmitter submitter(renderContext->m_commandList);
			backBuffer.transition(D3D12_RESOURCE_STATE_COPY_SOURCE, submitter);
		}

		D3D12_TEXTURE_COPY_LOCATION dstCopy = {};
		D3D12_TEXTURE_COPY_LOCATION srcCopy = {};
		dstCopy.pResource = m_bufferStage;
		dstCopy.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dstCopy.PlacedFootprint = m_footprint;
		srcCopy.pResource = backBuffer;
		srcCopy.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcCopy.SubresourceIndex = 0u;
		renderContext->m_commandList->CopyTextureRegion(&dstCopy, 0, 0, 0, &srcCopy, nullptr);
	}

	{
		nvidia::Common::Dx12BarrierSubmitter submitter(renderContext->m_commandList);
		backBuffer.transition(D3D12_RESOURCE_STATE_PRESENT, submitter);
	}

	renderContext->m_commandList->Close();

	// submit command list
	ID3D12CommandList* cmdLists[] = { renderContext->m_commandList };
	renderContext->m_commandQueue->ExecuteCommandLists(1, cmdLists);

	renderContext->m_commandListOpenCount = 0;

	// Inform the manager that the work has been submitted
	m_renderStateManager->onGpuWorkSubmitted(renderContext->m_commandQueue);

	HANDLE completeEvent = m_graphicsCompleteEvent;

	renderContext->m_commandQueue->Signal(m_graphicsCompleteFence, m_graphicsCompleteFenceValue);
}

void DemoContextD3D12::getRenderDevice(void** device, void** context)
{
	*device = m_renderContext->m_device;
	*context = m_renderContext->m_commandQueue;
}

void DemoContextD3D12::startGpuWork()
{
	AppGraphCtxBeginGpuWork(m_renderContext);
}

void DemoContextD3D12::endGpuWork()
{
	AppGraphCtxEndGpuWork(m_renderContext);
}

void DemoContextD3D12::presentFrame(bool fullsync)
{
	AppGraphCtxFramePresent(cast_from_AppGraphCtxD3D12(m_renderContext), fullsync);
}

void DemoContextD3D12::readFrame(int* buffer, int width, int height)
{
	AppGraphCtxD3D12* renderContext = getRenderContext();
	
	AppGraphCtxWaitForGPU(renderContext);

	int *pData;
	m_bufferStage->Map(0, nullptr, (void**)&pData);
	// y-coordinate is flipped for DirectX
	for (int i = 0; i < height; i++)
	{
		memcpy(buffer + (width * i), ((int *)pData) + (width * (height - i)), width * sizeof(int));
	}
	m_bufferStage->Unmap(0, nullptr);
}

void DemoContextD3D12::getViewRay(int x, int y, FlexVec3& origin, FlexVec3& dir)
{
	//using namespace DirectX;
	AppGraphCtxD3D12* renderContext = getRenderContext();

	int width = renderContext->m_winW;
	int height = renderContext->m_winH;

	XMVECTOR nearVector = XMVector3Unproject(XMVectorSet(float(x), float(height - y), 0.0f, 0.0f), 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f, (XMMATRIX)m_proj, XMMatrixIdentity(), (XMMATRIX)m_view);
	XMVECTOR farVector = XMVector3Unproject(XMVectorSet(float(x), float(height - y), 1.0f, 0.0f), 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f, (XMMATRIX)m_proj, XMMatrixIdentity(), (XMMATRIX)m_view);

	origin = FlexVec3(XMVectorGetX(nearVector), XMVectorGetY(nearVector), XMVectorGetZ(nearVector));
	XMVECTOR tmp = farVector - nearVector;
	dir = Normalize(FlexVec3(XMVectorGetX(tmp), XMVectorGetY(tmp), XMVectorGetZ(tmp)));
}

void DemoContextD3D12::setView(Matrix44 view, Matrix44 projection)
{
	Matrix44 vp = projection*view;
	MeshDrawParamsD3D& params = m_meshDrawParams;

	params.model = XMMatrixIdentity();
	params.view = (XMMATRIX)view;
	params.projection = (XMMATRIX)(RenderParamsUtilD3D::convertGLToD3DProjection(projection));

	m_view = view;
	m_proj = RenderParamsUtilD3D::convertGLToD3DProjection(projection);
}

void DemoContextD3D12::renderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers* buffersIn, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, FlexVec3 lightPos, FlexVec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, FlexVec4 color, float blur, float ior, bool debug)
{
	FluidRenderBuffersD3D12& buffers = *reinterpret_cast<FluidRenderBuffersD3D12*>(buffersIn);
	if (n == 0)
		return;

	typedef PointRenderAllocationD3D12 Alloc;

	Alloc alloc;
	alloc.init(PRIMITIVE_POINT);

	alloc.m_numPrimitives = n;
	alloc.m_numPositions = 0;	// unused param
	alloc.m_offset = offset;

	alloc.m_indexBufferView = buffers.m_indicesView;
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_DENSITY] = buffers.m_densitiesView;
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION] = buffers.m_positionsView;
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1] = buffers.m_anisotropiesViewArr[0];
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY2] = buffers.m_anisotropiesViewArr[1];
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY3] = buffers.m_anisotropiesViewArr[2];

	AppGraphCtxD3D12* renderContext = getRenderContext();

	FluidDrawParamsD3D params;

	params.renderMode = FLUID_RENDER_SOLID;
	params.cullMode = FLUID_CULL_NONE;// FLUID_CULL_BACK;
	params.model = XMMatrixIdentity();
	params.view = (XMMATRIX&)m_view;
	params.projection = (XMMATRIX&)m_proj;

	//params.offset = offset;
	//params.n = n;
	params.renderStage = FLUID_DRAW_LIGHT;

	const float viewHeight = tanf(fov / 2.0f);
	params.invViewport = Hlsl::float3(1.0f / screenWidth, screenAspect / screenWidth, 1.0f);
	params.invProjection = Hlsl::float3(screenAspect * viewHeight, viewHeight, 1.0f);

	// make sprites larger to get smoother thickness texture
	const float thicknessScale = 4.0f;
	params.pointRadius = thicknessScale * radius;

	params.debug = 0;
	params.shadowMap = nullptr;

	{
		m_fluidThicknessRenderTarget->toWritable(renderContext);
		m_fluidThicknessRenderTarget->bindAndClear(renderContext);

		m_meshRenderer->drawTransitory(alloc, sizeof(Alloc), m_fluidThicknessPipeline.get(), &params);

		m_fluidThicknessRenderTarget->toReadable(renderContext);
	}

	{
		m_fluidPointRenderTarget->toWritable(renderContext);
		m_fluidPointRenderTarget->bindAndClear(renderContext);

		// Draw the ellipsoids
		m_meshRenderer->drawTransitory(alloc, sizeof(Alloc), m_fluidPointPipeline.get(), &params);

		m_fluidPointRenderTarget->toReadable(renderContext);
	}

	//---------------------------------------------------------------
	// build smooth depth

	{
		m_fluidSmoothRenderTarget->toWritable(renderContext);
		m_fluidSmoothRenderTarget->bind(renderContext);

		params.blurRadiusWorld = radius * 0.5f;
		params.blurScale = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
		params.invTexScale = Hlsl::float4(1.0f / screenAspect, 1.0f, 0.0f, 0.0f);
		params.blurFalloff = blur;
		params.debug = debug;
		params.m_sampleDescriptorBase = m_linearSamplerIndex;

		m_meshRenderer->draw(m_screenQuadMesh.get(), m_fluidSmoothPipeline.get(), &params);

		m_fluidSmoothRenderTarget->toReadable(renderContext);
	}

	// First lets resolve the render target 
	ID3D12GraphicsCommandList* commandList = m_renderContext->m_commandList;
	{
		// Lets take what's rendered so far and 
		NvCo::Dx12ResourceBase* targetResource = m_renderContext->m_renderTargets[m_renderContext->m_renderTargetIndex];
		NvCo::Dx12ResourceBase& fluidResolvedTarget = m_fluidResolvedTarget->getResource(NvCo::Dx12RenderTarget::BUFFER_TARGET);

		if (m_renderContext->m_numMsaaSamples > 1)
		{
			// If enabled can stop/reduce flickering issues on some GPUs related to a problem around ResolveSubresource/ 
			//m_renderContext->submitGpuWork();

			{
				NvCo::Dx12BarrierSubmitter submitter(commandList);
				targetResource->transition(D3D12_RESOURCE_STATE_RESOLVE_SOURCE, submitter);
				fluidResolvedTarget.transition(D3D12_RESOURCE_STATE_RESOLVE_DEST, submitter);
			}
			// Do the resolve
			const DXGI_FORMAT format = fluidResolvedTarget.getResource()->GetDesc().Format;
			commandList->ResolveSubresource(fluidResolvedTarget, 0, *targetResource, 0, format);
		}
		else
		{
			{
				NvCo::Dx12BarrierSubmitter submitter(commandList);
				targetResource->transition(D3D12_RESOURCE_STATE_COPY_SOURCE, submitter);
				fluidResolvedTarget.transition(D3D12_RESOURCE_STATE_COPY_DEST, submitter);
			}
			commandList->CopyResource(fluidResolvedTarget, *targetResource);
		}

		{
			NvCo::Dx12BarrierSubmitter submitter(commandList);
			targetResource->transition(D3D12_RESOURCE_STATE_RENDER_TARGET, submitter);
			fluidResolvedTarget.transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, submitter);
		}
	}

	//---------------------------------------------------------------
	// composite

	{
		m_shadowMap->toReadable(renderContext);

		AppGraphCtxPrepareRenderTarget(renderContext);

		FlexVec4 aspectWork(1.0f / screenWidth, screenAspect / screenWidth, 0, 0);
		params.invTexScale = (Hlsl::float4&)aspectWork;
		FlexVec4 clipPosToEyeWork(tanf(fov*0.5f)*screenAspect, tanf(fov*0.5f), 0, 0);
		params.clipPosToEye = (Hlsl::float4&)clipPosToEyeWork;
		params.color = (Hlsl::float4&)color;
		params.ior = ior;
		params.spotMin = gSpotMin;
		params.spotMax = gSpotMax;
		params.debug = debug;

		params.lightPos = (Hlsl::float3&)lightPos;
		FlexVec3 lightDirWork = -Normalize(lightTarget - lightPos);
		params.lightDir = (Hlsl::float3&)lightDirWork;
		Matrix44 lightTransformWork = RenderParamsUtilD3D::convertGLToD3DProjection(lightTransform);
		params.lightTransform = (XMMATRIX&)lightTransformWork;

		params.m_srvDescriptorBase = m_fluidCompositeSrvBaseIndex;
		params.m_sampleDescriptorBase = m_linearSamplerIndex;
		params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

		m_meshRenderer->draw(m_screenQuadMesh.get(), m_fluidCompositePipeline.get(), &params);
	}
}

void DemoContextD3D12::updateFluidRenderBuffers(FluidRenderBuffers* buffersIn, FlexVec4* particles, float* densities, FlexVec4* anisotropy1, FlexVec4* anisotropy2, FlexVec4* anisotropy3, int numParticles, int* indices, int numIndices)
{
	FluidRenderBuffersD3D12& buffers = *reinterpret_cast<FluidRenderBuffersD3D12*>(buffersIn);

	typedef PointRenderAllocationD3D12 Alloc;
	Alloc alloc;

	PointData pointData;

	pointData.positions = (const Vec4*)particles;
	pointData.density = densities;
	pointData.phase = nullptr;
	pointData.indices = (uint32_t*)indices;
	pointData.numIndices = numIndices;
	pointData.numPoints = numParticles;
	pointData.anisotropy[0] = (const Vec4*)anisotropy1;
	pointData.anisotropy[1] = (const Vec4*)anisotropy2;
	pointData.anisotropy[2] = (const Vec4*)anisotropy3;

	m_meshRenderer->allocateTransitory(pointData, alloc, sizeof(alloc));
	
	buffers.m_positionsView = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION];
	buffers.m_densitiesView = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_DENSITY];
	
	buffers.m_anisotropiesViewArr[0] = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1];
	buffers.m_anisotropiesViewArr[1] = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY2];
	buffers.m_anisotropiesViewArr[2] = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY3];
	
	buffers.m_indicesView = alloc.m_indexBufferView;

	buffers.m_numParticles = numIndices;
}

void DemoContextD3D12::shadowBegin(::ShadowMap* map)
{
	assert(map);
	assert(m_targetShadowMap == nullptr);

	AppGraphCtxD3D12* renderContext = getRenderContext();
	NvCo::Dx12RenderTarget* shadowMap = reinterpret_cast<NvCo::Dx12RenderTarget*>(map);

	shadowMap->toWritable(renderContext);
	shadowMap->bindAndClear(renderContext);

	m_targetShadowMap = shadowMap;

	m_meshDrawParams.renderStage = MESH_DRAW_SHADOW;
}

void DemoContextD3D12::shadowEnd()
{
	AppGraphCtxD3D12* renderContext = getRenderContext();
	NvCo::Dx12RenderTarget* shadowMap = m_targetShadowMap;
	shadowMap->toReadable(renderContext);
	m_targetShadowMap = nullptr;

	// Restore to regular render target
	AppGraphCtxPrepareRenderTarget(renderContext);

	m_meshDrawParams.renderStage = MESH_DRAW_NULL;
}

void DemoContextD3D12::drawMesh(const Mesh* m, FlexVec3 color)
{
	MeshDrawParamsD3D& params = m_meshDrawParams;

	if (m)
	{
		MeshData meshData;

		meshData.positions = (Vec3*)&m->m_positions[0];
		meshData.normals = (Vec3*)&m->m_normals[0];
		meshData.colors = nullptr;
		meshData.texcoords = nullptr;
		meshData.indices = (uint32_t*)&m->m_indices[0];
		meshData.numFaces = m->GetNumFaces();
		meshData.numVertices = m->GetNumVertices();

		params.colorArray = 0;
		if (m->m_colours.size())
		{
			params.colorArray = 1;
			meshData.colors = (Vec4*)&m->m_colours[0];
		}

		params.color = Hlsl::float4(color.x, color.y, color.z, 1);
		params.secondaryColor = params.color;
		params.objectTransform = (Hlsl::float4x4&)Matrix44::kIdentity;
		params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

		m_meshRenderer->drawImmediate(meshData, m_meshPipeline.get(), &params);

		if (m->m_colours.size())
		{
			params.colorArray = 0;
		}
	}
}

void DemoContextD3D12::drawCloth(const FlexVec4* positions, const FlexVec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth)
{
	if (!numTris)
		return;

	MeshData2 meshData;

	meshData.positions = (const Vec4*)positions;
	meshData.normals = (const Vec4*)normals;
	meshData.texcoords = nullptr; /* (const Vec2*)uvs; */
	meshData.colors = nullptr;
	meshData.numFaces = numTris;
	meshData.indices = (const uint32_t*)indices;
	meshData.numVertices = numPositions;

	if (twosided)
		SetCullMode(false);

	MeshDrawParamsD3D& params = m_meshDrawParams;

	params.bias = 0.0f;
	params.expand = expand;

	params.color = (Hlsl::float4&)(g_colors[colorIndex + 1] * 1.5f);
	params.secondaryColor = (Hlsl::float4&)(g_colors[colorIndex] * 1.5f);
	params.objectTransform = (Hlsl::float4x4&)Matrix44::kIdentity;
	params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

	m_meshRenderer->drawImmediate(meshData, m_meshPipeline.get(), &params);

	if (twosided)
		SetCullMode(true);

	params.bias = gShadowBias;
	params.expand = 0.0f;
}

void DemoContextD3D12::drawRope(FlexVec4* positions, int* indices, int numIndices, float radius, int color)
{
	if (numIndices < 2)
		return;

	std::vector<FlexVec3> vertices;
	std::vector<FlexVec3> normals;
	std::vector<int> triangles;

	// flatten curve
	std::vector<FlexVec3> curve(numIndices);
	for (int i = 0; i < numIndices; ++i)
	{
		curve[i] = FlexVec3(positions[indices[i]]);
	}

	const int resolution = 8;
	const int smoothing = 3;

	vertices.reserve(resolution*numIndices*smoothing);
	normals.reserve(resolution*numIndices*smoothing);
	triangles.reserve(numIndices*resolution * 6 * smoothing);

	Extrude(&curve[0], int(curve.size()), vertices, normals, triangles, radius, resolution, smoothing);

	SetCullMode(false);

	MeshDrawParamsD3D& params = m_meshDrawParams;

	params.color = (Hlsl::float4&)(g_colors[color % 8] * 1.5f);
	params.secondaryColor = (Hlsl::float4&)(g_colors[color % 8] * 1.5f);
	params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

	MeshData meshData;

	meshData.positions = (const Vec3*)&vertices[0];
	meshData.normals = (const Vec3*)&normals[0];
	meshData.texcoords = nullptr;
	meshData.colors = nullptr;
	meshData.indices = (const uint32_t*)&triangles[0];
	meshData.numFaces = int(triangles.size()) / 3;
	meshData.numVertices = int(vertices.size());

	m_meshRenderer->drawImmediate(meshData, m_meshPipeline.get(), &params);

	SetCullMode(true);
}

void DemoContextD3D12::drawPlane(const FlexVec4& p, bool color)
{
	std::vector<FlexVec3> vertices;
	std::vector<FlexVec3> normals;
	std::vector<int> indices;

	FlexVec3 u, v;
	BasisFromVector(FlexVec3(p.x, p.y, p.z), &u, &v);

	FlexVec3 c = FlexVec3(p.x, p.y, p.z)*-p.w;

	MeshDrawParamsD3D& params = m_meshDrawParams;

	params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

	if (color)
		params.color = (Hlsl::float4&)(p * 0.5f + FlexVec4(0.5f, 0.5f, 0.5f, 0.5f));

	const float kSize = 200.0f;
	const int kGrid = 3;

	// draw a grid of quads, otherwise z precision suffers
	for (int x = -kGrid; x <= kGrid; ++x)
	{
		for (int y = -kGrid; y <= kGrid; ++y)
		{
			FlexVec3 coff = c + u*float(x)*kSize*2.0f + v*float(y)*kSize*2.0f;

			int indexStart = int(vertices.size());

			vertices.push_back(FlexVec3(coff + u*kSize + v*kSize));
			vertices.push_back(FlexVec3(coff - u*kSize + v*kSize));
			vertices.push_back(FlexVec3(coff - u*kSize - v*kSize));
			vertices.push_back(FlexVec3(coff + u*kSize - v*kSize));

			normals.push_back(FlexVec3(p.x, p.y, p.z));
			normals.push_back(FlexVec3(p.x, p.y, p.z));
			normals.push_back(FlexVec3(p.x, p.y, p.z));
			normals.push_back(FlexVec3(p.x, p.y, p.z));


			indices.push_back(indexStart + 0);
			indices.push_back(indexStart + 1);
			indices.push_back(indexStart + 2);

			indices.push_back(indexStart + 2);
			indices.push_back(indexStart + 3);
			indices.push_back(indexStart + 0);
		}
	}

	MeshData meshData;
	meshData.texcoords = nullptr;
	meshData.colors = nullptr;
	meshData.positions = (Vec3*)&vertices[0];
	meshData.normals = (Vec3*)&normals[0];
	meshData.indices = (uint32_t*)&indices[0];
	meshData.numFaces = int(indices.size() / 3);
	meshData.numVertices = int(vertices.size());

	m_meshRenderer->drawImmediate(meshData, m_meshPipeline.get(), &params);
}

void DemoContextD3D12::drawPlanes(FlexVec4* planes, int n, float bias)
{
	MeshDrawParamsD3D& params = m_meshDrawParams;

	params.color = (Hlsl::float4&)FlexVec4(0.9f, 0.9f, 0.9f, 1.0f);

	params.bias = 0.0f;
	params.grid = 1;
	params.expand = 0;

	for (int i = 0; i < n; ++i)
	{
		FlexVec4 p = planes[i];
		p.w -= bias;

		drawPlane(p, false);
	}

	params.grid = 0;
	params.bias = gShadowBias;
}

void DemoContextD3D12::drawPoints(FluidRenderBuffers* buffersIn, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, FlexVec3 lightPos, FlexVec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowTex, bool showDensity)
{
	FluidRenderBuffersD3D12* buffers = reinterpret_cast<FluidRenderBuffersD3D12*>(buffersIn);
	// Okay we can draw the particles here. The update can copy the point positions into the heap, so here we just draw them 

	if (n == 0)
		return;

	PointRenderAllocationD3D12 pointAlloc;
	pointAlloc.init(PRIMITIVE_POINT);
	pointAlloc.m_vertexBufferViews[PointRenderAllocationD3D12::VERTEX_VIEW_POSITION] = buffers->m_positionsView;
	// It says 'color' as the parameter but actually its the 'density' parameter that is passed in here
	pointAlloc.m_vertexBufferViews[PointRenderAllocationD3D12::VERTEX_VIEW_DENSITY] = buffers->m_densitiesView;
	pointAlloc.m_vertexBufferViews[PointRenderAllocationD3D12::VERTEX_VIEW_PHASE] = buffers->m_densitiesView;
	pointAlloc.m_indexBufferView = buffers->m_indicesView;

	pointAlloc.m_numPrimitives = n;
	pointAlloc.m_numPositions = 0;	// unused param
	pointAlloc.m_offset = offset;
	
	PointDrawParamsD3D params;

	params.renderMode = POINT_RENDER_SOLID;
	params.cullMode = POINT_CULL_BACK;
	params.model = XMMatrixIdentity();
	params.view = (XMMATRIX&)m_view;
	params.projection = (XMMATRIX&)m_proj;

	params.pointRadius = radius;
	params.pointScale = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.spotMin = gSpotMin;
	params.spotMax = gSpotMax;

	int mode = 0;
	if (showDensity)
		mode = 1;
	if (shadowTex == 0)
		mode = 2;
	params.mode = mode;

	for (int i = 0; i < 8; i++)
		params.colors[i] = *((Hlsl::float4*)&g_colors[i].r);

	// set shadow parameters
	ShadowParamsD3D shadow;
	RenderParamsUtilD3D::calcShadowParams(lightPos, lightTarget, lightTransform, gShadowBias, &shadow);
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
		params.renderStage = POINT_DRAW_LIGHT;

	params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

	m_meshRenderer->drawTransitory(pointAlloc, sizeof(pointAlloc), m_pointPipeline.get(), &params);
}

void DemoContextD3D12::graphicsTimerBegin()
{
	ID3D12GraphicsCommandList* commandList = m_renderContext->m_commandList;

	commandList->EndQuery(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
}

void DemoContextD3D12::graphicsTimerEnd()
{
	ID3D12GraphicsCommandList* commandList = m_renderContext->m_commandList;

	commandList->EndQuery(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
	commandList->ResolveQueryData(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, m_queryResults, 0);
}

float DemoContextD3D12::rendererGetDeviceTimestamps(unsigned long long* begin, unsigned long long* end, unsigned long long* freq)
{
	AppGraphCtxD3D12* renderContext = getRenderContext();
	ID3D12CommandQueue* commandQueue = m_renderContext->m_commandQueue;

	AppGraphCtxWaitForGPU(renderContext);

	// Get timer frequency
	static uint64_t frequency = 0;

	if (frequency == 0)
	{
		commandQueue->GetTimestampFrequency(&frequency);
	}

	// Get render timestamps
	uint64_t* times;
	D3D12_RANGE readRange = { 0, 2 };
	m_queryResults->Map(0, &readRange, (void**)&times);
	uint64_t renderBegin = times[0];
	uint64_t renderEnd = times[1];
	D3D12_RANGE writtenRange = { 0, 0 };   // nothing was written
	m_queryResults->Unmap(0, &writtenRange);

	double renderTime = double(renderEnd - renderBegin) / double(frequency);

	if (begin) *begin = renderBegin;
	if (end) *end = renderEnd;
	if (freq) *freq = frequency;

	return float(renderTime);
}

void DemoContextD3D12::bindSolidShader(FlexVec3 lightPos, FlexVec3 lightTarget, Matrix44 lightTransform, ::ShadowMap* shadowMap, float bias, FlexVec4 fogColor)
{
	MeshDrawParamsD3D& params = m_meshDrawParams;

	params.renderStage = MESH_DRAW_LIGHT;

	params.grid = 0;
	params.spotMin = gSpotMin;
	params.spotMax = gSpotMax;
	params.fogColor = (Hlsl::float4&)fogColor;

	params.objectTransform = (Hlsl::float4x4&)Matrix44::kIdentity;

	ShadowParamsD3D shadow;
	RenderParamsUtilD3D::calcShadowParams(lightPos, lightTarget, lightTransform, gShadowBias, &shadow);
	params.lightTransform = shadow.lightTransform;
	params.lightDir = shadow.lightDir;
	params.lightPos = shadow.lightPos;
	params.bias = shadow.bias;
	memcpy(params.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));

	m_currentShadowMap = (NvCo::Dx12RenderTarget*)shadowMap;
}

void DemoContextD3D12::drawGpuMesh(GpuMesh* m, const Matrix44& xform, const FlexVec3& color)
{
	if (m)
	{
		MeshDrawParamsD3D params(m_meshDrawParams);

		params.color = (Hlsl::float4&)color;
		params.secondaryColor = (Hlsl::float4&)color;
		params.objectTransform = (Hlsl::float4x4&)xform;
		params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

		RenderMesh* renderMesh = (RenderMesh*)m;

		m_meshRenderer->draw(renderMesh, m_meshPipeline.get(), &params);
	}
}

void DemoContextD3D12::drawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const FlexVec3& color)
{
	if (m)
	{
		MeshDrawParamsD3D& contextParams = m_meshDrawParams;

		contextParams.color = (Hlsl::float4&)color;
		contextParams.secondaryColor = (Hlsl::float4&)color;
		contextParams.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

		// copy params
		MeshDrawParamsD3D params(contextParams);

		for (int i = 0; i < n; ++i)
		{
			params.objectTransform = (Hlsl::float4x4&)xforms[i];
			RenderMesh* renderMesh = (RenderMesh*)m;
			m_meshRenderer->draw(renderMesh, m_meshPipeline.get(), &params);
		}
	}
}

void DemoContextD3D12::drawDiffuse(FluidRenderer* render, const DiffuseRenderBuffers* buffersIn, int n, float radius, float screenWidth, float screenAspect, float fov, FlexVec4 color, FlexVec3 lightPos, FlexVec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front)
{
	const DiffuseRenderBuffersD3D12& buffers = *reinterpret_cast<const DiffuseRenderBuffersD3D12*>(buffersIn);
	if (n == 0)
		return;

	typedef PointRenderAllocationD3D12 Alloc;
	Alloc alloc;
	alloc.init(PRIMITIVE_POINT);

	alloc.m_numPrimitives = n;
	alloc.m_numPositions = 0;	// unused param
	alloc.m_offset = 0;			// unused param

	alloc.m_indexBufferView = buffers.m_indicesView;
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION] = buffers.m_positionsView;
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1] = buffers.m_velocitiesView;			// Velocity stored in ANISO1

	DiffuseDrawParamsD3D params;

	params.model = XMMatrixIdentity();
	params.view = (const XMMATRIX&)m_view;
	params.projection = (const XMMATRIX&)m_proj;
	params.diffuseRadius = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.diffuseScale = radius;
	params.spotMin = gSpotMin;
	params.spotMax = gSpotMax;
	params.color = Hlsl::float4(1.0f, 1.0f, 1.0f, 1.0f);
	params.motionScale = motionBlur;

	// set shadow parameters
	ShadowParamsD3D shadow;
	RenderParamsUtilD3D::calcShadowParams(lightPos, lightTarget, lightTransform, gShadowBias, &shadow);
	params.lightTransform = shadow.lightTransform;
	params.lightDir = shadow.lightDir;
	params.lightPos = shadow.lightPos;
	params.shadowMap = (ShadowMapD3D*)m_currentShadowMap;

	memcpy(params.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));

	m_meshRenderer->drawTransitory(alloc, sizeof(alloc), m_diffusePointPipeline.get(), &params);
}

void DemoContextD3D12::beginLines()
{
	assert(!m_inLineDraw);
	m_inLineDraw = true;
	m_debugLineVertices.clear();
}

void DemoContextD3D12::drawLine(const Vec3& p, const Vec3& q, const Vec4& color)
{
	assert(m_inLineDraw);

	if (m_debugLineVertices.size() + 2 > MAX_DEBUG_LINE_SIZE)
	{
		_flushDebugLines();
	}

	LineData::Vertex dst[2];
	dst[0].position = (const Vec3&)p;
	dst[0].color = (const Vec4&)color;
	dst[1].position = (const Vec3&)q;
	dst[1].color = (const Vec4&)color;

	m_debugLineVertices.push_back(dst[0]);
	m_debugLineVertices.push_back(dst[1]);
}

void DemoContextD3D12::_flushDebugLines()
{
	assert(m_inLineDraw);

	if (m_debugLineVertices.size() > 0)
	{
		LineDrawParams params;

		const Matrix44 modelWorldProjection = ((Matrix44&)(m_meshDrawParams.projection)) * ((Matrix44&)(m_meshDrawParams.view));
		// draw
		params.m_modelWorldProjection = (const Hlsl::float4x4&)modelWorldProjection;
		params.m_drawStage = (m_targetShadowMap) ? LINE_DRAW_SHADOW : LINE_DRAW_NORMAL;

		LineData lineData;
		lineData.init();
		lineData.vertices = &m_debugLineVertices.front();
		lineData.numVertices = m_debugLineVertices.size();
		lineData.numLines = lineData.numVertices / 2;

		m_meshRenderer->drawImmediate(lineData, m_linePipeline.get(), &params);
	
		m_debugLineVertices.clear();
	}
}

void DemoContextD3D12::endLines()
{
	_flushDebugLines();
	// No longer in line drawing
	m_inLineDraw = false;
}

void DemoContextD3D12::flushGraphicsAndWait() 
{
	AppGraphCtxWaitForGPU(getRenderContext());
}

FluidRenderer* DemoContextD3D12::createFluidRenderer(uint32_t width, uint32_t height)
{
	// It's always created.. so just return the context
	return (FluidRenderer*)this;
}

void DemoContextD3D12::destroyFluidRenderer(FluidRenderer* renderer)
{
	/// Don't need to do anything, as will be destroyed with context
}

FluidRenderBuffers* DemoContextD3D12::createFluidRenderBuffers(int numParticles, bool enableInterop)
{
	return reinterpret_cast<FluidRenderBuffers*>(new FluidRenderBuffersD3D12(numParticles));
}

void DemoContextD3D12::updateFluidRenderBuffers(FluidRenderBuffers* buffers, NvFlexSolver* flex, bool anisotropy, bool density)
{
	printf("Not implemented");
	assert(0);
}

void DemoContextD3D12::destroyFluidRenderBuffers(FluidRenderBuffers* buffers)
{
	delete reinterpret_cast<FluidRenderBuffersD3D12*>(buffers);
}

ShadowMap* DemoContextD3D12::shadowCreate()
{
	// Currently only allows single shadow map, which is pre created
	return (::ShadowMap*)m_shadowMap.get();
}

void DemoContextD3D12::shadowDestroy(ShadowMap* map)
{
	assert(map);
	assert(m_shadowMap.get() == (NvCo::Dx12RenderTarget*)map);
}

void DemoContextD3D12::unbindSolidShader()
{
	m_meshDrawParams.renderStage = MESH_DRAW_NULL;

	// !!! Other code appears to assume that this will be set
	//context->m_currentShadowMap = nullptr;
}

GpuMesh* DemoContextD3D12::createGpuMesh(const Mesh* m)
{
	if (m)
	{
		return (GpuMesh*)MeshUtil::createRenderMesh(m_meshRenderer.get(), *m);
	}
	return nullptr;
}

void DemoContextD3D12::destroyGpuMesh(GpuMesh* m)
{
	delete reinterpret_cast<RenderMesh*>(m);
}

DiffuseRenderBuffers* DemoContextD3D12::createDiffuseRenderBuffers(int numDiffuseParticles, bool& enableInterop)
{
	return reinterpret_cast<DiffuseRenderBuffers*>(new DiffuseRenderBuffersD3D12(numDiffuseParticles));
}

void DemoContextD3D12::destroyDiffuseRenderBuffers(DiffuseRenderBuffers* buffers)
{
	delete reinterpret_cast<DiffuseRenderBuffersD3D12*>(buffers);
}

int DemoContextD3D12::getNumDiffuseRenderParticles(DiffuseRenderBuffers* buffers)
{
	return reinterpret_cast<DiffuseRenderBuffersD3D12*>(buffers)->m_numParticles;
}

void DemoContextD3D12::updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffersIn, Vec4* diffusePositions, Vec4* diffuseVelocities, int numDiffuseParticles)
{
	DiffuseRenderBuffersD3D12& buffers = *reinterpret_cast<DiffuseRenderBuffersD3D12*>(buffersIn);

	typedef PointRenderAllocationD3D12 Alloc;

	Alloc alloc;

	PointData pointData;
	pointData.init();
	pointData.numIndices = numDiffuseParticles;
	pointData.positions = (Vec4*)diffusePositions;
	pointData.anisotropy[0] = (Vec4*)diffuseVelocities;			// We'll store the velocities in the anisotropy buffer, cos it's the right size
	pointData.numPoints = buffers.m_numParticles;

	m_meshRenderer->allocateTransitory(pointData, alloc, sizeof(alloc));

	buffers.m_indicesView = alloc.m_indexBufferView;
	buffers.m_positionsView = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION];
	buffers.m_velocitiesView = alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1];			// The velocity

	buffers.m_numParticles = numDiffuseParticles;
}

void DemoContextD3D12::updateDiffuseRenderBuffers(DiffuseRenderBuffers* buffers, NvFlexSolver* solver)
{
	printf("Not implemented");
	assert(0);
}

void* DemoContextD3D12::getGraphicsCommandQueue() 
{ 
	return m_renderContext->m_commandQueue; 
}

void DemoContextD3D12::setFillMode(bool wire)
{
	m_meshDrawParams.renderMode = wire ? MESH_RENDER_WIREFRAME : MESH_RENDER_SOLID;
}

void DemoContextD3D12::setCullMode(bool enabled)
{
	m_meshDrawParams.cullMode = enabled ? MESH_CULL_BACK : MESH_CULL_NONE;
}

void DemoContextD3D12::drawImguiGraph()
{
	imguiGraphDraw();
}

} // namespace FlexSample 
