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

#include "NvFlexTypes.h"
#include "Vector.h"
#include <Windows.h>
#include <vector>
#include <atomic>
#include <map>

 // NV shader extensions
#include "../../../external/nvapi/include/nvShaderExtnEnums.h"


#define ENABLE_AMD_AGS 1 // enable AMD AGS shader extensions, used for warp shuffle based reductions
#if ENABLE_AMD_AGS
 // AMD shader extensions
#include <external/ags_lib/inc/amd_ags.h>
#endif

#define USE_GPUBB 0 //Used for D3D12 shader debugging
#define ENABLE_D3D12 1

enum GpuVendorId
{
	VENDOR_ID_NVIDIA = 0x10DE,
	VENDOR_ID_AMD = 0x1002,
	VENDOR_ID_OTHERS = 0x00EE,
};

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

//! graphics API dependent descriptions
struct NvFlexContextDesc;

//! export interop buffers
struct NvFlexBuffer;
struct NvFlexTexture3D;

//! graphics API dependent descriptions
struct NvFlexBufferViewDesc;
struct NvFlexTexture3DViewDesc;

/// common object type
struct NvFlexObject
{
	virtual NvFlexUint addRef() = 0;
	virtual NvFlexUint release() = 0;
};

//struct NvFlexBuffer : public NvFlexObject {};
//struct NvFlexTexture3D : public NvFlexObject {};

namespace NvFlex
{
	struct Context;

	struct Allocable
	{
		void* operator new(size_t size);
		void* operator new[](size_t size);
		void operator delete(void* ptr);
		void operator delete[](void* ptr);
	};

	struct Object : public Allocable
	{
		virtual NvFlexUint addRefInternal();

		virtual NvFlexUint releaseInternal();

		virtual ~Object();

	private:
		std::atomic_uint32_t m_refCount = 1u;
	};

	#define NV_FLEX_OBJECT_IMPL \
	virtual NvFlexUint addRef() { return Object::addRefInternal(); } \
	virtual NvFlexUint release() { return Object::releaseInternal(); }

	#define NV_FLEX_DISPATCH_MAX_READ_TEXTURES ( 30u )
	#define NV_FLEX_DISPATCH_MAX_WRITE_TEXTURES ( 8u )

	#define NV_FLEX_DRAW_MAX_READ_TEXTURES ( 8u )
	#define NV_FLEX_DRAW_MAX_WRITE_TEXTURES ( 1u )

	struct ConstantBufferDesc
	{
		NvFlexUint stride;
		NvFlexUint dim;
		bool uploadAccess;
#if defined(_DEBUG)
		const char* name;
#endif
	};

	enum BufferTypes
	{
		eBuffer = 1 << 0,
		eUAV_SRV = 1 << 1,
		eStage = 1 << 2,
		eRaw = 1 << 3,
		eStructured = 1 << 4,
		eIndirect = 1 << 5,
		eShared = 1 << 6
	};

	struct BufferDesc
	{
		NvFlexFormat format;
		NvFlexUint dim;
		NvFlexUint stride;
		unsigned int bufferType;
		const void* data;
#if defined(_DEBUG)
		const char* name;
#endif
	};

	struct BufferViewDesc
	{
		NvFlexFormat format;
	};

	struct Texture3DDesc
	{
		NvFlexFormat format;
		NvFlexDim dim;
		bool uploadAccess;
		bool downloadAccess;
		const void* data;
#if defined(_DEBUG)
		const char* name;
#endif
	};

	struct ComputeShaderDesc
	{
		ComputeShaderDesc() : cs(nullptr), cs_length(0), label(L""), NvAPI_Slot(~0u) {}
		ComputeShaderDesc(void* shaderByteCode, NvFlexUint64 byteCodeLength, wchar_t* shaderLabel = L"", NvFlexUint nvApiSlot = ~0u): cs(shaderByteCode), cs_length (byteCodeLength), label(shaderLabel), NvAPI_Slot(nvApiSlot){}
		const void* cs;
		NvFlexUint64 cs_length;
		const wchar_t* label;
		NvFlexUint NvAPI_Slot;
	};

