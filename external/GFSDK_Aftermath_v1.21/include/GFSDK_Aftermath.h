/*
* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

/* 
*   █████  █████ ██████ ████  ████   ███████   ████  ██████ ██   ██
*   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
*   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
*   ██████ ████    ██   ████  █████  ██ ██ ██ ██████   ██   ███████
*   ██  ██ ██      ██   ██    ██  ██ ██    ██ ██  ██   ██   ██   ██
*   ██  ██ ██      ██   █████ ██  ██ ██    ██ ██  ██   ██   ██   ██   DEBUGGER
*                                                           ██   ██
*  ████████████████████████████████████████████████████████ ██ █ ██ ████████████ 
*
*       
*  HOW TO USE;
*           
*  1)  Call, 'GFSDK_Aftermath_DXxx_Initialize', to initialize the library.            
*      This must be done before any other library calls are made, and the method
*      must return 'GFSDK_Aftermath_Result_Success' for initialization to
*      be complete.
*
*      NOTE: Your application will need to be enabled for Aftermath - if this isn't
*            the case, you can rename the executable to; "NvAftermath-Enable.exe"
*            to enable Aftermath during development.
*
*
*  2)  Call 'GFSDK_Aftermath_DXxx_SetEventMarker', to inject an event 
*      marker directly into the command stream at that point.
*
*
*  3)  Once TDR/hang occurs, call the 'GFSDK_Aftermath_DXxx_GetData' API 
*      to fetch the event marker last processed by the GPU for each context.
*      This API also supports fetching the current execution state for each
*      the GPU.
*
*
*
*  PERFORMANCE TIP;
*
*  o) Try make as few calls to 'GFSDK_Aftermath_DXxx_GetData', as possible.  The
*     API is flexible enough to allow collection of data from all contexts in one call.
*
*
*      Contact: Alex Dunn [adunn@nvidia.com]
*/

#ifndef GFSDK_Aftermath_H
#define GFSDK_Aftermath_H

#include "defines.h"

enum GFSDK_Aftermath_Version { GFSDK_Aftermath_Version_API = 0x00000121 }; // Version 1.21

enum GFSDK_Aftermath_Result
{
    GFSDK_Aftermath_Result_Success = 0x1,
        
    GFSDK_Aftermath_Result_Fail = 0xBAD00000,

    // The callee tries to use a library version
    //  which does not match the built binary.
    GFSDK_Aftermath_Result_FAIL_VersionMismatch = GFSDK_Aftermath_Result_Fail | 1,

    // The library hasn't been initialized, see;
    //  'GFSDK_Aftermath_Initialize'.
    GFSDK_Aftermath_Result_FAIL_NotInitialized = GFSDK_Aftermath_Result_Fail | 2,

    // The callee tries to use the library with 
    //  a non-supported GPU.  Currently, only 
    //  NVIDIA GPUs are supported.
    GFSDK_Aftermath_Result_FAIL_InvalidAdapter = GFSDK_Aftermath_Result_Fail | 3,

    // The callee passed an invalid parameter to the 
    //  library, likely a null pointer or bad handle.
    GFSDK_Aftermath_Result_FAIL_InvalidParameter = GFSDK_Aftermath_Result_Fail | 4,

    // Something weird happened that caused the 
    //  library to fail for some reason.
    GFSDK_Aftermath_Result_FAIL_Unknown = GFSDK_Aftermath_Result_Fail | 5,

    // Got a fail error code from the graphics API.
    GFSDK_Aftermath_Result_FAIL_ApiError = GFSDK_Aftermath_Result_Fail | 6,

    // Make sure that the NvAPI DLL is up to date.
    GFSDK_Aftermath_Result_FAIL_NvApiIncompatible = GFSDK_Aftermath_Result_Fail | 7,

    // It would appear as though a call has been
    //  made to fetch the Aftermath data for a 
    //  context that hasn't been used with 
    //  the EventMarker API yet.
    GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList = GFSDK_Aftermath_Result_Fail | 8,

    // Looks like the library has already been initialized.
     GFSDK_Aftermath_Result_FAIL_AlreadyInitialized = GFSDK_Aftermath_Result_Fail | 9,

