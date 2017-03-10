#include "core/maths.h"
#include "core/extrude.h"

#include "shaders.h"

#include "meshRender.h"
#include "pointRender.h"
#include "fluidRender.h"
#include "diffuseRender.h"
#include "debugLineRender.h"

#include "shadowMap.h"
#include "renderTarget.h"

#include "imguiGraph.h"
#include "imguiGraphD3D11.h"

#include "appD3D11Ctx.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <cstdlib>

DebugLineRender gDebugLineRender;
MeshRenderer gMeshRenderer;
PointRenderer gPointRenderer;
DiffuseRenderer gDiffuseRenderer;

AppGraphCtx* gAppGraphCtx;

static float gSpotMin = 0.5f;
static float gSpotMax = 1.0f;
float gShadowBias = 0.075f;

struct RenderContext
{
	RenderContext() 
	{
		memset(&mMeshDrawParams, 0, sizeof(MeshDrawParams));
	}

	MeshDrawParams mMeshDrawParams;

	Matrix44 view;
	Matrix44 proj;

	ShadowMap* shadowMap;
	GpuMesh* immediateMesh;

	SDL_Window* window;

	int msaaSamples;
};

RenderContext gContext;


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

void InitRender(SDL_Window* window, bool fullscreen, int msaaSamples)
{
	// must always have at least one sample
	msaaSamples = Max(1, msaaSamples);

	// create app graph context
	gAppGraphCtx = AppGraphCtxCreate(0);

	AppGraphCtxInitRenderTarget(gAppGraphCtx, window, fullscreen, msaaSamples);
	//gScene = getScene(0);

	float clearVal[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	AppGraphCtxFrameStart(gAppGraphCtx, clearVal);

	// create imgui, connect to app graph context

	ImguiGraphDesc desc;
	desc.device = gAppGraphCtx->m_device;
	desc.deviceContext = gAppGraphCtx->m_deviceContext;
	desc.winW = gAppGraphCtx->m_winW;
	desc.winH = gAppGraphCtx->m_winH;

	imguiGraphInit("../../data/DroidSans.ttf", &desc);

	AppGraphCtxFramePresent(gAppGraphCtx, true);

	gPointRenderer.Init(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext);
	gMeshRenderer.Init(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext);
	gDebugLineRender.Init(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext);
	gDiffuseRenderer.Init(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext);

	// create a mesh for immediate mode rendering
	gContext.immediateMesh = new GpuMesh(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext);

	gContext.window = window;
	gContext.msaaSamples = msaaSamples;
}

void GetRenderDevice(ID3D11Device** device, ID3D11DeviceContext** context)
{
	*device = gAppGraphCtx->m_device;
	*context = gAppGraphCtx->m_deviceContext;
}

void ReshapeRender(SDL_Window* window)
{
	AppGraphCtxReleaseRenderTarget(gAppGraphCtx);
	AppGraphCtxInitRenderTarget(gAppGraphCtx, window, false, gContext.msaaSamples);
}

void DestroyRender()
{
	gDebugLineRender.Destroy();
	gMeshRenderer.Destroy();
	gPointRenderer.Destroy();
	gDiffuseRenderer.Destroy();

	imguiGraphDestroy();

	delete gContext.immediateMesh;

	// do this first, since it flushes all GPU work
	AppGraphCtxReleaseRenderTarget(gAppGraphCtx);
	AppGraphCtxRelease(gAppGraphCtx);

	gAppGraphCtx = nullptr;
}

void StartFrame(Vec4 clear)
{
	AppGraphCtxFrameStart(gAppGraphCtx, clear);

	MeshDrawParams meshDrawParams;
	memset(&meshDrawParams, 0, sizeof(MeshDrawParams));
	meshDrawParams.renderStage = MESH_DRAW_LIGHT;
	meshDrawParams.renderMode = MESH_RENDER_SOLID;
	meshDrawParams.cullMode = MESH_CULL_BACK;
	meshDrawParams.projection = (XMMATRIX)Matrix44::kIdentity;
	meshDrawParams.view = (XMMATRIX)Matrix44::kIdentity;
	meshDrawParams.model = DirectX::XMMatrixMultiply(
		DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f),
		DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f)
	);

	gContext.mMeshDrawParams = meshDrawParams;
}

