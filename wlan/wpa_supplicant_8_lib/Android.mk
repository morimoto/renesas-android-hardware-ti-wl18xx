#
# Copyright (C) 2008 The Android Open Source Project
# Copyright (C) 2019 GlobalLogic
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ifeq ($(WPA_SUPPLICANT_VERSION),VER_0_8_X)

LOCAL_PATH := $(call my-dir)

WPA_SUPPL_DIR = external/wpa_supplicant_8

include $(WPA_SUPPL_DIR)/wpa_supplicant/android.config


include $(CLEAR_VARS)
LOCAL_MODULE := lib_driver_cmd_wl18xx
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := driver_cmd_nl80211.c

LOCAL_C_INCLUDES := \
	$(WPA_SUPPL_DIR)/src \
	$(WPA_SUPPL_DIR)/src/drivers \
	$(WPA_SUPPL_DIR)/src/utils

# Try to be neat.
LOCAL_CFLAGS := \
	-Wall \
	-Wextra \
	-Werror \

# Suppress warnings which are "inherited" via headers.
LOCAL_CFLAGS += \
	-Wno-unused-parameter \
	-Wno-macro-redefined \

# This is essential to use the same CFI options as
# the wpa_supplicant to prevent run-time crash.
LOCAL_CFLAGS += \
	-fvisibility=default \
	-flto \
	-fsanitize=cfi

include $(BUILD_STATIC_LIBRARY)

endif #($(WPA_SUPPLICANT_VERSION),VER_0_8_X)
