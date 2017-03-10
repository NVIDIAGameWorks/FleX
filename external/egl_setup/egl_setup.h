//----------------------------------------------------------------------------------
// File:        common/egl_setup/include/egl_setup.h
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
#ifndef __EGL_SETUP_H
#define __EGL_SETUP_H

// File including this must include the desired EGL implementation before this file
#include <android/native_window.h>

class EGLCapabilities {
public:
    friend class EGLInfo;
    static const EGLCapabilities* create();
    ~EGLCapabilities();

    bool isESCapable() const { return m_esConfig > 0; }
    bool isGLCapable() const { return m_glConfig > 0; }

protected:
    EGLCapabilities();
    EGLConfig m_esConfig;
    EGLConfig m_glConfig;
};

class EGLInfo {
public:
    enum {
        API_ES,
        API_GL
    };

    static EGLInfo* create(const EGLCapabilities& caps, unsigned int api, unsigned int minAPIVersion);
    ~EGLInfo();

    bool createWindowSurface(ANativeWindow* window);

    EGLDisplay getDisplay() const { return m_display; }
    EGLContext getContext() const { return m_context; }
    EGLConfig getConfig() const { return m_config; }
    EGLSurface getSurface() const { return m_surface; }
    unsigned int getAPI() { return m_api; }
    int getAPIVersion() { return m_apiVersion; }

protected:
    EGLInfo();

    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
	EGLConfig m_config;
    unsigned int m_api;
    int m_apiVersion;
};

#endif
