#include "meshRendererD3D12.h"

namespace FlexSample {

const D3D12_INPUT_ELEMENT_DESC MeshRendererD3D12::MeshInputElementDescs[4] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};

const D3D12_INPUT_ELEMENT_DESC MeshRendererD3D12::PointInputElementDescs[3] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "DENSITY", 0, DXGI_FORMAT_R32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "PHASE", 0, DXGI_FORMAT_R32_SINT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Dx12RenderMesh !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

RenderMeshD3D12::RenderMeshD3D12()
{
	m_numVertices = 0;
	m_numFaces = 0;
}

int RenderMeshD3D12::initialize(const RenderStateD3D12& state, const MeshData& meshData)
{
	NV_RETURN_ON_FAIL(m_positionBuffer.init(state, sizeof(Vec3), meshData.numVertices, meshData.positions));
	NV_RETURN_ON_FAIL(m_normalBuffer.init(state, sizeof(Vec3), meshData.numVertices, meshData.normals));
	NV_RETURN_ON_FAIL(m_texcoordBuffer.init(state, sizeof(Vec2), meshData.numVertices, meshData.texcoords));
	NV_RETURN_ON_FAIL(m_colorBuffer.init(state, sizeof(Vec4), meshData.numVertices, meshData.colors));
	NV_RETURN_ON_FAIL(m_indexBuffer.init(state, sizeof(uint32_t), meshData.numFaces * 3, meshData.indices));

	m_numVertices = meshData.numVertices;
	m_numFaces = meshData.numFaces;

	_setBufferNames();
	return NV_OK;
}

int RenderMeshD3D12::initialize(const RenderStateD3D12& state, const MeshData2& meshData)
{
	NV_RETURN_ON_FAIL(m_positionBuffer.init(state, sizeof(Vec4), sizeof(Vec3), meshData.numVertices, meshData.positions));
	NV_RETURN_ON_FAIL(m_normalBuffer.init(state, sizeof(Vec4), sizeof(Vec3), meshData.numVertices, meshData.normals));
	NV_RETURN_ON_FAIL(m_texcoordBuffer.init(state, sizeof(Vec2), meshData.numVertices, meshData.texcoords));
	NV_RETURN_ON_FAIL(m_colorBuffer.init(state, sizeof(Vec4), meshData.numVertices, meshData.colors));
	NV_RETURN_ON_FAIL(m_indexBuffer.init(state, sizeof(uint32_t), meshData.numFaces * 3, meshData.indices));

	m_numVertices = meshData.numVertices;
	m_numFaces = meshData.numFaces;

	_setBufferNames();
	return NV_OK;
}

void RenderMeshD3D12::_setBufferNames()
{
	m_positionBuffer.setDebugName(L"positions");
	m_normalBuffer.setDebugName(L"normals");
	m_texcoordBuffer.setDebugName(L"texcoords");
	m_colorBuffer.setDebugName(L"colors");
	m_indexBuffer.setDebugName(L"indices");
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Dx12MeshRenderer !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

MeshRendererD3D12::MeshRendererD3D12()
{
}

int MeshRendererD3D12::initialize(const RenderStateD3D12& state)
{
	m_renderState = state;
	return NV_OK;
}

D3D12_VERTEX_BUFFER_VIEW MeshRendererD3D12::_newImmediateVertexBuffer(const void* data, int stride, ptrdiff_t numVertices)
{
	D3D12_VERTEX_BUFFER_VIEW view = {};
	if (data)
	{
		const size_t bufferSize = stride * numVertices;
		NvCo::Dx12CircularResourceHeap::Cursor cursor = m_renderState.m_constantHeap->allocateVertexBuffer(bufferSize);

		memcpy(cursor.m_position, data, bufferSize);

		view.BufferLocation = m_renderState.m_constantHeap->getGpuHandle(cursor);
		view.SizeInBytes = UINT(bufferSize);
		view.StrideInBytes = stride;
	}
	return view;
}

D3D12_VERTEX_BUFFER_VIEW MeshRendererD3D12::_newStridedImmediateVertexBuffer(const void* data, int srcStride, int dstStride, ptrdiff_t numElements)
{
	if (srcStride == dstStride)
	{
		return _newImmediateVertexBuffer(data, srcStride, numElements);
	}

	D3D12_VERTEX_BUFFER_VIEW view = {};
	if (srcStride == 4 * 4 && dstStride == 4 * 3)
	{
		const size_t bufferSize = dstStride * numElements;
		NvCo::Dx12CircularResourceHeap::Cursor cursor = m_renderState.m_constantHeap->allocateVertexBuffer(bufferSize);

		uint32_t* dst = (uint32_t*)cursor.m_position;
		const uint32_t* src = (const uint32_t*)data;

		// Copy taking into account stride difference
		for (ptrdiff_t i = 0; i < numElements; i++, dst += 3, src += 4)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
		}

		view.BufferLocation = m_renderState.m_constantHeap->getGpuHandle(cursor);
		view.SizeInBytes = UINT(bufferSize);
		view.StrideInBytes = dstStride;

		return view;
	}
	
	printf("Unhandled conversion");
	return view;
}

