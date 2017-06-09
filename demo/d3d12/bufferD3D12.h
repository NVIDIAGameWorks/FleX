#ifndef BUFFER_D3D12_H
#define BUFFER_D3D12_H

#include <NvCoDx12Resource.h>

#include "renderStateD3D12.h"

#define NOMINMAX
#include <dxgi.h>
#include <d3d12.h>

namespace FlexSample { 
using namespace nvidia;

struct IndexBufferD3D12: public NvCo::Dx12Resource
{
	int init(const RenderStateD3D12& state, int stride, ptrdiff_t numIndices, const void* sysMem);
		/// Reset
	void reset() { setResourceNull(); }

	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};

struct VertexBufferD3D12: public NvCo::Dx12Resource
{
		/// 
	int init(const RenderStateD3D12& state, int stride, ptrdiff_t numElements, const void* sysMem, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE);
	int init(const RenderStateD3D12& state, int srcStride, int stride, ptrdiff_t numElements, const void* sysMem, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE);

		/// Ctor
	VertexBufferD3D12() {}

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
};

} // namespace FlexSample

#endif // BUFFER_D3D12_H
