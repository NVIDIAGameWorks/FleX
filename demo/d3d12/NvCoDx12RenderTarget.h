#ifndef NV_CO_DX12_RENDER_TARGET_H
#define NV_CO_DX12_RENDER_TARGET_H

#include <DirectXMath.h>
#include <NvCoDx12DescriptorHeap.h>
#include <NvCoDx12Resource.h>
#include "appD3D12Ctx.h"
#include "core/maths.h"

namespace nvidia {
namespace Common {

using namespace DirectX;

class Dx12RenderTarget
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS_BASE(Dx12RenderTarget);
public:
	enum BufferType
	{
		BUFFER_TARGET,
		BUFFER_DEPTH_STENCIL,
		BUFFER_COUNT_OF,
	};

	struct Desc
	{
		void init(int width, int height, DXGI_FORMAT targetFormat = DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D32_FLOAT, int usageFlags = 0)
		{
			m_width = width;
			m_height = height;
			m_targetFormat = targetFormat;
			m_depthStencilFormat = depthStencilFormat;
			m_usageFlags = usageFlags;

			const Vec4 clearColor = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
			m_targetClearColor = clearColor;
			m_depthStencilClearDepth = 1.0;
		}

		int m_usageFlags;								///< Usage flags from DxFormatUtil
		int m_width;
		int m_height;
		DXGI_FORMAT m_depthStencilFormat;				///< DXGI_FORMAT_UNKNOWN means don't allocate resource
		DXGI_FORMAT m_targetFormat;						///< DXGI_FORMAT_UNKNOWN means don't allocate resource 
		Vec4 m_targetClearColor;
		float m_depthStencilClearDepth;
	};

	int init(AppGraphCtxD3D12* renderContext, const Desc& desc);
	
	void setShadowDefaultLight(FXMVECTOR eye, FXMVECTOR at, FXMVECTOR up);
	void setShadowLightMatrices(FXMVECTOR eye, FXMVECTOR lookAt, FXMVECTOR up, float sizeX, float sizeY, float zNear, float zFar);

	void bind(AppGraphCtxD3D12* renderContext);

	void clear(AppGraphCtxD3D12* renderContext);
	void bindAndClear(AppGraphCtxD3D12* renderContext);
	
	void toWritable(AppGraphCtxD3D12* renderContext);
	void toReadable(AppGraphCtxD3D12* renderContext);
		
		/// Set the debug name
	void setDebugName(BufferType type, const std::wstring& name);
		/// Set name (plus if depth/target) for all buffers
	void setDebugName(const std::wstring& name);

		/// Get the 'primary' buffer type. 
	BufferType getPrimaryBufferType() const { return m_renderTarget ? BUFFER_TARGET : BUFFER_DEPTH_STENCIL; }

		/// Get the resource by the buffer type
	const Dx12ResourceBase& getResource(BufferType type) const { return type == BUFFER_TARGET ? m_renderTarget : m_depthStencilBuffer; }
	Dx12Resource& getResource(BufferType type) { return type == BUFFER_TARGET ? m_renderTarget : m_depthStencilBuffer; }

		/// Calculates a default shader resource view 
	D3D12_SHADER_RESOURCE_VIEW_DESC calcDefaultSrvDesc(BufferType type) const;

		/// Allocates a srv view. Stores the index in m_srvView. If < 0 there isn't an associated srv
		/// If the desc isn't set one will be created via calcDefaultSrvDesc
	int allocateSrvView(BufferType type, ID3D12Device* device, Dx12DescriptorHeap& heap, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc = nullptr);

		/// Gets the appropriate format for reading as srv
	DXGI_FORMAT getSrvFormat(BufferType type) const;
		/// Gets the suitable target for rendering to
	DXGI_FORMAT getTargetFormat(BufferType type) const;
		/// Get the amount of render targets
	inline int getNumRenderTargets() const { return m_renderTarget ? 1 : 0; }

		/// Create a srv at the index on the heap for the buffer type
	void createSrv(ID3D12Device* device, Dx12DescriptorHeap& heap, BufferType type, int descriptorIndex);

		/// Get an associated srv heap index (as produced by allocateSrvView)
	inline int getSrvHeapIndex(BufferType type) const { return m_srvIndices[type]; }

		/// Get the desc
	inline const Desc& getDesc() const { return m_desc; }

	Dx12RenderTarget();

	static DxFormatUtil::UsageType getUsageType(BufferType type);

	int m_srvIndices[BUFFER_COUNT_OF];

	XMMATRIX m_shadowLightView;
	XMMATRIX m_shadowLightProjection;
	XMMATRIX m_shadowLightWorldToTex;

	Desc m_desc;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	Dx12Resource m_renderTarget;
	ComPtr<ID3D12DescriptorHeap> m_descriptorHeapRtv;
	Dx12Resource m_depthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> m_descriptorHeapDsv;
};

} // namespace Common 
} // namespace nvidia

#endif // NV_CO_DX12_RENDER_TARGET_H