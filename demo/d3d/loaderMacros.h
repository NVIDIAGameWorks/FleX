/*
 * Copyright (c) 2014-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

 #pragma once

/// ----------------------------- Module Loader -----------------------------

#define LOADER_ESC_N(...) __VA_ARGS__

#define LOADER_PARAM_DECLARE(a, b) a b
#define LOADER_PARAM_NAME(a, b) b

#define LOADER_ESC_PARAM0(PARAM, p0)
#define LOADER_ESC_PARAM1(PARAM, p0) PARAM p0
#define LOADER_ESC_PARAM2(PARAM, p0, p1) PARAM p0,						LOADER_ESC_PARAM1(PARAM, p1)
#define LOADER_ESC_PARAM3(PARAM, p0, p1, p2) PARAM p0,					LOADER_ESC_PARAM2(PARAM, p1, p2)
#define LOADER_ESC_PARAM4(PARAM, p0, p1, p2, p3) PARAM p0,				LOADER_ESC_PARAM3(PARAM, p1, p2, p3)
#define LOADER_ESC_PARAM5(PARAM, p0, p1, p2, p3, p4) PARAM p0,			LOADER_ESC_PARAM4(PARAM, p1, p2, p3, p4)
#define LOADER_ESC_PARAM6(PARAM, p0, p1, p2, p3, p4, p5) PARAM p0,		LOADER_ESC_PARAM5(PARAM, p1, p2, p3, p4, p5)
#define LOADER_ESC_PARAM7(PARAM, p0, p1, p2, p3, p4, p5, p6) PARAM p0,	LOADER_ESC_PARAM6(PARAM, p1, p2, p3, p4, p5, p6)

#define LOADER_ESC_MERGE(a, b) (a, b)

#define LOADER_ESC(mode, numParams, params) \
	LOADER_ESC_PARAM##numParams LOADER_ESC_MERGE(mode, LOADER_ESC_N params)

#define LOADER_DECLARE_FUNCTION_PTR(inst, inst_func, retType, method, numParams, params) \
	typedef retType (*method##_ptr_t)( LOADER_ESC (LOADER_PARAM_DECLARE, numParams, params) );

#define LOADER_DECLARE_FUNCTION_VAR(inst, inst_func, retType, method, numParams, params) \
	method##_ptr_t method##_ptr; 
	
#define LOADER_DECLARE_FUNCTION_NAME(inst, inst_func, retType, method, numParams, params) \
	extern retType inst_func(method) (LOADER_ESC (LOADER_PARAM_DECLARE, numParams, params) );

#define LOADER_SET_FUNCTION(inst, inst_func, retType, method, numParams, params) \
	inst.method##_ptr = inst_func(method); 
	

#define LOADER_FUNCTION_PTR_CALL(inst, inst_func, retType, method, numParams, params) \
	retType method( LOADER_ESC (LOADER_PARAM_DECLARE, numParams, params) ) { \
		return inst.method##_ptr( LOADER_ESC (LOADER_PARAM_NAME, numParams, params) ); \
	}

#define LOADER_LOAD_FUNCTION_PTR(inst, inst_func, retType, method, numParams, params) \
	inst->method##_ptr = (retType (*)( LOADER_ESC (LOADER_PARAM_DECLARE, numParams, params) ))(inst_func(inst, #method));

#define LOADER_CLEAR_FUNCTION_PTR(inst, inst_func, retType, method, numParams, params) \
	inst->method##_ptr = nullptr;

#define LOADER_DECLARE_FUNCTION(inst, inst_func, retType, method, numParams, params) \
	retType method( LOADER_ESC (LOADER_PARAM_DECLARE, numParams, params) );

#define LOADER_FUNCTION_IMPL(inst, inst_func, retType, method, numParams, params) \
	retType method( LOADER_ESC (LOADER_PARAM_DECLARE, numParams, params) ) { \
		return method##Impl( LOADER_ESC (LOADER_PARAM_NAME, numParams, params) ); \
	}