	struct InputElementDesc
	{
		const char* semanticName;
		NvFlexFormat format;
	};

	/// A basic map-discard constant buffer
	struct ConstantBuffer : public NvFlexObject
	{
		virtual ~ConstantBuffer() {}

		ConstantBufferDesc m_desc;
	};

	/// A GPU resource that supports read operations
	struct Resource : public NvFlexObject {};

	/// A GPU resource that supports read/write operations
	/// includes a Resource
	struct ResourceRW : public NvFlexObject
	{
		virtual Resource* getResource() = 0;
	};

	
	/// Context created resources
	/// includes Resource, ResourceRW
	struct Buffer : public NvFlexObject
	{
		virtual Resource* getResource() = 0;
		virtual ResourceRW* getResourceRW() = 0;
		virtual ~Buffer() {}
		BufferDesc m_desc;
	};

	/// Staging resource
	struct Stage : public NvFlexObject 
	{
		virtual ~Stage() {}
		BufferDesc m_desc;
	};

	/// includes Resource, ResourceRW
	struct Texture3D : public NvFlexObject
	{
		virtual Resource* getResource() = 0;
		virtual ResourceRW* getResourceRW() = 0;
		virtual ~Texture3D() {}
		Texture3DDesc m_desc;
	};

	struct ComputeShader : public NvFlexObject
	{
		virtual ~ComputeShader() {}
		ComputeShaderDesc m_desc;
	};

	struct Timer : public NvFlexObject
	{
		virtual ~Timer() {}
	};

	struct Fence : public NvFlexObject
	{
		virtual ~Fence() {}
	};

	struct DispatchParams
	{
		ComputeShader* shader;
		NvFlexDim gridDim;
		ConstantBuffer* rootConstantBuffer;
		ConstantBuffer* secondConstantBuffer;
		Resource* readOnly[NV_FLEX_DISPATCH_MAX_READ_TEXTURES];
		ResourceRW* readWrite[NV_FLEX_DISPATCH_MAX_WRITE_TEXTURES];
		Buffer * IndirectLaunchArgs;
	};

	struct MappedData
	{
		void* data;
		NvFlexUint rowPitch;
		NvFlexUint depthPitch;
	};

	// D3D events are expensive to create and destroy, so this pool enables 
	// DxRangeTimers to be reused.
	struct TimerPool
	{
		virtual ~TimerPool() {}
		// We initialize new elements instead of std::vector doing it, to avoid 
		// having to allocate D3D11Query in a copy constructor
		//returns an index to the timer in the pool
		//virtual int alloc() = 0;

		virtual int begin() = 0;
		virtual void end(int index) = 0;

		// synchronize timers between GPU and CPU
		virtual void resolve() = 0;
		// Copy timers to cpu
		virtual void sync() = 0;

		virtual void clear() = 0;

		virtual float get(int index) = 0;
		// Set the number of timers in the pool
		// In D3D12 this results in a reallocation of the timer heap
		virtual void reserve(size_t size) = 0;
		static const int m_initPoolSize = 200;
		size_t m_timerPoolSize = 0;		//The size of the heap
		size_t m_usedElemCount = 0;		//How many elements are used
		size_t m_requestedElemCount = 0;	//How many timers were requested
	};