void FlushFrame()
{
	gAppGraphCtx->m_deviceContext->Flush();

}

void EndFrame()
{
	FlushFrame();

	ImguiGraphDesc desc;
	desc.device = gAppGraphCtx->m_device;
	desc.deviceContext = gAppGraphCtx->m_deviceContext;
	desc.winW = gAppGraphCtx->m_winW;
	desc.winH = gAppGraphCtx->m_winH;
	
	imguiGraphUpdate(&desc);
}

void PresentFrame(bool fullsync)
{
	AppGraphCtxFramePresent(gAppGraphCtx, fullsync);

}

void ReadFrame(int* backbuffer, int width, int height)
{
	assert(0);
}

void GetViewRay(int x, int y, Vec3& origin, Vec3& dir)
{
	using namespace DirectX;

	XMVECTOR nearVector = XMVector3Unproject(XMVectorSet(float(x), float(gAppGraphCtx->m_winH-y), 0.0f, 0.0f), 0.0f, 0.0f, (float)gAppGraphCtx->m_winW, (float)gAppGraphCtx->m_winH, 0.0f, 1.0f, (XMMATRIX)gContext.proj, XMMatrixIdentity(), (XMMATRIX)gContext.view);
	XMVECTOR farVector = XMVector3Unproject(XMVectorSet(float(x), float(gAppGraphCtx->m_winH-y), 1.0f, 0.0f), 0.0f, 0.0f, (float)gAppGraphCtx->m_winW, (float)gAppGraphCtx->m_winH, 0.0f, 1.0f, (XMMATRIX)gContext.proj, XMMatrixIdentity(), (XMMATRIX)gContext.view);

	origin = Vec3(XMVectorGetX(nearVector), XMVectorGetY(nearVector), XMVectorGetZ(nearVector));
	XMVECTOR tmp = farVector - nearVector;
	dir = Normalize(Vec3(XMVectorGetX(tmp), XMVectorGetY(tmp), XMVectorGetZ(tmp)));

}

Colour gColors[] =
{
	Colour(0.0f, 0.5f, 1.0f),
	Colour(0.797f, 0.354f, 0.000f),
	Colour(0.092f, 0.465f, 0.820f),
	Colour(0.000f, 0.349f, 0.173f),
	Colour(0.875f, 0.782f, 0.051f),
	Colour(0.000f, 0.170f, 0.453f),
	Colour(0.673f, 0.111f, 0.000f),
	Colour(0.612f, 0.194f, 0.394f)
};


void SetFillMode(bool wire)
{
	gContext.mMeshDrawParams.renderMode = wire?MESH_RENDER_WIREFRAME:MESH_RENDER_SOLID;
}

void SetCullMode(bool enabled)
{
	gContext.mMeshDrawParams.cullMode = enabled?MESH_CULL_BACK:MESH_CULL_NONE;
}

void SetView(Matrix44 view, Matrix44 projection)
{
	Matrix44 vp = projection*view;

	gContext.mMeshDrawParams.model = (XMMATRIX)Matrix44::kIdentity;
	gContext.mMeshDrawParams.view = (XMMATRIX)view;
	gContext.mMeshDrawParams.projection = (XMMATRIX)(ConvertToD3DProjection(projection));

	gContext.view = view;
	gContext.proj = ConvertToD3DProjection(projection);
}



FluidRenderer* CreateFluidRenderer(uint32_t width, uint32_t height)
{
	FluidRenderer* renderer = new(_aligned_malloc(sizeof(FluidRenderer), 16)) FluidRenderer();
	renderer->Init(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext, width, height);

	return renderer;
}

void DestroyFluidRenderer(FluidRenderer* renderer)
{
	renderer->Destroy();
	renderer->~FluidRenderer();

	_aligned_free(renderer);
}

