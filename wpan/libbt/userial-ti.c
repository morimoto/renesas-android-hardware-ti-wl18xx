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

#define LOG_TAG "bt_vendor_userial"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "bt_vendor-ti.h"
#include "userial-ti.h"
#include <unistd.h>
#include <time.h>

vnd_userial_cb_t vnd_userial;

void userial_vendor_init(void) {
    vnd_userial.fd = -1;
    snprintf(vnd_userial.port_name, VND_PORT_NAME_MAXLEN, "%s", \
             BLUETOOTH_UART_DEVICE_PORT);
}

int userial_vendor_open(void) {
    vnd_userial.fd = open(vnd_userial.port_name, O_RDWR);

    if (vnd_userial.fd < 0) {
        ALOGE(" Can't open hci_tty (%s)", strerror(errno));
        return -1;
    }

    if (userial_set_default_baud() < 0) {
        ALOGE(" userial_set_default_baud() failed");
        return -1;
    }

    fcntl(vnd_userial.fd, F_SETFL, fcntl(vnd_userial.fd, F_GETFL) | O_NONBLOCK);
    return vnd_userial.fd;
}

void userial_vendor_close(void) {
    int result;

    if (vnd_userial.fd == -1) {
        return;
    }

    ALOGI("device fd = %d close", vnd_userial.fd);
    // flush Tx before close to make sure no chars in buffer
    tcflush(vnd_userial.fd, TCIOFLUSH);

    if ((result = close(vnd_userial.fd)) < 0) {
        ALOGE( "close(fd:%d) FAILED result:%d", vnd_userial.fd, result);
    }

    vnd_userial.fd = -1;
}

int userial_vendor_set_baud(int baud_rate, int flow_ctrl) {
    struct termios2 ti2;
    userial_get_termios();

    /*Set the UART flow control */
    if (flow_ctrl) {
        vnd_userial.termios.c_cflag |= CRTSCTS;
    } else {
        vnd_userial.termios.c_cflag &= ~CRTSCTS;
    }

    /*
     * Set the parameters associated with the UART
     * The change will occur immediately by using TCSANOW
     */
    if (tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios) < 0) {
        ALOGE(" Can't set port settings");
        return -1;
    }

    tcflush(vnd_userial.fd, TCIOFLUSH);
    /*Set the actual baud rate */
    ioctl(vnd_userial.fd, TCGETS2, &ti2);
    ti2.c_cflag &= ~CBAUD;
    ti2.c_cflag |= BOTHER;
    ti2.c_ospeed = baud_rate;
    ioctl(vnd_userial.fd, TCSETS2, &ti2);
    return 0;
}

int userial_set_default_baud(void) {
    userial_get_termios();
    /* Change the UART attributes before
     * setting the default baud rate*/
    cfmakeraw(&vnd_userial.termios);
    vnd_userial.termios.c_cflag |= 1;
    vnd_userial.termios.c_cflag |= CRTSCTS;
    /* Set the attributes of UART after making
     * the above changes*/
    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);
    /* Set the actual default baud rate */
    cfsetospeed(&vnd_userial.termios, B115200);
    cfsetispeed(&vnd_userial.termios, B115200);
    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);
    tcflush(vnd_userial.fd, TCIOFLUSH);
    return 0;
}

void userial_get_termios(void) {
    memset (&vnd_userial.termios, 0, sizeof vnd_userial.termios);
    tcflush(vnd_userial.fd, TCIOFLUSH);

    /* Get the attributes of UART */
    if (tcgetattr(vnd_userial.fd, &vnd_userial.termios) < 0) {
        ALOGE(" Can't get port settings");
    }
}
