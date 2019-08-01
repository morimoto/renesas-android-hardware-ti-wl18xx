/*
 * Copyright (C) 2019 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "bt_vendor_test"

extern "C" {
#include "bt_vendor-ti.h"
#include "upio-ti.h"
#include "userial-ti.h"
#include "bt_vendor_lib.h"
#include "bt_vendor-ti-tests.h"
#include "upio-ti-tests.h"
#include "userial-ti-tests.h"
}

#include <string.h>

#include <gtest/gtest.h>

#include <cutils/properties.h>
#include <utils/Log.h>

#include <libbt_test.h>

extern upio_ti_stubs upio_stubs;
extern userial_ti_stubs userial_stubs;

extern vnd_userial_cb_t vnd_userial;

extern bt_vendor_callbacks_t* bt_vendor_cbacks;
extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];
extern unsigned int hci_tty_fd;

int return_failure(void) {
    return STATUS_FAIL;
}
int return_success(void) {
    return STATUS_SUCCESS;
}

class LibbtVendorTest : public ::testing::Test {
protected:
    void SetUp() override {
        userial_set_stubs(NULL);

        upio_set_stubs(NULL, NULL);
    }

    void TearDown() override {
        userial_set_stubs(NULL);

        upio_set_stubs(NULL, NULL);
    }
};

TEST_F(LibbtVendorTest, UpioIsRfkillDisabledTest) {
    char value[PROPERTY_VALUE_MAX];
    property_get("ro.rfkilldisabled", value, "0");
    if (strcmp(value, "1") == 0) {
        ASSERT_EQ(get_is_rfkill_disabled()(), STATUS_FAIL);
    }
    else {
        ASSERT_EQ(get_is_rfkill_disabled()(), STATUS_SUCCESS);
    }
}

TEST_F(LibbtVendorTest, UpioInitRfkillTest) {
    upio_stubs.is_rfkill_disabled_stub = return_failure;
    ASSERT_EQ(get_init_rfkill()(), STATUS_FAIL);
}

TEST_F(LibbtVendorTest, UpioSetBtPowerTest) {
    upio_stubs.is_rfkill_disabled_stub = return_failure;
    ASSERT_EQ(upio_set_bluetooth_power(UPIO_BT_POWER_ON), STATUS_FAIL);
    upio_stubs.is_rfkill_disabled_stub = return_success;
    upio_stubs.init_rfkill_stub = return_failure;
    ASSERT_EQ(upio_set_bluetooth_power(UPIO_BT_POWER_ON), STATUS_FAIL);
}

TEST_F(LibbtVendorTest, UserialVendorOpenTest) {
    char prev_port_name[VND_PORT_NAME_MAXLEN];
    snprintf(prev_port_name, VND_PORT_NAME_MAXLEN, "%s", \
             vnd_userial.port_name);
    snprintf(vnd_userial.port_name, VND_PORT_NAME_MAXLEN, "%s", \
             "/file_not_exist");
    ASSERT_EQ(userial_vendor_open(), -1);
    snprintf(vnd_userial.port_name, VND_PORT_NAME_MAXLEN, "%s", \
             prev_port_name);
}

TEST_F(LibbtVendorTest, BtVendorTiInitWithNull) {
    uint8_t new_bd_addr[BD_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
            old_bd_addr[BD_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ASSERT_EQ(getVendorInterface()->init(nullptr, new_bd_addr), BT_HC_STATUS_FAIL);
    for(int i = 0; i < BD_ADDR_LEN; ++i) {
        ASSERT_EQ(vnd_local_bd_addr[i], old_bd_addr[i]);
    }
}
