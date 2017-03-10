LOCAL_PATH := $(call my-dir)
# Regal prebuilt static library
include $(CLEAR_VARS)
LOCAL_MODULE := regal_static

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/lib/armeabi-v7a/libRegal_static.a

include $(PREBUILT_STATIC_LIBRARY)