	// String comparison function for timer name to timer index map
	struct ltstr
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};


	// WString comparison function for timer name to timer index map
	struct ltwstr
	{
		bool operator()(const wchar_t* s1, const wchar_t* s2) const
		{
			return wcscmp(s1, s2) < 0;
		}
	};

	// Each named range can occur more than once, for example once per iteration
	// So the map from name to timer has to be a multimap

	typedef std::multimap<const char*, int, ltstr> NameToTimerMap;
	typedef NameToTimerMap::iterator NameToTimerMapIter;

	typedef std::multimap<const wchar_t*, int, ltwstr> NameToTimerMapW;
	typedef NameToTimerMapW::iterator NameToTimerMapWIter;

	struct Context
	{
		// ************ public interface *****************

		virtual void updateContext(const NvFlexContextDesc* desc) = 0;
		
		//virtual void updateBufferViewDesc(NvFlexBuffer* buffer, NvFlexBufferViewDesc* desc) = 0;

		//virtual void updateTexture3DViewDesc(NvFlexTexture3D* buffer, NvFlexTexture3DViewDesc* desc) = 0;

		// ************ internal resources ***************

		virtual ConstantBuffer* createConstantBuffer(const ConstantBufferDesc* desc) = 0;

		virtual void* map(ConstantBuffer* buffer) = 0;

		virtual void unmap(ConstantBuffer* buffer) = 0;

		virtual void copy(ConstantBuffer* dst, Buffer* src) = 0;

		virtual Buffer* createBuffer(const BufferDesc* desc, void* ptr = 0) = 0;

		virtual Buffer* createBufferView(Buffer* buffer, const BufferViewDesc* desc) = 0;

		virtual void* map(Buffer* buffer) = 0;

		virtual void* mapUpload(Buffer* buffer) = 0;

		virtual void unmap(Buffer* buffer) = 0;

		virtual void upload(Buffer* buffer) = 0;

		virtual void upload(Buffer* buffer, NvFlexUint offset, NvFlexUint numBytes) = 0;

		virtual void download(Buffer* buffer) = 0;

		virtual void download(Buffer* buffer, NvFlexUint offset, NvFlexUint numBytes) = 0;

		virtual void copy(Buffer* dst, unsigned dstByteOffset, Buffer* src, NvFlexUint srcByteOffset, NvFlexUint numBytes) = 0;

		virtual void copyToNative(void* dst, unsigned dstByteOffset, Buffer* src, NvFlexUint srcByteOffset, NvFlexUint numBytes) = 0;

		virtual void copyToDevice(Buffer* dst, unsigned dstByteOffset, Buffer* src, NvFlexUint srcByteOffset, NvFlexUint numBytes) = 0;

		virtual void copyToHost(Buffer* dst, unsigned dstByteOffset, Buffer* src, NvFlexUint srcByteOffset, NvFlexUint numBytes) = 0;

		virtual void clearUnorderedAccessViewUint(Buffer* buffer, const NvFlexUint * values) = 0;

		virtual void copyResourceState(Buffer* bufferFrom, Buffer* bufferTo) = 0;

		//virtual void updateSubresource(Buffer* bufferIn, const NvFlexUint * box, const void * data) = 0;

		virtual Texture3D* createTexture3D(const Texture3DDesc* desc) = 0;

		virtual MappedData map(Texture3D* buffer) = 0;

		virtual void unmap(Texture3D* buffer) = 0;

		virtual void download(Texture3D* buffer) = 0;

		virtual MappedData mapDownload(Texture3D* buffer) = 0;

		virtual void unmapDownload(Texture3D* buffer) = 0;

		virtual void copy(Texture3D* dst, Texture3D* srv) = 0;

		virtual ComputeShader* createComputeShader(const ComputeShaderDesc* desc) = 0;

		// ***************** Operations ****************

		virtual void dispatch(const DispatchParams* params) = 0;

		virtual void dispatchIndirect(const DispatchParams* params) = 0;
		
		virtual TimerPool * createTimerPool() = 0;

		virtual Fence* createFence() = 0;

		virtual void fenceSet(Fence * fence) = 0;

		virtual void fenceWait(Fence * fence) = 0;

		virtual void startFrame() = 0;

		virtual void flush() = 0;

		virtual void clearState() = 0;

		// ***************** Profiling ****************

		virtual void eventMarker(const wchar_t* label) = 0;

		virtual void contextPush() = 0;

		virtual void contextPop() = 0;

		bool mEnableTimers;

		NameToTimerMapW mNameToInternalTimerMap;
		NameToTimerMap mNameToExternalTimerMap;

		Context();
		virtual ~Context();
	};
}