FluidRenderBuffers CreateFluidRenderBuffers(int numParticles, bool enableInterop)
{
	FluidRenderBuffers buffers = {};
	buffers.mNumFluidParticles = numParticles;

	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = numParticles*sizeof(Vec4);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.StructureByteStride = 0;

		gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mPositionVBO);
	
		gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mAnisotropyVBO[0]);
		gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mAnisotropyVBO[1]);
		gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mAnisotropyVBO[2]);

		bufDesc.ByteWidth = numParticles*sizeof(float);
		gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mDensityVBO);
	}

	{
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.ByteWidth = numParticles * sizeof(int);
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.StructureByteStride = 0;

		gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mIndices);
	}

	if (enableInterop)
	{
		extern NvFlexLibrary* g_flexLib;

		buffers.mPositionBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mPositionVBO, numParticles, sizeof(Vec4));
		buffers.mDensitiesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mDensityVBO, numParticles, sizeof(float));
		buffers.mIndicesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mIndices, numParticles, sizeof(int));

		buffers.mAnisotropyBuf[0] = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mAnisotropyVBO[0], numParticles, sizeof(Vec4));
		buffers.mAnisotropyBuf[1] = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mAnisotropyVBO[1], numParticles, sizeof(Vec4));
		buffers.mAnisotropyBuf[2] = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mAnisotropyVBO[2], numParticles, sizeof(Vec4));
	}
	
	return buffers;
}

void UpdateFluidRenderBuffers(FluidRenderBuffers buffers, NvFlexSolver* solver, bool anisotropy, bool density)
{
	if (!anisotropy)
	{
		// regular particles
		NvFlexGetParticles(solver, buffers.mPositionBuf, buffers.mNumFluidParticles);
	}
	else
	{
		// fluid buffers
		NvFlexGetSmoothParticles(solver, buffers.mPositionBuf, buffers.mNumFluidParticles);
		NvFlexGetAnisotropy(solver, buffers.mAnisotropyBuf[0], buffers.mAnisotropyBuf[1], buffers.mAnisotropyBuf[2]);
	}

	if (density)
	{
		NvFlexGetDensities(solver, buffers.mDensitiesBuf, buffers.mNumFluidParticles);
	}
	else
	{
		NvFlexGetPhases(solver, buffers.mDensitiesBuf, buffers.mNumFluidParticles);
	}

	NvFlexGetActive(solver, buffers.mIndicesBuf);
}

void UpdateFluidRenderBuffers(FluidRenderBuffers buffers, Vec4* particles, float* densities, Vec4* anisotropy1, Vec4* anisotropy2, Vec4* anisotropy3, int numParticles, int* indices, int numIndices)
{
	D3D11_MAPPED_SUBRESOURCE res;

	// vertices
	if (particles)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mPositionVBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, particles, sizeof(Vec4)*numParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mPositionVBO, 0);
	}

	if (anisotropy1)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mAnisotropyVBO[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, anisotropy1, sizeof(Vec4)*numParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mAnisotropyVBO[0], 0);
	}

	if (anisotropy2)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mAnisotropyVBO[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, anisotropy2, sizeof(Vec4)*numParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mAnisotropyVBO[1], 0);
	}

	if (anisotropy3)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mAnisotropyVBO[2], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, anisotropy3, sizeof(Vec4)*numParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mAnisotropyVBO[2], 0);
	}

	if (densities)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mDensityVBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, densities, sizeof(float)*numParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mDensityVBO, 0);
	}

	// indices
	if (indices)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mIndices, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, indices, sizeof(int)*numIndices);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mIndices, 0);
	}

}

void DestroyFluidRenderBuffers(FluidRenderBuffers buffers)
{
	COMRelease(buffers.mPositionVBO);
	COMRelease(buffers.mAnisotropyVBO[0]);
	COMRelease(buffers.mAnisotropyVBO[1]);
	COMRelease(buffers.mAnisotropyVBO[2]);
	COMRelease(buffers.mDensityVBO);
	COMRelease(buffers.mIndices);

	NvFlexUnregisterD3DBuffer(buffers.mPositionBuf);
	NvFlexUnregisterD3DBuffer(buffers.mDensitiesBuf);
	NvFlexUnregisterD3DBuffer(buffers.mIndicesBuf);

	NvFlexUnregisterD3DBuffer(buffers.mAnisotropyBuf[0]);
	NvFlexUnregisterD3DBuffer(buffers.mAnisotropyBuf[1]);
	NvFlexUnregisterD3DBuffer(buffers.mAnisotropyBuf[2]);
}


