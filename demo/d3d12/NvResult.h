/* Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited. */

#ifndef NV_RESULT_H
#define NV_RESULT_H

#include <assert.h>

/** \addtogroup core
@{
*/

/*! Result is designed to be compatible with COM HRESULT. 

It's layout in bits is as follows

Severity | Facility | Code
---------|----------|-----
31       |    30-16 | 15-0

Severity - 1 fail, 0 is success.
Facility is where the error originated from
Code is the code specific to the facility.
     
The layout is designed such that failure is a negative number, and success is positive due to Result
being represented by an Int32.

Result codes have the following style

1. NV_name
2. NV_s_f_name
3. NV_s_name

where s is severity as a single letter S - success, and E for error
Style 1 is reserved for NV_OK and NV_FAIL as they are so common and not tied to a facility

s is S for success, E for error 
f is the short version of the facility name

For the common used NV_OK and NV_FAIL, the name prefix is dropped
It is acceptable to expand 'f' to a longer name to differentiate a name
ie for a facility 'DRIVER' it might make sense to have an error of the form NV_E_DRIVER_OUT_OF_MEMORY */
typedef int NvResult;

//! Make a result
#define NV_MAKE_RESULT(sev, fac, code) ((((int)(sev))<<31) | (((int)(fac))<<16) | ((int)(code)))

//! Will be 0 - for ok, 1 for failure
#define NV_GET_RESULT_SEVERITY(r)	((int)(((NvUInt32)(r)) >> 31))
//! Will be 0 - for ok, 1 for failure
#define NV_GET_RESULT_FACILITY(r)	((int)(((r) >> 16) & 0x7fff))
//! Get the result code
#define NV_GET_RESULT_CODE(r)		((int)((r) & 0xffff))

#define NV_SEVERITY_ERROR         1
#define NV_SEVERITY_SUCCESS       0

#define NV_MAKE_ERROR(fac, code)	NV_MAKE_RESULT(NV_SEVERITY_ERROR, NV_FACILITY_##fac, code)
#define NV_MAKE_SUCCESS(fac, code)	NV_MAKE_RESULT(NV_SEVERITY_SUCCESS, NV_FACILITY_##fac, code)

/*************************** Facilities ************************************/

//! General - careful to make compatible with HRESULT
#define NV_FACILITY_GENERAL      0

//! Base facility -> so as to not clash with HRESULT values
#define NV_FACILITY_BASE		 0x100				

/*! Facilities numbers must be unique across a project to make the resulting result a unique number!
It can be useful to have a consistent short name for a facility, as used in the name prefix */
#define NV_FACILITY_DISK         (NV_FACILITY_BASE + 1)
#define NV_FACILITY_INTERFACE    (NV_FACILITY_BASE + 2)
#define NV_FACILITY_UNKNOWN      (NV_FACILITY_BASE + 3)
#define NV_FACILITY_MEMORY		 (NV_FACILITY_BASE + 4)
#define NV_FACILITY_MISC	     (NV_FACILITY_BASE + 5)

/// Base for external facilities. Facilities should be unique across modules. 
#define NV_FACILITY_EXTERNAL_BASE 0x200
#define NV_FACILITY_HAIR		(NV_FACILITY_EXTERNAL_BASE + 1)

/* *************************** Codes **************************************/

// Memory
#define NV_E_MEM_OUT_OF_MEMORY            NV_MAKE_ERROR(MEMORY, 1)
#define NV_E_MEM_BUFFER_TOO_SMALL         NV_MAKE_ERROR(MEMORY, 2)

//! NV_OK indicates success, and is equivalent to NV_MAKE_RESULT(0, NV_FACILITY_GENERAL, 0)
#define NV_OK                          0
//! NV_FAIL is the generic failure code - meaning a serious error occurred and the call couldn't complete
#define NV_FAIL                        NV_MAKE_ERROR(GENERAL, 1)

//! Used to identify a Result that has yet to be initialized.  
//! It defaults to failure such that if used incorrectly will fail, as similar in concept to using an uninitialized variable. 
#define NV_E_MISC_UNINITIALIZED			NV_MAKE_ERROR(MISC, 2)
//! Returned from an async method meaning the output is invalid (thus an error), but a result for the request is pending, and will be returned on a subsequent call with the async handle. 			
#define NV_E_MISC_PENDING				NV_MAKE_ERROR(MISC, 3)
//! Indicates that a handle passed in as parameter to a method is invalid. 
#define NV_E_MISC_INVALID_HANDLE		NV_MAKE_ERROR(MISC, 4)

/*! Set NV_HANDLE_RESULT_FAIL(x) to code to be executed whenever an error occurs, and is detected by one of the macros */
#ifndef NV_HANDLE_RESULT_FAIL
#	define NV_HANDLE_RESULT_FAIL(x)
#endif

/* !!!!!!!!!!!!!!!!!!!!!!!!! Checking codes !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

//! Use to test if a result was failure. Never use result != NV_OK to test for failure, as there may be successful codes != NV_OK.
#define NV_FAILED(status) ((status) < 0)
//! Use to test if a result succeeded. Never use result == NV_OK to test for success, as will detect other successful codes as a failure.
#define NV_SUCCEEDED(status) ((status) >= 0)

//! Helper macro, that makes it easy to add result checking to calls in functions/methods that themselves return Result. 
#define NV_RETURN_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_HANDLE_RESULT_FAIL(_res); return _res; } }
//! Helper macro that can be used to test the return value from a call, and will return in a void method/function
#define NV_RETURN_VOID_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_HANDLE_RESULT_FAIL(_res); return; } }
//! Helper macro that will return false on failure.
#define NV_RETURN_FALSE_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { NV_HANDLE_RESULT_FAIL(_res); return false; } }

//! Helper macro that will assert if the return code from a call is failure, also returns the failure.
#define NV_CORE_ASSERT_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { assert(false); return _res; } }
//! Helper macro that will assert if the result from a call is a failure, also returns. 
#define NV_CORE_ASSERT_VOID_ON_FAIL(x) { NvResult _res = (x); if (NV_FAILED(_res)) { assert(false); return; } }


#if __cplusplus
namespace nvidia {
typedef NvResult Result;
} // namespace nvidia
#endif

/** @} */

#endif
