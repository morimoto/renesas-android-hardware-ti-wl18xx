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

#ifndef LIBBTVENDORTI_H
#define LIBBTVENDORTI_H
#include "bt_vendor_lib.h"

#ifndef VENDOR_LIB_RUNTIME_TUNING_ENABLED
#define VENDOR_LIB_RUNTIME_TUNING_ENABLED   FALSE
#endif // VENDOR_LIB_RUNTIME_TUNING_ENABLED

#ifndef USE_CONTROLLER_BDADDR
#define USE_CONTROLLER_BDADDR               TRUE
#endif // USE_CONTROLLER_BDADDR

// Device port name where Bluetooth controller attached
#ifndef BLUETOOTH_UART_DEVICE_PORT
#define BLUETOOTH_UART_DEVICE_PORT          "/dev/ttySC1"
#endif // BLUETOOTH_UART_DEVICE_PORT

#ifndef UART_TARGET_BAUD_RATE
#define UART_TARGET_BAUD_RATE               2900000
#endif // UART_TARGET_BAUD_RATE

#define UART_TARGET_FLOW_CNTRL              1
// BD address length in format xx:xx:xx:xx:xx:xx
#define BD_ADDR_LEN                         6

enum {
    STATUS_FAIL,
    STATUS_SUCCESS
};

extern bt_vendor_callbacks_t* bt_vendor_cbacks;

#endif // LIBBTVENDORTI_H