static const int kShadowResolution = 2048;


ShadowMap* ShadowCreate()
{
	ShadowMap* shadowMap = new(_aligned_malloc(sizeof(ShadowMap), 16)) ShadowMap();
	shadowMap->init(gAppGraphCtx->m_device, kShadowResolution);

	return shadowMap;
}

void ShadowDestroy(ShadowMap* map)
{
	map->~ShadowMap();
	_aligned_free(map);
}

void ShadowBegin(ShadowMap* map)
{
	ShadowMap* shadowMap = map;
	shadowMap->bindAndClear(gAppGraphCtx->m_deviceContext);

	gContext.mMeshDrawParams.renderStage = MESH_DRAW_SHADOW;
}

void ShadowEnd()
{
	AppGraphCtx* context = gAppGraphCtx;

	// reset to main frame buffer
	context->m_deviceContext->RSSetViewports(1, &context->m_viewport);
	context->m_deviceContext->OMSetRenderTargets(1, &context->m_rtv, context->m_dsv);
	context->m_deviceContext->OMSetDepthStencilState(context->m_depthState, 0u);
	context->m_deviceContext->ClearDepthStencilView(context->m_dsv, D3D11_CLEAR_DEPTH, 1.0, 0);

	gContext.mMeshDrawParams.renderStage = MESH_DRAW_NULL;

}


struct ShadowParams
{
	DirectX::XMMATRIX lightTransform;
	float3 lightPos;
	float3 lightDir;
	float bias;
	float4 shadowTaps[12];
};

void ShadowApply(ShadowParams* params, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, void* shadowTex)
{
	params->lightTransform = (DirectX::XMMATRIX&)(ConvertToD3DProjection(lightTransform));
	params->lightPos = (float3&)lightPos;
	params->lightDir = (float3&)Normalize(lightTarget - lightPos);
	params->bias = gShadowBias;

	const Vec4 taps[] =
	{
		Vec2(-0.326212f,-0.40581f), Vec2(-0.840144f,-0.07358f),
		Vec2(-0.695914f,0.457137f), Vec2(-0.203345f,0.620716f),
		Vec2(0.96234f,-0.194983f), Vec2(0.473434f,-0.480026f),
		Vec2(0.519456f,0.767022f), Vec2(0.185461f,-0.893124f),
		Vec2(0.507431f,0.064425f), Vec2(0.89642f,0.412458f),
		Vec2(-0.32194f,-0.932615f), Vec2(-0.791559f,-0.59771f)
	};
	memcpy(params->shadowTaps, taps, sizeof(taps));

}

void BindSolidShader(Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float bias, Vec4 fogColor)
{
	gContext.mMeshDrawParams.renderStage = MESH_DRAW_LIGHT;

	gContext.mMeshDrawParams.grid = 0;
	gContext.mMeshDrawParams.spotMin = gSpotMin;
	gContext.mMeshDrawParams.spotMax = gSpotMax;
	gContext.mMeshDrawParams.fogColor = (float4&)fogColor;

	gContext.mMeshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;

	ShadowParams shadow;
	ShadowApply(&shadow, lightPos, lightTarget, lightTransform, shadowMap);
	gContext.mMeshDrawParams.lightTransform = shadow.lightTransform;
	gContext.mMeshDrawParams.lightDir = shadow.lightDir;
	gContext.mMeshDrawParams.lightPos = shadow.lightPos;
	gContext.mMeshDrawParams.bias = shadow.bias;
	memcpy(gContext.mMeshDrawParams.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));

	gContext.shadowMap = shadowMap;

}

void UnbindSolidShader()
{
	gContext.mMeshDrawParams.renderStage = MESH_DRAW_NULL;	
}