    // Debug layer not compatible with Aftermath.
    GFSDK_Aftermath_Result_FAIL_D3DDebugLayerNotCompatible = GFSDK_Aftermath_Result_Fail | 10,

    // Aftermath not enabled in the driver.  This is
    //  generally dealt with via application profile,
    //  but if installed driver version is greater than 
    //  382.53, this issue can be resolved by renaming
    //  the application to: "NvAftermath-Enable.exe".
    GFSDK_Aftermath_Result_FAIL_NotEnabledInDriver = GFSDK_Aftermath_Result_Fail | 11,

    // Aftermath requires driver version r378.66 and beyond
    GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported = GFSDK_Aftermath_Result_Fail | 12,

    // The system ran out of memory for allocations
    GFSDK_Aftermath_Result_FAIL_OutOfMemory = GFSDK_Aftermath_Result_Fail | 13,

    // No need to get data on bundles, as markers 
    //  execute on the command list.
    GFSDK_Aftermath_Result_FAIL_GetDataOnBundle = GFSDK_Aftermath_Result_Fail | 14,

    // No need to get data on deferred contexts, as markers 
    //  execute on the immediate context.
    GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext = GFSDK_Aftermath_Result_Fail | 15,
};

#define GFSDK_Aftermath_SUCCEED(value) (((value) & 0xFFF00000) != GFSDK_Aftermath_Result_Fail)


enum GFSDK_Aftermath_Context_Status
{
    // GPU:
    // The GPU has not started processing this command list yet.
    GFSDK_Aftermath_Context_Status_NotStarted = 0,

    // This command list has begun execution on the GPU.Thi
    GFSDK_Aftermath_Context_Status_Executing,

    // This command list has finished execution on the GPU.
    GFSDK_Aftermath_Context_Status_Finished,

    // This context has an invalid state, which could be
    //  caused by an error.  
    //
    //  NOTE: See, 'GFSDK_Aftermath_ContextData::getErrorCode()'
    //  for more information.
    GFSDK_Aftermath_Context_Status_Invalid,
};


enum GFSDK_Aftermath_Status
{
    // The GPU is still active, and hasn't gone down.
    GFSDK_Aftermath_Status_Active = 0,

    // A long running shader/operation has caused a 
    //  GPU timeout. Reconfiguring the timeout length
    //  might help tease out the problem.
    GFSDK_Aftermath_Status_Timeout,

    // Run out of memory to complete operations.
    GFSDK_Aftermath_Status_OutOfMemory,

    // An invalid VA access has caused a fault.
    GFSDK_Aftermath_Status_PageFault,

    // Unknown problem - likely using an older driver
    //  incompatible with this Aftermath feature.
    GFSDK_Aftermath_Status_Unknown,
};


// Used with, 'GFSDK_Aftermath_DXxx_GetData'.  Filled with information,
//  about each requested context.
extern "C" {
    struct GFSDK_Aftermath_DLLSPEC GFSDK_Aftermath_ContextData
    {
        void* markerData;
        unsigned int markerSize;
        GFSDK_Aftermath_Context_Status status;

        // Call this when 'status' is 'GFSDK_Aftermath_Context_Status_Invalid;
        //  to determine what the error failure reason is.
        GFSDK_Aftermath_Result getErrorCode();
    };
}


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_DX11_Initialize
// GFSDK_Aftermath_DX12_Initialize
// ---------------------------------
//
// [pDx11Device]; DX11-Only
//      the current dx11 device pointer.
//
// [pDx12Device]; DX12-Only
//      the current dx12 device pointer.
//
// version;
//      use the version supplied in this header - library will match 
//      that with what it believes to be the version internally.
//
//// DESCRIPTION;
//      Library must be initialized before any other call is made.
//      This should be done after device creation.
// 
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX11_Initialize(GFSDK_Aftermath_Version version, ID3D11Device* const pDx11Device);
#endif
#ifdef __d3d12_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version version, ID3D12Device* const pDx12Device);
#endif


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Dx11_SetEventMarker
// GFSDK_Aftermath_Dx12_SetEventMarker
// -------------------------------------
// 
// (pDx11DeviceContext); DX11-Only
//      Command list currently being populated. 
//
// (pDx12CommandList); DX12-Only
//      Command list currently being populated. 
// 
// markerData;
//      Pointer to data used for event marker.
//      NOTE: An internal copy will be made of this data, no 
//      need to keep it around after this call - stack alloc is safe.
//
// markerSize;
//      Size of event in bytes.
//      NOTE:   Passing a 0 for this parameter is valid, and will
//              only copy off the ptr supplied by markerData, rather
//              than internally making a copy.
//              NOTE:   This is a requirement for deferred contexts on D3D11.
//
// DESCRIPTION;
//      Drops a event into the command stream with a payload that can be 
//      linked back to the data given here, 'markerData'.  It's 
//      safe to call from multiple threads simultaneously, normal D3D 
//      API threading restrictions apply.
//
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX11_SetEventMarker(ID3D11DeviceContext* const pDx11DeviceContext, void* markerData, unsigned int markerSize);
#endif
#ifdef __d3d12_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX12_SetEventMarker(ID3D12GraphicsCommandList* const pDx12CommandList, void* markerData, unsigned int markerSize);
#endif


