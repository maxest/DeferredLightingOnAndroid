LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := deferred_lighting

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/BlossomFramework/modules/common/include \
	$(LOCAL_PATH)/BlossomFramework/modules/math/include \

LOCAL_SRC_FILES := \
	deferred_lighting.cpp \
	BlossomFramework/modules/common/src/camera.cpp \
	BlossomFramework/modules/math/src/common.cpp \
	BlossomFramework/modules/math/src/plane.cpp \
	BlossomFramework/modules/math/src/vector.cpp \
	BlossomFramework/modules/math/src/matrix.cpp \
	
LOCAL_LDLIBS := -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)