void DrawMesh(const Mesh* m, Vec3 color)
{
	if (m)
	{
		if (m->m_colours.size())
		{
			gContext.mMeshDrawParams.colorArray = 1;
			gContext.immediateMesh->UpdateData((Vec3*)&m->m_positions[0], &m->m_normals[0], NULL, (Vec4*)&m->m_colours[0], (int*)&m->m_indices[0], m->GetNumVertices(), int(m->GetNumFaces()));
		}
		else
		{
			gContext.mMeshDrawParams.colorArray = 0;
			gContext.immediateMesh->UpdateData((Vec3*)&m->m_positions[0], &m->m_normals[0], NULL, NULL, (int*)&m->m_indices[0], m->GetNumVertices(), int(m->GetNumFaces()));
		}

		gContext.mMeshDrawParams.color = (float4&)color;
		gContext.mMeshDrawParams.secondaryColor = (float4&)color;
		gContext.mMeshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;
		gContext.mMeshDrawParams.shadowMap = gContext.shadowMap;

		gMeshRenderer.Draw(gContext.immediateMesh, &gContext.mMeshDrawParams);

		if (m->m_colours.size())
			gContext.mMeshDrawParams.colorArray = 0;

	}
}

void DrawCloth(const Vec4* positions, const Vec4* normals, const float* uvs, const int* indices, int numTris, int numPositions, int colorIndex, float expand, bool twosided, bool smooth)
{
	if (!numTris)
		return;

	gContext.immediateMesh->UpdateData(positions, normals, NULL, NULL, indices, numPositions, numTris);
	
	if (twosided)
		SetCullMode(false);

	gContext.mMeshDrawParams.bias = 0.0f;
	gContext.mMeshDrawParams.expand = expand;

	gContext.mMeshDrawParams.color = (float4&)(gColors[colorIndex + 1] * 1.5f);
	gContext.mMeshDrawParams.secondaryColor = (float4&)(gColors[colorIndex] * 1.5f);
	gContext.mMeshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;
	gContext.mMeshDrawParams.shadowMap = gContext.shadowMap;

	gMeshRenderer.Draw(gContext.immediateMesh, &gContext.mMeshDrawParams);

	if (twosided)
		SetCullMode(true);

	gContext.mMeshDrawParams.bias = gShadowBias;
	gContext.mMeshDrawParams.expand = 0.0f;
	
}

void DrawRope(Vec4* positions, int* indices, int numIndices, float radius, int color)
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

	gContext.immediateMesh->UpdateData(&vertices[0], &normals[0], NULL, NULL, &triangles[0], int(vertices.size()), int(triangles.size())/3);

	SetCullMode(false);

	gContext.mMeshDrawParams.color = (float4&)(gColors[color % 8] * 1.5f);
	gContext.mMeshDrawParams.secondaryColor = (float4&)(gColors[color % 8] * 1.5f);
	gContext.mMeshDrawParams.objectTransform = (float4x4&)Matrix44::kIdentity;
	gContext.mMeshDrawParams.shadowMap = gContext.shadowMap;

	gMeshRenderer.Draw(gContext.immediateMesh, &gContext.mMeshDrawParams);

	SetCullMode(true);
}

void DrawPlane(const Vec4& p, bool color)
{
	std::vector<Vec3> vertices;
	std::vector<Vec3> normals;
	std::vector<int> indices;
	
	Vec3 u, v;
	BasisFromVector(Vec3(p.x, p.y, p.z), &u, &v);

	Vec3 c = Vec3(p.x, p.y, p.z)*-p.w;

	gContext.mMeshDrawParams.shadowMap = gContext.shadowMap;

	if (color)
		gContext.mMeshDrawParams.color = (float4&)(p * 0.5f + Vec4(0.5f, 0.5f, 0.5f, 0.5f));

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

	gContext.immediateMesh->UpdateData(&vertices[0], &normals[0], NULL, NULL, &indices[0], int(vertices.size()), int(indices.size())/3);
	gMeshRenderer.Draw(gContext.immediateMesh, &gContext.mMeshDrawParams);
}

void DrawPlanes(Vec4* planes, int n, float bias)
{
	gContext.mMeshDrawParams.color = (float4&)Vec4(0.9f, 0.9f, 0.9f, 1.0f);

	gContext.mMeshDrawParams.bias = 0.0f;
	gContext.mMeshDrawParams.grid = 1;
	gContext.mMeshDrawParams.expand = 0;

	for (int i = 0; i < n; ++i)
	{
		Vec4 p = planes[i];
		p.w -= bias;

		DrawPlane(p, false);
	}

	gContext.mMeshDrawParams.grid = 0;
	gContext.mMeshDrawParams.bias = gShadowBias;
		
}

