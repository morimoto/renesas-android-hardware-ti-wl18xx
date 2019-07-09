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

#ifndef USERIAL_VENDOR_H
#define USERIAL_VENDOR_H

#include <termios.h>
#include "bt_vendor-ti.h"
#include "userial.h"

#define VND_PORT_NAME_MAXLEN    256

/* vendor serial control block */
typedef struct {
    int fd;                     /* fd to Bluetooth device */
    struct termios termios;     /* serial terminal of BT port */
    char port_name[VND_PORT_NAME_MAXLEN];
} vnd_userial_cb_t;

extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];

void userial_vendor_init(void);
int userial_vendor_open(void);
void userial_vendor_close(void);
int userial_vendor_set_baud(int baud_rate, int flow_ctrl);
int userial_set_default_baud(void);
int read_hci_event(unsigned char* buf, int size);
int read_hci_packet(unsigned char* buf, int size);
void userial_get_termios(void);

#endif // USERIAL_VENDOR_H
