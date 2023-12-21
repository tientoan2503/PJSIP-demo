LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE 	:= libSoliCallSDK
LOCAL_SRC_FILES := libSoliCallSDK-debug.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE 	:= solicall_jni
LOCAL_SRC_FILES := solicall_jni.cpp test.cpp
					
LOCAL_C_INCLUDES +=  /home/shaul/AAA/V0/inc
LOCAL_CFLAGS = -DANDROID -D_LINUX

LOCAL_ARM_MODE := arm
LOCAL_LDLIBS := -ldl -llog


LOCAL_STATIC_LIBRARIES := libSoliCallSDK

include $(BUILD_SHARED_LIBRARY)