GpuMesh* CreateGpuMesh(const Mesh* m)
{
	GpuMesh* mesh = new GpuMesh(gAppGraphCtx->m_device, gAppGraphCtx->m_deviceContext);

	mesh->UpdateData((Vec3*)&m->m_positions[0], &m->m_normals[0], NULL, NULL, (int*)&m->m_indices[0], m->GetNumVertices(), int(m->GetNumFaces()));

	return mesh;
}

void DestroyGpuMesh(GpuMesh* m)
{
	delete m;
}

void DrawGpuMesh(GpuMesh* m, const Matrix44& xform, const Vec3& color)
{
	if (m)
	{
		MeshDrawParams params = gContext.mMeshDrawParams;

		params.color = (float4&)color;
		params.secondaryColor = (float4&)color;
		params.objectTransform = (float4x4&)xform;
		params.shadowMap = gContext.shadowMap;

		gMeshRenderer.Draw(m, &params);
	}
}

void DrawGpuMeshInstances(GpuMesh* m, const Matrix44* xforms, int n, const Vec3& color)
{
	if (m)
	{
		gContext.mMeshDrawParams.color = (float4&)color;
		gContext.mMeshDrawParams.secondaryColor = (float4&)color;
		gContext.mMeshDrawParams.shadowMap = gContext.shadowMap;

		// copy params
		MeshDrawParams params = gContext.mMeshDrawParams;

		for (int i = 0; i < n; ++i)
		{
			params.objectTransform = (float4x4&)xforms[i];

			gMeshRenderer.Draw(m, &params);
		}
	}
}

void DrawPoints(VertexBuffer positions, VertexBuffer colors, IndexBuffer indices, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowTex, bool showDensity)
{
	if (n == 0)
		return;

	PointDrawParams params;

	params.renderMode = POINT_RENDER_SOLID;
	params.cullMode = POINT_CULL_BACK;
	params.model = (XMMATRIX&)Matrix44::kIdentity;
	params.view = (XMMATRIX&)gContext.view;
	params.projection = (XMMATRIX&)gContext.proj;

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
		params.colors[i] = *((float4*)&gColors[i].r);

	// set shadow parameters
	ShadowParams shadow;
	ShadowApply(&shadow, lightPos, lightTarget, lightTransform, shadowTex);
	params.lightTransform = shadow.lightTransform;
	params.lightDir = shadow.lightDir;
	params.lightPos = shadow.lightPos;
	memcpy(params.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));

	if (gContext.mMeshDrawParams.renderStage == MESH_DRAW_SHADOW)
	{
		params.renderStage = POINT_DRAW_SHADOW;
		params.mode = 2;
	}
	else
		params.renderStage = POINT_DRAW_LIGHT;
	
	params.shadowMap = gContext.shadowMap;

	gPointRenderer.Draw(&params, positions, colors, indices, n, offset);

}