D3D12_INDEX_BUFFER_VIEW MeshRendererD3D12::_newImmediateIndexBuffer(const void* data, int stride, ptrdiff_t numIndices)
{
	assert(stride == sizeof(uint32_t));
	D3D12_INDEX_BUFFER_VIEW view = {};

	if (data)
	{
		const size_t bufferSize = stride * numIndices;
		NvCo::Dx12CircularResourceHeap::Cursor cursor = m_renderState.m_constantHeap->allocateVertexBuffer(bufferSize);

		memcpy(cursor.m_position, data, bufferSize);

		view.BufferLocation = m_renderState.m_constantHeap->getGpuHandle(cursor);
		view.SizeInBytes = UINT(bufferSize);
		view.Format = DXGI_FORMAT_R32_UINT;
	}

	return view;
}

int MeshRendererD3D12::draw(RenderMesh* meshIn, RenderPipeline* pipeline, const void* params)
{
	RenderMeshD3D12* mesh = static_cast<RenderMeshD3D12*>(meshIn);

	// Set up the allocation block
	MeshRenderAllocationD3D12 alloc;
	alloc.init(PRIMITIVE_TRIANGLE);

	alloc.m_vertexBufferViews[0] = mesh->m_positionBuffer.m_vertexBufferView;
	alloc.m_vertexBufferViews[1] = mesh->m_normalBuffer.m_vertexBufferView;
	alloc.m_vertexBufferViews[2] = mesh->m_texcoordBuffer.m_vertexBufferView;
	alloc.m_vertexBufferViews[3] = mesh->m_colorBuffer.m_vertexBufferView;

	alloc.m_indexBufferView = mesh->m_indexBuffer.m_indexBufferView;
	alloc.m_numPrimitives = mesh->m_numFaces;
	alloc.m_numPositions = mesh->m_numVertices;

	return drawTransitory(alloc, sizeof(MeshRenderAllocationD3D12), pipeline, params);
}

int MeshRendererD3D12::drawImmediate(const MeshData& mesh, RenderPipeline* pipeline, const void* params)
{
	MeshRenderAllocationD3D12 alloc;
	allocateTransitory(mesh, alloc, sizeof(alloc));
	return drawTransitory(alloc, sizeof(alloc), pipeline, params);
}

int MeshRendererD3D12::drawImmediate(const MeshData2& mesh, RenderPipeline* pipeline, const void* params)
{
	MeshRenderAllocationD3D12 alloc;
	allocateTransitory(mesh, alloc, sizeof(alloc));
	return drawTransitory(alloc, sizeof(alloc), pipeline, params);
}

int MeshRendererD3D12::drawImmediate(const LineData& lineData, RenderPipeline* pipeline, const void* params)
{
	LineRenderAllocationD3D12 alloc;
	allocateTransitory(lineData, alloc, sizeof(alloc));
	return drawTransitory(alloc, sizeof(alloc), pipeline, params);
}

int MeshRendererD3D12::drawImmediate(const PointData& pointData, RenderPipeline* pipeline, const void* params)
{
	PointRenderAllocationD3D12 alloc;
	allocateTransitory(pointData, alloc, sizeof(alloc));
	return drawTransitory(alloc, sizeof(alloc), pipeline, params);
}

