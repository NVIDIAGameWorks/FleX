#ifndef NV_CO_DX12_RESOURCE_H
#define NV_CO_DX12_RESOURCE_H

#include <NvResult.h>
#include <NvCoDxFormatUtil.h>

#define NOMINMAX
#include <d3d12.h>
#include <string>
#include <wrl.h>

using namespace Microsoft::WRL;

namespace nvidia { 
namespace Common {

// Enables more conservative barriers - restoring the state of resources after they are used.
// Should not need to be enabled in normal builds, as the barriers should correctly sync resources
// If enabling fixes an issue it implies regular barriers are not correctly used. 
#define NV_CO_ENABLE_CONSERVATIVE_RESOURCE_BARRIERS 0

struct Dx12BarrierSubmitter
{
	enum { MAX_BARRIERS = 8 };

		/// Expand one space to hold a barrier
	inline D3D12_RESOURCE_BARRIER& expandOne() { return (m_numBarriers < MAX_BARRIERS) ? m_barriers[m_numBarriers++] : _expandOne(); }
		/// Flush barriers to command list 
	inline void flush() { if (m_numBarriers > 0) _flush(); }

		/// Ctor
	inline Dx12BarrierSubmitter(ID3D12GraphicsCommandList* commandList):m_numBarriers(0), m_commandList(commandList) { }
		/// Dtor
	inline ~Dx12BarrierSubmitter() { flush(); }

	protected:
	D3D12_RESOURCE_BARRIER& _expandOne();
	void _flush();

	ID3D12GraphicsCommandList* m_commandList;
	ptrdiff_t m_numBarriers;
	D3D12_RESOURCE_BARRIER m_barriers[MAX_BARRIERS];
};

/** The base class for resource types allows for tracking of state. It does not allow for setting of the resource though, such that
an interface can return a Dx12ResourceBase, and a client cant manipulate it's state, but it cannot replace/change the actual resource */
struct Dx12ResourceBase
{
		/// Add a transition if necessary to the list
	void transition(D3D12_RESOURCE_STATES nextState, Dx12BarrierSubmitter& submitter);
		/// Get the current state
	inline D3D12_RESOURCE_STATES getState() const { return m_state; }

		/// Get the associated resource
	inline ID3D12Resource* getResource() const { return m_resource; }

		/// True if a resource is set
	inline bool isSet() const { return m_resource != nullptr; }

		/// Coercable into ID3D12Resource
	inline operator ID3D12Resource*() const { return m_resource; }

		/// restore previous state
#if NV_CO_ENABLE_CONSERVATIVE_RESOURCE_BARRIERS
	inline void restore(Dx12BarrierSubmitter& submitter) { transition(m_prevState, submitter); } 
#else
	inline void restore(Dx12BarrierSubmitter& submitter) { (void)submitter; }
#endif

		/// Given the usage, flags, and format will return the most suitable format. Will return DXGI_UNKNOWN if combination is not possible
	static DXGI_FORMAT calcFormat(DxFormatUtil::UsageType usage, ID3D12Resource* resource);

		/// Ctor
	inline Dx12ResourceBase() :
		m_state(D3D12_RESOURCE_STATE_COMMON),
		m_prevState(D3D12_RESOURCE_STATE_COMMON),
		m_resource(nullptr)
	{}
	
	protected:
		/// This is protected so as clients cannot slice the class, and so state tracking is lost
	~Dx12ResourceBase() {}

	ID3D12Resource* m_resource;
	D3D12_RESOURCE_STATES m_state;
	D3D12_RESOURCE_STATES m_prevState;
};

struct Dx12Resource: public Dx12ResourceBase
{

	/// Dtor
	~Dx12Resource()
	{
		if (m_resource)
		{
			m_resource->Release();
		}
	}

		/// Initialize as committed resource
	int initCommitted(ID3D12Device* device, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initState, const D3D12_CLEAR_VALUE * clearValue);

		/// Set a resource with an initial state
	void setResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState);
		/// Make the resource null
	void setResourceNull();
		/// Returns the attached resource (with any ref counts) and sets to nullptr on this.
	ID3D12Resource* detach();

		/// Swaps the resource contents with the contents of the smart pointer
	void swap(ComPtr<ID3D12Resource>& resourceInOut);

		/// Sets the current state of the resource (the current state is taken to be the future state once the command list has executed)
		/// NOTE! This must be used with care, otherwise state tracking can be made incorrect.
	void setState(D3D12_RESOURCE_STATES state);

		/// Set the debug name on a resource
	static void setDebugName(ID3D12Resource* resource, const std::wstring& name);

		/// Set the the debug name on the resource
	void setDebugName(const wchar_t* name);
		/// Set the debug name
	void setDebugName(const std::wstring& name);
};

/// Convenient way to set blobs 
struct Dx12Blob : public D3D12_SHADER_BYTECODE
{
	template <size_t SIZE>
	inline Dx12Blob(const BYTE(&in)[SIZE]) { pShaderBytecode = in; BytecodeLength = SIZE; }
	inline Dx12Blob(const BYTE* in, size_t size) { pShaderBytecode = in; BytecodeLength = size; }
	inline Dx12Blob() { pShaderBytecode = nullptr; BytecodeLength = 0; }
	inline Dx12Blob(ID3DBlob* blob) { pShaderBytecode = blob->GetBufferPointer(); BytecodeLength = blob->GetBufferSize(); }
};

} // namespace Common 
} // namespace nvidia 

#endif // NV_CO_DX12_RESOURCE_H