void RenderEllipsoids(FluidRenderer* renderer, FluidRenderBuffers buffers, int n, int offset, float radius, float screenWidth, float screenAspect, float fov, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, Vec4 color, float blur, float ior, bool debug)
{
	if (n == 0)
		return;

	FluidDrawParams params;

	params.renderMode = FLUID_RENDER_SOLID;
	params.cullMode = FLUID_CULL_BACK;
	params.model = (XMMATRIX&)Matrix44::kIdentity;
	params.view = (XMMATRIX&)gContext.view;
	params.projection = (XMMATRIX&)gContext.proj;

	params.offset = offset;
	params.n = n;
	params.renderStage = FLUID_DRAW_LIGHT;

	const float viewHeight = tanf(fov / 2.0f);
	params.invViewport = float3(1.0f / screenWidth, screenAspect / screenWidth, 1.0f);
	params.invProjection = float3(screenAspect * viewHeight, viewHeight, 1.0f);

	params.shadowMap = gContext.shadowMap;

	renderer->mDepthTex.BindAndClear(gAppGraphCtx->m_deviceContext);

	// draw static shapes into depth buffer
	//DrawShapes();

	renderer->DrawEllipsoids(&params, &buffers);


	//---------------------------------------------------------------
	// build smooth depth

	renderer->mDepthSmoothTex.BindAndClear(gAppGraphCtx->m_deviceContext);

	params.blurRadiusWorld = radius * 0.5f;
	params.blurScale = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.invTexScale = float4(1.0f / screenAspect, 1.0f, 0.0f, 0.0f);
	params.blurFalloff = blur;
	params.debug = debug;

	renderer->DrawBlurDepth(&params);
	
	//---------------------------------------------------------------
	// composite

	AppGraphCtx* context = gAppGraphCtx;

	{
		context->m_deviceContext->RSSetViewports(1, &context->m_viewport);
		context->m_deviceContext->RSSetScissorRects(0, nullptr);

		context->m_deviceContext->OMSetRenderTargets(1, &context->m_rtv, context->m_dsv);
		context->m_deviceContext->OMSetDepthStencilState(context->m_depthState, 0u);

		float blendFactor[4] = { 1.0, 1.0, 1.0, 1.0 };
		context->m_deviceContext->OMSetBlendState(context->m_blendState, blendFactor, 0xffff);
	}

	params.invTexScale = (float4&)Vec2(1.0f / screenWidth, screenAspect / screenWidth);
	params.clipPosToEye = (float4&)Vec2(tanf(fov*0.5f)*screenAspect, tanf(fov*0.5f));
	params.color = (float4&)color;
	params.ior = ior;
	params.spotMin = gSpotMin;
	params.spotMax = gSpotMax;
	params.debug = debug;

	params.lightPos = (float3&)lightPos;
	params.lightDir = (float3&)-Normalize(lightTarget - lightPos);
	params.lightTransform = (XMMATRIX&)(ConvertToD3DProjection(lightTransform));
		
	
	AppGraphCtxResolveFrame(context);	


	renderer->DrawComposite(&params, context->m_resolvedTargetSRV);

	{
		AppGraphCtx* context = gAppGraphCtx;
		context->m_deviceContext->OMSetBlendState(nullptr, 0, 0xffff);
	}
}

DiffuseRenderBuffers CreateDiffuseRenderBuffers(int numParticles, bool& enableInterop)
{
	DiffuseRenderBuffers buffers = {};
	buffers.mNumDiffuseParticles = numParticles;
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

			gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mDiffusePositionVBO);
			gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mDiffuseVelocityVBO);
		}

		{
			D3D11_BUFFER_DESC bufDesc;
			bufDesc.ByteWidth = numParticles * sizeof(int);
			bufDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufDesc.MiscFlags = 0;
			if (enableInterop)
			{
				bufDesc.CPUAccessFlags = 0;
				bufDesc.Usage = D3D11_USAGE_DEFAULT;
				bufDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
			}

			gAppGraphCtx->m_device->CreateBuffer(&bufDesc, NULL, &buffers.mDiffuseIndicesIBO);
		}

		if (enableInterop)
		{
			extern NvFlexLibrary* g_flexLib;

			buffers.mDiffuseIndicesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mDiffuseIndicesIBO, numParticles, sizeof(int));
			buffers.mDiffusePositionsBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mDiffusePositionVBO, numParticles, sizeof(Vec4));
			buffers.mDiffuseVelocitiesBuf = NvFlexRegisterD3DBuffer(g_flexLib, buffers.mDiffuseVelocityVBO, numParticles, sizeof(Vec4));

			if (buffers.mDiffuseIndicesBuf		== nullptr ||
				buffers.mDiffusePositionsBuf	== nullptr ||
				buffers.mDiffuseVelocitiesBuf	== nullptr)
				enableInterop = false;
		}
	}

	return buffers;
}

