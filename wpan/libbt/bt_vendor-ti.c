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

#define LOG_TAG "bt_vendor"

#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <utils/Log.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <bt_vendor_lib.h>
#include <bt_hci_bdroid.h>

#include "bt_vendor-ti.h"
#include "userial-ti.h"
#include "upio-ti.h"

extern void hw_config_start(void);

bt_vendor_callbacks_t* bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[BD_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const int BT_HC_STATUS_FAIL = -1;
unsigned int hci_tty_fd = -1;

static int ti_init(const bt_vendor_callbacks_t* p_cb,
                   unsigned char* local_bdaddr) {
    ALOGI("vendor Init");

    if (p_cb == NULL) {
        ALOGE("init failed with no user callbacks!");
        return BT_HC_STATUS_FAIL;
    }

    userial_vendor_init();
    memcpy(vnd_local_bd_addr, local_bdaddr, BD_ADDR_LEN);
    // store reference to user callbacks
    bt_vendor_cbacks = (bt_vendor_callbacks_t*)p_cb;
    return 0;
}

static void ti_cleanup(void) {
    ALOGI("vendor cleanup");
    bt_vendor_cbacks = NULL;
}

static int ti_op(bt_vendor_opcode_t opcode, void* param) {
    ALOGD("vendor op - %d", opcode);

    switch (opcode) {
    case BT_VND_OP_POWER_CTRL: {
        int* state = (int*)param;
        upio_set_bluetooth_power(UPIO_BT_POWER_OFF);

        if (*state == BT_VND_PWR_ON) {
            ALOGW("NOTE: BT_VND_PWR_ON now forces power-off first");
            upio_set_bluetooth_power(UPIO_BT_POWER_ON);
        } else {
            upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
        }

        break;
    }

    case BT_VND_OP_FW_CFG: {
        hw_config_start();
        break;
    }

    // Since new stack expects scocfg_cb we are returning SUCCESS here
    case BT_VND_OP_SCO_CFG: {
        bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);
        break;
    }

    case BT_VND_OP_USERIAL_OPEN: {
        int fd = userial_vendor_open();
        int* fd_array = (int*)param;
        fd_array[CH_CMD] = fd;
        hci_tty_fd = fd; // for userial_close op
        return 1;        // CMD/EVT/ACL on same fd
    }

    case BT_VND_OP_USERIAL_CLOSE: {
        close(hci_tty_fd);
        break;
    }

    case BT_VND_OP_LPM_WAKE_SET_STATE: {
        ALOGD("vendor op - BT_VND_OP_LPM_WAKE_SET_STATE");
        break;
    }

    default: {
        break;
    }
    }

    return 0;
}

const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE  = {
    sizeof(bt_vendor_interface_t),
    ti_init,
    ti_op,
    ti_cleanup,
};
