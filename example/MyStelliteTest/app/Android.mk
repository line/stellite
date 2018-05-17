# $(warning hhddebug xxx : $(xxx))

APP_STL := c++_static
LOCAL_PATH := $(call my-dir)
CPP_PATH := $(LOCAL_PATH)/src/main/cpp
LIB_PATH := $(LOCAL_PATH)/src/main/cpp/stellite/bin/$(TARGET_ARCH_ABI)



####################################################################################
## STELLITE
include $(CLEAR_VARS)
LOCAL_MODULE := STELLITE
LOCAL_SRC_FILES := $(LIB_PATH)/libstellite_http_client.so
#LOCAL_SRC_FILES := $(LIB_PATH)/libstellite_http_client.a
include $(PREBUILT_SHARED_LIBRARY)
#include $(PREBUILT_STATIC_LIBRARY)



####################################################################################
## native-lib
include $(CLEAR_VARS)
LOCAL_MODULE := native-lib
LOCAL_LDLIBS := -llog -latomic

LOCAL_C_INCLUDES := $(CPP_PATH)/ \
                    $(CPP_PATH)/stellite/include/

LOCAL_SRC_FILES :=	$(CPP_PATH)/native-lib.cpp \
					$(CPP_PATH)/MyStelliteHttpCallback.cpp

LOCAL_SHARED_LIBRARIES := STELLITE
#LOCAL_STATIC_LIBRARIES := STELLITE
include $(BUILD_SHARED_LIBRARY)