#
# Copyright 2001-2012 Texas Instruments, Inc. - http://www.ti.com/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
ifeq ($(TARGET_DEVICE),kingfisher)

LOCAL_PATH := $(call my-dir)

BDROID_DIR := system/bt

LIBBT_SRC_FILES := \
        bt_vendor-ti.c \
        userial-ti.c \
        upio-ti.c \
        hardware-ti.c \

LIBBT_TEST_FILES := \
        tests/libbt_test.cpp \

LIBBT_CFLAGS := -Wall -Werror

LIBBT_LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/device/include \

LIBBT_LOCAL_TEST_INCLUDES := \
        $(LOCAL_PATH)/tests_include \

LIBBT_SHARED_LIBS := \
        libcutils \
        liblog \

include $(CLEAR_VARS)
LOCAL_CFLAGS += $(LIBBT_CFLAGS)
LOCAL_SRC_FILES := $(LIBBT_SRC_FILES)
LOCAL_HEADER_LIBRARIES := libutils_headers
LOCAL_C_INCLUDES += \
        $(LIBBT_LOCAL_C_INCLUDES) \
        $(BDROID_DIR) \

LOCAL_SHARED_LIBRARIES := $(LIBBT_SHARED_LIBS)
LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := ti
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

# UNIT TESTS
include $(CLEAR_VARS)
LOCAL_CFLAGS += \
        $(LIBBT_CFLAGS) \
        -DUNITTEST=1 \

LOCAL_SRC_FILES := \
        $(LIBBT_SRC_FILES) \
        $(LIBBT_TEST_FILES) \

LOCAL_HEADER_LIBRARIES := libutils_headers
LOCAL_C_INCLUDES := \
        $(LIBBT_LOCAL_C_INCLUDES) \
        $(LIBBT_LOCAL_TEST_INCLUDES) \

LOCAL_SHARED_LIBRARIES := \
        $(LIBBT_SHARED_LIBS) \

LOCAL_MODULE := libbt_vendor_test
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_OWNER := ti
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_NATIVE_TEST)

LIBBT_SRC_FILES :=
LIBBT_TEST_FILES :=
LIBBT_CFLAGS :=
LIBBT_LOCAL_C_INCLUDES :=
LIBBT_LOCAL_TEST_INCLUDES :=
LIBBT_SHARED_LIBS :=

endif # kingfisher
