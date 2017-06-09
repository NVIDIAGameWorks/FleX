#ifndef MESH_RENDERER_D3D12_H
#define MESH_RENDERER_D3D12_H

#include "bufferD3D12.h"

#include "renderStateD3D12.h"
#include "meshRenderer.h"

// Predeclare so all use the same
struct ShadowMap;

namespace FlexSample {
using namespace nvidia;

struct MeshRendererD3D12;
struct RenderMeshD3D12: public RenderMesh
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS(RenderMeshD3D12, RenderMesh);
public:
	typedef RenderMesh Parent;

	friend struct MeshRendererD3D12;

	RenderMeshD3D12();
	int initialize(const RenderStateD3D12& state, const MeshData& meshData);
	int initialize(const RenderStateD3D12& state, const MeshData2& meshData);
	
	protected:
	void _setBufferNames();

	VertexBufferD3D12 m_positionBuffer;
	VertexBufferD3D12 m_normalBuffer;
	VertexBufferD3D12 m_texcoordBuffer;
	VertexBufferD3D12 m_colorBuffer;
	IndexBufferD3D12 m_indexBuffer;

	ptrdiff_t m_numVertices;
	ptrdiff_t m_numFaces;
};

struct PointRenderAllocationD3D12: public RenderAllocation
{
	typedef RenderAllocation Parent;
	enum VertexViewType
	{
		// NOTE! Do not change order without fixing pipelines that use it!
		VERTEX_VIEW_POSITION,			
		VERTEX_VIEW_DENSITY,
		VERTEX_VIEW_PHASE,

		VERTEX_VIEW_ANISOTROPY1,
		VERTEX_VIEW_ANISOTROPY2,
		VERTEX_VIEW_ANISOTROPY3,

		VERTEX_VIEW_COUNT_OF,
	};
	enum 
	{	
		// For typical point rendering we don't need anisotropy, so typically just bind the first 3
		NUM_DEFAULT_VERTEX_VIEWS = VERTEX_VIEW_PHASE + 1
	};

		/// Initialize state
	void init(PrimitiveType primitiveType)
	{
		Parent::init(primitiveType);
		D3D12_VERTEX_BUFFER_VIEW nullView = {};
		for (int i = 0; i < _countof(m_vertexBufferViews); i++)
		{
			m_vertexBufferViews[i] = nullView;
		}
		D3D12_INDEX_BUFFER_VIEW nullIndexView = {};
		m_indexBufferView = nullIndexView;
	}

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[VERTEX_VIEW_COUNT_OF];
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

struct MeshRenderAllocationD3D12:public RenderAllocation
{
	typedef RenderAllocation Parent;

	// Vertex buffer viewer are in the order to be set 
	enum VertexViewType
	{
		// NOTE! Do not change order without fixing pipelines that use it!
		VERTEX_VIEW_POSITION,
		VERTEX_VIEW_NORMAL,
		VERTEX_VIEW_TEX_COORDS,
		VERTEX_VIEW_COLOR,

		VERTEX_VIEW_COUNT_OF,
	};

	void init(PrimitiveType primType)
	{
		Parent::init(primType);
		D3D12_VERTEX_BUFFER_VIEW nullView = {};
		for (int i = 0; i < _countof(m_vertexBufferViews); i++)
		{
			m_vertexBufferViews[i] = nullView;
		}
		D3D12_INDEX_BUFFER_VIEW nullIndexView = {};
		m_indexBufferView = nullIndexView;
	}

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[VERTEX_VIEW_COUNT_OF];
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

struct LineRenderAllocationD3D12 :public RenderAllocation
{
	typedef RenderAllocation Parent;

	void init(PrimitiveType primType)
	{
		Parent::init(primType);
		const D3D12_VERTEX_BUFFER_VIEW nullView = {};
		m_vertexBufferView = nullView;
		const D3D12_INDEX_BUFFER_VIEW nullIndexView = {};
		m_indexBufferView = nullIndexView;
	}

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

struct MeshRendererD3D12: public MeshRenderer
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS(MeshRendererD3D12, MeshRenderer);
public:
	typedef MeshRenderer Parent;

		/// MeshRenderer
	virtual int draw(RenderMesh* mesh, RenderPipeline* pipeline, const void* params) override;
	virtual int drawImmediate(const MeshData& mesh, RenderPipeline* pipeline, const void* params) override;
	virtual int drawImmediate(const MeshData2& meshData, RenderPipeline* pipeline, const void* params) override;
	virtual int drawImmediate(const PointData& pointData, RenderPipeline* pipeline, const void* params) override;
	virtual int drawImmediate(const LineData& lineData, RenderPipeline* pipeline, const void* params) override;

	virtual RenderMesh* createMesh(const MeshData& meshData) override;
	virtual RenderMesh* createMesh(const MeshData2& meshData) override;

	virtual int allocateTransitory(const PointData& pointData, RenderAllocation& allocation, size_t sizeOfAlloc) override;
	virtual int allocateTransitory(const MeshData& meshData, RenderAllocation& allocIn, size_t sizeofAlloc) override;
	virtual int allocateTransitory(const MeshData2& meshData, RenderAllocation& allocation, size_t sizeofAlloc) override;
	virtual int allocateTransitory(const LineData& lineData, RenderAllocation& allocation, size_t sizeOfAlloc) override;

	virtual int drawTransitory(RenderAllocation& allocation, size_t sizeOfAlloc, RenderPipeline* pipeline, const void* params) override;

	int initialize(const RenderStateD3D12& state);

	MeshRendererD3D12();

		/// Default drawing impls, can be used in pipelines
	static int defaultDraw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState);

		/// The input desc layout used
	static const D3D12_INPUT_ELEMENT_DESC MeshInputElementDescs[4];
		/// Layout for points
	static const D3D12_INPUT_ELEMENT_DESC PointInputElementDescs[3];

	protected:

	D3D12_VERTEX_BUFFER_VIEW _newImmediateVertexBuffer(const void* data, int stride, ptrdiff_t numVertices);
	D3D12_VERTEX_BUFFER_VIEW _newStridedImmediateVertexBuffer(const void* data, int srcStride, int dstStride, ptrdiff_t numElements);
	D3D12_INDEX_BUFFER_VIEW _newImmediateIndexBuffer(const void* data, int stride, ptrdiff_t numIndices);

	RenderStateD3D12 m_renderState;
};

} // namespace FlexSample

#endif