/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_DX11_GetData
// GFSDK_Aftermath_DX12_GetData
// ------------------------------
// 
// numContexts;
//      Number of contexts to fetch information for.
//      NOTE:   Passing a 0 for this parameter will only 
//              return the GPU status in pStatusOut.
//
// (ppDx11DeviceContexts); DX11-Only
//      Array of pointers to ID3D11DeviceContexts containing Aftermath 
//      event markers. 
//
// (ppDx12CommandLists); DX12-Only
//      Array of pointers to ID3D12GraphicsCommandList containing Aftermath 
//      event markers.
// 
// pContextDataOut;
//      OUTPUT: context data for each context requested. Contains event
//              last reached on the GPU, and status of context if
//              applicable (DX12-Only).
//      NOTE: must allocate enough space for 'numContexts' worth of structures.
//            stack allocation is fine.
//
// pStatusOut;
//      OUTPUT: the current status of the GPU.
//
// DESCRIPTION;
//      Once a TDR/crash/hang has occurred (or whenever you like), call 
//      this API to retrieve the event last processed by the GPU on the 
//      given context.  Also - fetch the GPUs current execution status.
//      
//      NOTE:   Passing a 0 for numContexts will only return the GPU 
//              status in pStatusOut and skip fetching the current value
//              for each context.  You will still need to pass something
//              via ppDx11DeviceContexts or ppDx12CommandLists though...
//
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX11_GetData(const unsigned int numContexts, ID3D11DeviceContext* const* ppDx11DeviceContexts, GFSDK_Aftermath_ContextData* pContextDataOut, GFSDK_Aftermath_Status* pStatusOut);
#endif
#ifdef __d3d12_h__
GFSDK_Aftermath_API GFSDK_Aftermath_DX12_GetData(const unsigned int numContexts, ID3D12GraphicsCommandList* const* ppDx12CommandLists, GFSDK_Aftermath_ContextData* pContextDataOut, GFSDK_Aftermath_Status* pStatusOut);
#endif


/////////////////////////////////////////////////////////////////////////
//
// NOTE: Function table provided - if dynamic loading is preferred.
//
/////////////////////////////////////////////////////////////////////////
#ifdef __d3d11_h__
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX11_Initialize)(GFSDK_Aftermath_Version version, ID3D11Device* const pDx11Device);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX11_SetEventMarker)(ID3D11DeviceContext* const pDx11DeviceContext, void* markerData, unsigned int markerSize);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX11_GetData)(const unsigned int numContexts, ID3D11DeviceContext* const* ppDx11DeviceContexts, GFSDK_Aftermath_ContextData* pContextDataOut, GFSDK_Aftermath_Status* pStatusOut);
#endif
#ifdef __d3d12_h__
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX12_Initialize)(GFSDK_Aftermath_Version version, ID3D12Device* const pDx12Device);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX12_SetEventMarker)(ID3D12GraphicsCommandList* const pDx12CommandList, void* markerData, unsigned int markerSize);
GFSDK_Aftermath_PFN(*PFN_GFSDK_Aftermath_DX12_GetData)(const unsigned int numContexts, ID3D12GraphicsCommandList* const* ppDx12CommandLists, GFSDK_Aftermath_ContextData* pContextDataOut, GFSDK_Aftermath_Status* pStatusOut);
#endif

#endif // GFSDK_Aftermath_H