void DestroyDiffuseRenderBuffers(DiffuseRenderBuffers buffers)
{
	if (buffers.mNumDiffuseParticles > 0)
	{
		COMRelease(buffers.mDiffusePositionVBO);
		COMRelease(buffers.mDiffuseVelocityVBO);
		COMRelease(buffers.mDiffuseIndicesIBO);

		NvFlexUnregisterD3DBuffer(buffers.mDiffuseIndicesBuf);
		NvFlexUnregisterD3DBuffer(buffers.mDiffusePositionsBuf);
		NvFlexUnregisterD3DBuffer(buffers.mDiffuseVelocitiesBuf);
	}
}

void UpdateDiffuseRenderBuffers(DiffuseRenderBuffers buffers, NvFlexSolver* solver)
{
	// diffuse particles
	if (buffers.mNumDiffuseParticles)
	{
		NvFlexGetDiffuseParticles(solver, buffers.mDiffusePositionsBuf, buffers.mDiffuseVelocitiesBuf, buffers.mDiffuseIndicesBuf);
	}
}

void UpdateDiffuseRenderBuffers(DiffuseRenderBuffers buffers, Vec4* diffusePositions, Vec4* diffuseVelocities, int* diffuseIndices, int numDiffuseParticles) 
{
	D3D11_MAPPED_SUBRESOURCE res;

	// vertices
	if (diffusePositions)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mDiffusePositionVBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, diffusePositions, sizeof(Vec4)*numDiffuseParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mDiffusePositionVBO, 0);
	}

	if (diffuseVelocities)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mDiffuseVelocityVBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, diffuseVelocities, sizeof(Vec4)*numDiffuseParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mDiffuseVelocityVBO, 0);
	}

	if (diffuseIndices)
	{
		gAppGraphCtx->m_deviceContext->Map(buffers.mDiffuseIndicesIBO, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		memcpy(res.pData, diffuseIndices, sizeof(int)*numDiffuseParticles);
		gAppGraphCtx->m_deviceContext->Unmap(buffers.mDiffuseIndicesIBO, 0);
	}

}

void RenderDiffuse(FluidRenderer* render, DiffuseRenderBuffers buffers, int n, float radius, float screenWidth, float screenAspect, float fov, Vec4 color, Vec3 lightPos, Vec3 lightTarget, Matrix44 lightTransform, ShadowMap* shadowMap, float motionBlur, float inscatter, float outscatter, bool shadowEnabled, bool front)
{
	if (n == 0)
		return;

	DiffuseDrawParams params;

	params.model = (const XMMATRIX&)Matrix44::kIdentity;
	params.view = (const XMMATRIX&)gContext.view;
	params.projection = (const XMMATRIX&)gContext.proj;
	params.diffuseRadius = screenWidth / screenAspect * (1.0f / (tanf(fov * 0.5f)));
	params.diffuseScale = radius;
	params.spotMin = gSpotMin;
	params.spotMax = gSpotMax;
	params.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	params.motionScale = motionBlur;

	// set shadow parameters
	ShadowParams shadow;
	ShadowApply(&shadow, lightPos, lightTarget, lightTransform, shadowMap);
	params.lightTransform = shadow.lightTransform;
	params.lightDir = shadow.lightDir;
	params.lightPos = shadow.lightPos;
	params.shadowMap = gContext.shadowMap;

	memcpy(params.shadowTaps, shadow.shadowTaps, sizeof(shadow.shadowTaps));
	

	gDiffuseRenderer.Draw(&params, buffers.mDiffusePositionVBO, buffers.mDiffuseVelocityVBO, buffers.mDiffuseIndicesIBO, n);

	// reset depth stencil state
	gAppGraphCtx->m_deviceContext->OMSetDepthStencilState(gAppGraphCtx->m_depthState, 0u);


}


void BeginLines() 
{
	
}

void DrawLine(const Vec3& p, const Vec3& q, const Vec4& color) 
{
	gDebugLineRender.AddLine(p, q, color);
}

void EndLines() 
{
	// draw
	Matrix44 projectionViewWorld = ((Matrix44&)(gContext.mMeshDrawParams.projection))*((Matrix44&)(gContext.mMeshDrawParams.view));

	gDebugLineRender.FlushLines(projectionViewWorld);
}

void BeginPoints(float size) {}
void DrawPoint(const Vec3& p, const Vec4& color) {}
void EndPoints() {} 

