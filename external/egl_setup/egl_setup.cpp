//----------------------------------------------------------------------------------
// File:        common/egl_setup/source/egl_setup.cpp
// SDK Version: v1.0 
// Site:        http://developer.nvidia.com/
//
// Copyright (C) 2013 NVIDIA CORPORATION.  All Rights Reserved.
// 
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation is governed by 
// the NVIDIA Pre-Release License Agreement between NVIDIA CORPORATION and
// the licensee. All other uses are strictly forbidden.
//
//----------------------------------------------------------------------------------
#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <stdlib.h>

#include "egl_setup.h"

// EGL_KHR_create_context
#define EGL_CONTEXT_MAJOR_VERSION_KHR           0x3098
#define EGL_CONTEXT_MINOR_VERSION_KHR           0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR	    0x30FD
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR 0x00000002 

#include <android/log.h>

#define EGLLOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "egl_setup", __VA_ARGS__))
#define EGLLOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "egl_setup", __VA_ARGS__))
#define EGLLOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "egl_setup", __VA_ARGS__))
#define EGLLOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "egl_setup", __VA_ARGS__))

static EGLConfig GetBestConfigMatch(EGLDisplay display, EGLint renderable) {
	EGLint count = 0;
	if (!eglGetConfigs(display, NULL, 0, &count))
	{
		EGLLOGE("defaultEGLChooser cannot query count of all configs");
		return 0;
	}

    EGLint numConfigs;

	EGLLOGD("Config count = %d", count);

	EGLConfig* configs = new EGLConfig[count];
	if (!eglGetConfigs(display, configs, count, &count))
	{
		EGLLOGE("defaultEGLChooser cannot query all configs");
		return 0;
	}

	int bestMatch = 1<<30;
	int bestIndex = -1;

	int i;
	for (i = 0; i < count; i++)
	{
		int match = 0;
		EGLint surfaceType = 0;
		EGLint blueBits = 0;
		EGLint greenBits = 0;
		EGLint redBits = 0;
		EGLint alphaBits = 0;
		EGLint depthBits = 0;
		EGLint stencilBits = 0;
		EGLint renderableFlags = 0;

		eglGetConfigAttrib(display, configs[i], EGL_SURFACE_TYPE, &surfaceType);
		eglGetConfigAttrib(display, configs[i], EGL_BLUE_SIZE, &blueBits);
		eglGetConfigAttrib(display, configs[i], EGL_GREEN_SIZE, &greenBits);
		eglGetConfigAttrib(display, configs[i], EGL_RED_SIZE, &redBits);
		eglGetConfigAttrib(display, configs[i], EGL_ALPHA_SIZE, &alphaBits);
		eglGetConfigAttrib(display, configs[i], EGL_DEPTH_SIZE, &depthBits);
		eglGetConfigAttrib(display, configs[i], EGL_STENCIL_SIZE, &stencilBits);
		eglGetConfigAttrib(display, configs[i], EGL_RENDERABLE_TYPE, &renderableFlags);
		EGLLOGD("Config[%d]: R%dG%dB%dA%d D%dS%d Type=%04x Render=%04x",
			i, redBits, greenBits, blueBits, alphaBits, depthBits, stencilBits, surfaceType, renderableFlags);

        // We enforce basic requirements; these are showstoppers, to we do not
        // need to "weight" them
		if ((surfaceType & EGL_WINDOW_BIT) == 0)
			continue;
		if ((renderableFlags & renderable) == 0)
			continue;
		if (depthBits < 16)
			continue;
		if ((redBits < 5) || (greenBits < 6) || (blueBits < 5))
			continue;

		// try to find a config that is R8G8B8A8, D24, S8
	    int penalty = depthBits - 24;
	    match += penalty * penalty;
	    penalty = redBits - 8;
	    match += penalty * penalty;
	    penalty = greenBits - 8;
	    match += penalty * penalty;
	    penalty = blueBits - 8;
	    match += penalty * penalty;
	    penalty = alphaBits - 8;
	    match += penalty * penalty;
	    penalty = stencilBits - 8;
	    match += penalty * penalty;

        // Take any "better" config than the best we've found, and always take the first valid config
	    if ((match < bestMatch) || (bestIndex == -1)) {
		    bestMatch = match;
		    bestIndex = i;
		    EGLLOGD("Config[%d] is the new best config", i, configs[i]);
	    }
	}

	if (bestIndex < 0)
	{
		delete[] configs;
	    EGLLOGE("Fatal error!  No valid configs were found!");
		return 0;
	}

	EGLConfig config = configs[bestIndex];
	delete[] configs;

	return config;
}

