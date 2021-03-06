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

#define LOG_TAG "bt_upio"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/properties.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <bt_vendor-ti.h>
#include <upio-ti.h>
#include <userial-ti.h>

#ifdef UNITTEST
#include <upio-ti-tests.h>
upio_ti_stubs upio_stubs = {
    .init_rfkill_stub = NULL
};
#endif // UNITTEST

#define MAX_PATH_LEN    64
#define BUF_LEN         16

static int rfkill_id = -1;
static char* rfkill_state_path = NULL;

static int init_rfkill() {
    char path[MAX_PATH_LEN];
    char buf[BUF_LEN];
    int fd, sz, id;

#ifdef UNITTEST
    if ((upio_stubs.init_rfkill_stub != NULL) &&
        (upio_stubs.init_rfkill_stub != init_rfkill)) {
        return upio_stubs.init_rfkill_stub();
    }
#endif // UNITTEST

    for (id = 0; ; id++) {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
        fd = open(path, O_RDONLY);

        if (fd < 0) {
            ALOGE("init_rfkill : open(%s) failed: %s (%d)\n",
                  path, strerror(errno), errno);
            return STATUS_FAIL;
        }

        sz = read(fd, &buf, sizeof(buf));
        close(fd);

        if (sz >= 9 && memcmp(buf, "bluetooth", 9) == 0) {
            rfkill_id = id;
            break;
        }
    }

    asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", rfkill_id);
    return STATUS_SUCCESS;
}

int upio_set_bluetooth_power(int on) {
    int sz;
    int fd = -1;
    int ret = STATUS_FAIL;
    char buffer = '0';

    switch (on) {
    case UPIO_BT_POWER_OFF:
        buffer = '0';
        break;

    case UPIO_BT_POWER_ON:
        buffer = '1';
        break;
    }

    if (rfkill_id == -1) {
        if (init_rfkill() == STATUS_FAIL) {
            return ret;
        }
    }

    fd = open(rfkill_state_path, O_WRONLY);

    if (fd < 0) {
        ALOGE("set_bluetooth_power : open(%s) for write failed: %s (%d)",
              rfkill_state_path, strerror(errno), errno);
        return ret;
    }

    sz = write(fd, &buffer, 1);

    if (sz < 0) {
        ALOGE("set_bluetooth_power : write(%s) failed: %s (%d)",
              rfkill_state_path, strerror(errno), errno);
    } else {
        ret = STATUS_SUCCESS;
    }

    if (fd >= 0) {
        close(fd);
    }

    return ret;
}

#ifdef UNITTEST
void upio_set_stubs(iv_stub init_rfkill_stub) {
    upio_stubs.init_rfkill_stub = init_rfkill_stub;
}

upio_ti_stubs* get_upio_stubs(void) {
    return &upio_stubs;
}

iv_func get_init_rfkill(void) {
    return init_rfkill;
}
#endif // UNITTEST
