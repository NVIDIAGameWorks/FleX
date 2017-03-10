#pragma once

#include "Context.h"

#include <d3d11.h>
#include <d3d11_1.h>

#if ENABLE_D3D12
#include <d3d12.h>
#endif

#include <external/nvapi/include/nvapi.h>

#include <dxgi.h>
#include <string>
#include <vector>

#define ENABLE_LIVE_DEVICE_OBJECTS_REPORTING 0	// Provides detailed report of D3D object reference counts
#define USE_NVAPI_DEVICE 0						// Enable this for D3D11 SCG

struct FlexDeviceDesc
{
	FlexDeviceDesc() : mDeviceNumber(-1), mDevice(nullptr), mCommandQueue(nullptr), useComputeQueue(false), enablePresent(false) {}
	virtual ~FlexDeviceDesc() {}
	unsigned int mDeviceNumber;  //!< Compute device number
	void * mDevice;				 //!< pointer to existing device
	void * mCommandQueue;		 //!< pointer to existing command queue for DX12
	bool useComputeQueue;		 //!< Use the compute queue instead of the graphics queue for DX12
	bool enablePresent;			 //!< Present after each frame to that APIC and PB dump work
};

namespace NvFlex
{

	struct Device : public NvFlexObject
	{
		Device();
		virtual ~Device();

		virtual Context* createContext() = 0;

		virtual NvAPI_Status NvAPI_IsNvShaderExtnOpCodeSupported(NvU32 opCode, bool * pSupported) = 0;

#if ENABLE_AMD_AGS
		virtual AGSReturnCode AGS_DriverExtensions_Init(AGSContext* agsContext, unsigned int agsApiSlot, unsigned int *extensionsSupported) = 0;
		virtual void AGS_DeInit(AGSContext* agsContext) = 0;
#endif

		virtual DXGI_ADAPTER_DESC1 getDXGIAdapter(void* d3ddevice, int & deviceNumber) = 0;
		virtual DXGI_ADAPTER_DESC1 getDeviceNumber(const LUID adapterID, int & deviceNumber);
		virtual std::vector<IDXGIAdapter1 *> enumerateDXGIAdapters();
		virtual void queryDeviceCapabilities(void* d3ddevice);

		virtual void reportLiveDeviceObjects() {}

//#if APIC_CAPTURE
		virtual void createWindow();
		virtual void present() = 0;

		WNDCLASSEX mWindowDesc;
		RECT mWinRect;
		HWND m_hWnd;
		IDXGISwapChain*	m_swapChain;
//#endif

		IDXGIFactory1* m_pFactory;

		NvFlexUint mSMCount;
		std::string mDeviceName;
		// Gpu Vendor Id
		GpuVendorId mGpuVendorId;
		bool mD3D12Device;
		Context* m_context;
	};
}