int MeshRendererD3D12::allocateTransitory(const PointData& pointData, RenderAllocation& allocIn, size_t sizeOfAlloc)
{
	typedef PointRenderAllocationD3D12 Alloc;

	assert(sizeof(Alloc) == sizeOfAlloc);
	
	Alloc& alloc = static_cast<Alloc&>(allocIn);
	alloc.init(PRIMITIVE_POINT);

	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION] = _newImmediateVertexBuffer(pointData.positions, sizeof(Vec4), pointData.numPoints);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_DENSITY] = _newImmediateVertexBuffer(pointData.density, sizeof(float), pointData.numPoints);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_PHASE] = _newImmediateVertexBuffer(pointData.phase, sizeof(int), pointData.numPoints);

	if (pointData.anisotropy[0])
	{
		for (int i = 0; i < 3; i++)
		{
			alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_ANISOTROPY1 + i] = _newImmediateVertexBuffer(pointData.anisotropy[i], sizeof(Vec4), pointData.numPoints);
		}
	}

	alloc.m_indexBufferView = _newImmediateIndexBuffer(pointData.indices, sizeof(uint32_t), pointData.numIndices);

	alloc.m_numPrimitives = pointData.numIndices;
	alloc.m_numPositions = pointData.numPoints;
	return NV_OK;
}

int MeshRendererD3D12::allocateTransitory(const MeshData2& mesh, RenderAllocation& allocIn, size_t sizeOfAlloc)
{
	typedef MeshRenderAllocationD3D12 Alloc;
	assert(sizeof(Alloc) == sizeOfAlloc);

	Alloc& alloc = static_cast<Alloc&>(allocIn);
	alloc.init(PRIMITIVE_TRIANGLE);

	const int numIndices = int(mesh.numFaces * 3);
	alloc.m_indexBufferView = _newImmediateIndexBuffer(mesh.indices, sizeof(uint32_t), numIndices);

	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION] = _newStridedImmediateVertexBuffer(mesh.positions, sizeof(Vec4), sizeof(Vec3), mesh.numVertices);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_NORMAL] = _newStridedImmediateVertexBuffer(mesh.normals, sizeof(Vec4), sizeof(Vec3), mesh.numVertices);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_TEX_COORDS] = _newImmediateVertexBuffer(mesh.texcoords, sizeof(Vec2), mesh.numVertices);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_COLOR] = _newImmediateVertexBuffer(mesh.colors, sizeof(Vec4), mesh.numVertices);
	
	alloc.m_numPrimitives = mesh.numFaces;
	alloc.m_numPositions = mesh.numVertices;
	return NV_OK;
}

int MeshRendererD3D12::allocateTransitory(const LineData& lineData, RenderAllocation& allocIn, size_t sizeOfAlloc)
{
	typedef LineRenderAllocationD3D12 Alloc;
	assert(sizeof(Alloc) == sizeOfAlloc);

	Alloc& alloc = static_cast<Alloc&>(allocIn);
	alloc.init(PRIMITIVE_LINE);

	alloc.m_vertexBufferView = _newImmediateVertexBuffer(lineData.vertices, sizeof(LineData::Vertex), lineData.numVertices);
	alloc.m_indexBufferView = _newImmediateIndexBuffer(lineData.indices, sizeof(uint32_t), lineData.numLines * 2);
	
	alloc.m_numPrimitives = lineData.numLines;
	alloc.m_numPositions = lineData.numVertices;
	return NV_OK;
}

int MeshRendererD3D12::allocateTransitory(const MeshData& mesh, RenderAllocation& allocIn, size_t sizeOfAlloc)
{
	typedef MeshRenderAllocationD3D12 Alloc;
	assert(sizeof(Alloc) == sizeOfAlloc);

	Alloc& alloc = static_cast<Alloc&>(allocIn);

	alloc.init(PRIMITIVE_TRIANGLE);
	
	const int numIndices = int(mesh.numFaces * 3);
	alloc.m_indexBufferView = _newImmediateIndexBuffer(mesh.indices, sizeof(uint32_t), numIndices);

	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_POSITION] = _newImmediateVertexBuffer(mesh.positions, sizeof(Vec3), mesh.numVertices);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_NORMAL] = _newImmediateVertexBuffer(mesh.normals, sizeof(Vec3), mesh.numVertices);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_TEX_COORDS] = _newImmediateVertexBuffer(mesh.texcoords, sizeof(Vec2), mesh.numVertices);
	alloc.m_vertexBufferViews[Alloc::VERTEX_VIEW_COLOR] = _newImmediateVertexBuffer(mesh.colors, sizeof(Vec4), mesh.numVertices);

	alloc.m_numPrimitives = mesh.numFaces;
	alloc.m_numPositions = mesh.numVertices;

	return NV_OK;
}