const EGLCapabilities* EGLCapabilities::create() {
    EGLCapabilities* caps = new EGLCapabilities;
    EGLBoolean result = EGL_FALSE;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    result = eglInitialize(display, 0, 0);
    if (!result) {
        delete caps;
        return NULL;
    }
 
    caps->m_esConfig = GetBestConfigMatch(display, EGL_OPENGL_ES2_BIT);

  	result = eglBindAPI(EGL_OPENGL_API);
    if (result) {
        caps->m_glConfig = GetBestConfigMatch(display, EGL_OPENGL_BIT);
    } else {
        caps->m_glConfig = 0;
    }

  	result = eglBindAPI(EGL_OPENGL_ES_API);

    return caps;
}
    
EGLCapabilities::EGLCapabilities() {
    m_esConfig = 0;
    m_glConfig = 0;
}

EGLCapabilities::~EGLCapabilities() {
}

EGLInfo* EGLInfo::create(const EGLCapabilities& caps, unsigned int api, unsigned int minAPIVersion) {
    EGLInfo* info = new EGLInfo;

    info->m_api = api;
    info->m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLBoolean result = eglInitialize(info->m_display, 0, 0);
    if (!result) {
        delete info;
        return NULL;
    }

    if ((api == API_ES) && caps.isESCapable()) {
        info->m_config = caps.m_esConfig;

        EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, minAPIVersion,
                                  EGL_NONE };

        info->m_context = eglCreateContext(info->m_display, info->m_config, 
            NULL, contextAttrs);
    	if (info->m_context == EGL_NO_CONTEXT) {
            delete info;
	        EGLLOGW("Failed to create context!");
		    return NULL;
	    }
    } else if ((api == API_GL) && caps.isGLCapable()) {
    	result = eglBindAPI(EGL_OPENGL_API);
        if (!result) {
            delete info;
	        EGLLOGW("Failed to bind GL API!");
		    return NULL;
        }

        info->m_config = caps.m_glConfig;

        EGLint contextAttrs[] = { EGL_CONTEXT_MAJOR_VERSION_KHR, minAPIVersion, 
                                  EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                  EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
                                  EGL_NONE };

        info->m_context = eglCreateContext(info->m_display, info->m_config, 
            NULL, contextAttrs);
    	if (info->m_context == EGL_NO_CONTEXT) {
            delete info;
	        EGLLOGW("Failed to create context!");
		    return NULL;
	    }
    } else {
        delete info;
        return NULL;
    }

    eglQueryContext(info->m_display, info->m_context, EGL_CONTEXT_CLIENT_VERSION, &info->m_apiVersion);

    return info;
}

bool EGLInfo::createWindowSurface(ANativeWindow* window) {
    // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
    // guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
    // As soon as we picked a EGLConfig, we can safely reconfigure the
    // ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
	EGLint format;
	if (!eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &format)) {
	    EGLLOGE("Fatal error!  Failed to get config format!");
		return false;
	}

    ANativeWindow_setBuffersGeometry(window, 0, 0, format);
    m_surface = eglCreateWindowSurface(m_display, m_config, window, NULL);

    return m_surface != NULL;
}

EGLInfo::EGLInfo() {
	m_display = EGL_NO_DISPLAY;
    m_surface = EGL_NO_SURFACE;
    m_context = EGL_NO_CONTEXT;
	m_config = 0;
}

EGLInfo::~EGLInfo() {
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_display, m_context);
    }
    if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(m_display, m_surface);
    }
    eglTerminate(m_display);
}

