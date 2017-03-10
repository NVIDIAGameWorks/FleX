LOCAL_PATH := $(call my-dir)
# Regal prebuilt shared library
include $(CLEAR_VARS)
LOCAL_MODULE := regal_shared

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := $(LOCAL_PATH)/lib/armeabi-v7a/libRegal.so

include $(PREBUILT_SHARED_LIBRARY)