int MeshRendererD3D12::drawTransitory(RenderAllocation& allocIn, size_t sizeOfAlloc, RenderPipeline* pipeline, const void* params)
{
	if (allocIn.m_primitiveType == PRIMITIVE_UNKNOWN)
	{
		return NV_OK;
	}
	if (allocIn.m_primitiveType != pipeline->getPrimitiveType())
	{
		printf("Wrong pipeline primitive type");
		return NV_FAIL;
	}

	NV_RETURN_ON_FAIL(pipeline->bind(params, &m_renderState));
	NV_RETURN_ON_FAIL(pipeline->draw(allocIn, sizeOfAlloc, &m_renderState));
	return NV_OK;
}

RenderMesh* MeshRendererD3D12::createMesh(const MeshData& meshData)
{
	RenderMeshD3D12* mesh = new RenderMeshD3D12;
	if (NV_FAILED(mesh->initialize(m_renderState, meshData)))
	{
		delete mesh;
		return nullptr;
	}
	return mesh;
}

RenderMesh* MeshRendererD3D12::createMesh(const MeshData2& meshData)
{
	RenderMeshD3D12* mesh = new RenderMeshD3D12;
	if (NV_FAILED(mesh->initialize(m_renderState, meshData)))
	{
		delete mesh;
		return nullptr;
	}
	return mesh;
}

int MeshRendererD3D12::defaultDraw(const RenderAllocation& allocIn, size_t sizeOfAlloc, const void* platformState)
{
	const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;
	ID3D12GraphicsCommandList* commandList = state.m_commandList;

	switch (allocIn.m_primitiveType)
	{
		case PRIMITIVE_POINT:
		{
			typedef PointRenderAllocationD3D12 Alloc;
			const RenderStateD3D12& state = *(RenderStateD3D12*)platformState;

			assert(sizeof(Alloc) == sizeOfAlloc);
			const Alloc& alloc = static_cast<const Alloc&>(allocIn);

			assert(allocIn.m_numPrimitives >= 0);

			ID3D12GraphicsCommandList* commandList = state.m_commandList;

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			commandList->IASetVertexBuffers(0, Alloc::NUM_DEFAULT_VERTEX_VIEWS, alloc.m_vertexBufferViews);
			commandList->IASetIndexBuffer(&alloc.m_indexBufferView);

			if (alloc.m_indexBufferView.SizeInBytes)
			{
				commandList->DrawIndexedInstanced((UINT)allocIn.m_numPrimitives, 1, (UINT)allocIn.m_offset, 0, 0);
			}
			else
			{
				commandList->DrawInstanced((UINT)allocIn.m_numPrimitives, 1, (UINT)allocIn.m_offset, 0);
			}
			break;
		}
		case PRIMITIVE_LINE:
		{
			typedef LineRenderAllocationD3D12 Alloc;
			assert(sizeof(Alloc) == sizeOfAlloc);
			const Alloc& alloc = static_cast<const Alloc&>(allocIn);

			assert(alloc.m_numPrimitives >= 0);

			const int numIndices = int(alloc.m_numPrimitives * 2);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			commandList->IASetVertexBuffers(0, 1, &alloc.m_vertexBufferView);

			if (alloc.m_indexBufferView.SizeInBytes)
			{
				commandList->IASetIndexBuffer(nullptr);
				commandList->DrawIndexedInstanced((UINT)numIndices, 1, 0, 0, 0);
			}
			else
			{
				commandList->IASetIndexBuffer(&alloc.m_indexBufferView);
				commandList->DrawInstanced((UINT)numIndices, 1, 0, 0);
			}
			break;
		}
		case PRIMITIVE_TRIANGLE:
		{
			typedef MeshRenderAllocationD3D12 Alloc;
			assert(sizeof(Alloc) == sizeOfAlloc);
			const Alloc& alloc = static_cast<const Alloc&>(allocIn);

			const int numIndices = int(alloc.m_numPrimitives * 3);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, _countof(alloc.m_vertexBufferViews), alloc.m_vertexBufferViews);
			commandList->IASetIndexBuffer(&alloc.m_indexBufferView);

			if (alloc.m_indexBufferView.SizeInBytes)
			{
				commandList->DrawIndexedInstanced((UINT)numIndices, 1, 0, 0, 0);
			}
			else
			{
				commandList->DrawInstanced((UINT)numIndices, 1, 0, 0);
			}
			break;
		}
		default:
		{
			printf("Unhandled primitive type");
		}
	}

	return NV_OK;
}

} // namespace FlexSample
