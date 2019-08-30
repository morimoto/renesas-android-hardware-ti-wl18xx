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

#define LOG_TAG "bt_vendor_hardware"

#include <utils/Log.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "bt_hci_bdroid.h"
#include <bt_vendor-ti.h>
#include <userial-ti.h>
#include <upio-ti.h>

#define ACTION_SEND_COMMAND                 1
#define ACTION_WAIT_EVENT                   2
#define ACTION_SERIAL                       3
#define ACTION_DELAY                        4
#define ACTION_RUN_SCRIPT                   5
#define ACTION_REMARKS                      6

#define HCI_RESET                           0x0C03
#define HCI_VSC_UPDATE_BAUDRATE             0xFF36
#define HCI_VSC_WRITE_BD_ADDR               0xFC06
#define HCI_VS_SLP_CFG                      0xFD0C
#define HCI_READ_LOCAL_BDADDR               0x1009
#define HCI_READ_LOCAL_VERSION              0x1001
#define HCI_VS_WRITE_SCO_CONFIG             0xFE10
#define HCI_VS_WRITE_CODEC_CONFIG           0xFD06
#define HCI_VS_WRITE_CODEC_CONFIG_ENCHANCED 0xFD07

#define HCI_CMD_MAX_LEN                     258
#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE    5
#define HCI_EVT_CMD_CMPL_OPCODE             3
#define HCI_EVT_CMD_CMPL_LOCAL_BDADDR_ARRAY 6
#define UPDATE_BAUDRATE_CMD_PARAM_SIZE      6
#define HCI_CMD_PREAMBLE_SIZE               3
#define HCIC_PARAM_SIZE_SCO_CFG             5
#define HCIC_PARAM_SIZE_CODEC_CFG           34
#define HCIC_PARAM_SIZE_CODEC_CFG_ENHANCED  26

#define FILENAME_MAX_LEN                    255
#define ZERO_OFFSET                         0
#define MASK_CHIP                           0x7C00
#define MASK_MIN_VER                        0x007F
#define MASK_MAJ_VER                        0x0380
#define OFFSET_CHIP                         10
#define OFFSET_MAJ_VER                      7
#define MAJ_VER_CARRY_BIT                   0x8000

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define MAKEWORD(a, b)  ((unsigned short)(((unsigned char)(a)) \
    | ((unsigned short)((unsigned char)(b))) << 8))

// Hardware Configuration State
enum {
    HW_CFG_START = 1,
    HW_CFG_SET_UART_BAUD,
    HW_CFG_SET_BD_ADDR,
    HW_CFG_READ_LOCAL_VER,
    HW_CFG_DOWNLOAD,
    HW_CFG_DOWNLOAD_END
#if (USE_CONTROLLER_BDADDR == TRUE)
    , HW_CFG_READ_BD_ADDR
#endif
};

// h/w config control block
typedef struct {
    uint8_t state;                          // Hardware configuration state
    uint8_t* fw_data;                       // FW buffer
    uint8_t* cur_action;
    long len;
} bt_hw_cfg_cb_t;

static void hw_config_cback(void* p_mem);
static void hc_fill_hci_cmd_preamble(HC_BT_HDR* p_buf);
static void hci_vs_write_codec_configuration_enhanced(void);
static void hci_vs_write_codec_configuration(void);
static void hci_vs_write_sco_configuration(void);
static void hci_send(int command, HC_BT_HDR* p_buf, tINT_CMD_CBACK callback);
static void hc_fill_buffer_hci_vs_write_codec_config_enchanced(HC_BT_HDR* p_buf);
static void hc_fill_buffer_hci_vs_write_codec_config(HC_BT_HDR* p_buf);
static void hc_fill_buffer_hci_vs_write_sco_config(HC_BT_HDR* p_buf);
static HC_BT_HDR* hc_allocate_buffer(void);
static void hci_free_buffer(void* p_mem);
static void abort_sco_configuration(void);

extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];
static bt_hw_cfg_cb_t hw_cfg_cb = {0, NULL, NULL, 0};
extern vnd_userial_cb_t vnd_userial;

typedef struct __attribute__ ((packed)) {
    uint32_t magic;
    uint32_t version;
    uint8_t future[24];
    uint8_t actions[];
} bts_header_t;

// bts_action_t - Each .bts action has its own type of data.
typedef struct __attribute__ ((packed)) {
    uint16_t type;
    uint16_t size;
    uint8_t data[];
} bts_action_t;

typedef struct __attribute__ ((packed)) {
    uint8_t prefix;
    uint16_t opcode;
    uint8_t plen;
    uint8_t data[];
} hci_command_t;

const size_t bts_action_t_size = sizeof(bts_action_t);

static uint8_t hw_config_load_bts(const char* bts_src_filename) {
    FILE* bts_scr_file = fopen(bts_src_filename, "rb");

    if (bts_scr_file == NULL) {
        ALOGE("%s: Firmware file opening failed: %s", __func__, strerror(errno));
        return STATUS_FAIL;
    }

    fseek(bts_scr_file, ZERO_OFFSET, SEEK_END);
    hw_cfg_cb.len = ftell(bts_scr_file);
    rewind(bts_scr_file);
    hw_cfg_cb.fw_data = malloc(hw_cfg_cb.len);

    if (hw_cfg_cb.fw_data == NULL) {
        ALOGE("%s: malloc error", __func__);
        fclose(bts_scr_file);
        return STATUS_FAIL;
    }

    if ((long)(fread(hw_cfg_cb.fw_data, 1, hw_cfg_cb.len, bts_scr_file)) != hw_cfg_cb.len) {
        ALOGE("%s: Firmware file reading failed: %s", __func__, strerror(errno));
        fclose(bts_scr_file);
        return STATUS_FAIL;
    }

    fclose(bts_scr_file);
    hw_cfg_cb.cur_action = hw_cfg_cb.fw_data +
                           sizeof(bts_header_t);
    hw_cfg_cb.len -= (long)(sizeof(bts_header_t));
    hw_cfg_cb.state = HW_CFG_DOWNLOAD;
    return STATUS_SUCCESS;
}

static uint8_t hw_config_read_local_version(HC_BT_HDR* p_buf) {
    uint8_t* p = p_buf->data;
    UINT16_TO_STREAM(p, HCI_READ_LOCAL_VERSION);
    *p = 0; // parameter length
    p_buf->len = HCI_CMD_PREAMBLE_SIZE;
    hw_cfg_cb.state = HW_CFG_READ_LOCAL_VER;
    return bt_vendor_cbacks->xmit_cb(HCI_READ_LOCAL_VERSION, p_buf,
                                     hw_config_cback);
}

static void skip_hci_command(uint8_t** ptr, long* len) {
    if (ptr == NULL || *ptr == NULL || len == NULL) {
        ALOGE("(%s): wrong parameters", __func__);
        return;
    }

    bts_action_t* cur_action = (bts_action_t*)(*ptr);
    bts_action_t* nxt_action = (bts_action_t*)(*ptr + bts_action_t_size +
                               cur_action->size);

    if (nxt_action->type != ACTION_WAIT_EVENT) {
        ALOGE("invalid action after skipped command");
    } else {
        *ptr = *ptr + bts_action_t_size + cur_action->size;
        *len = *len - (long)(bts_action_t_size +
                       cur_action->size);
        // warn user on not commenting these in firmware
        ALOGW("skipping the wait event");
    }
}

void hw_config_start(void) {
    HC_BT_HDR* p_buf = NULL;
    uint8_t* p = NULL;
    hw_cfg_cb.state = 0;

    // Start from sending HCI_RESET
    if (bt_vendor_cbacks) {
        p_buf = (HC_BT_HDR*)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE +
                HCI_CMD_PREAMBLE_SIZE);
    }

    if (p_buf) {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = ZERO_OFFSET;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE;
        p = p_buf->data;
        UINT16_TO_STREAM(p, HCI_RESET);
        *p = 0; // parameter length
        hw_cfg_cb.state = HW_CFG_START;
        bt_vendor_cbacks->xmit_cb(HCI_RESET, p_buf, hw_config_cback);
    } else {
        if (bt_vendor_cbacks) {
            ALOGE("vendor lib fw conf aborted [no buffer]");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }
    }
}

static uint8_t hw_config_set_bdaddr(HC_BT_HDR* p_buf) {
    uint8_t retval = STATUS_FAIL;
    uint8_t* p = p_buf->data;
    ALOGI("Setting local bd addr to %02X:%02X:%02X:%02X:%02X:%02X",
          vnd_local_bd_addr[0], vnd_local_bd_addr[1], vnd_local_bd_addr[2],
          vnd_local_bd_addr[3], vnd_local_bd_addr[4], vnd_local_bd_addr[5]);
    UINT16_TO_STREAM(p, HCI_VSC_WRITE_BD_ADDR);
    *p++ = BD_ADDR_LEN; // parameter length
    *p++ = vnd_local_bd_addr[5];
    *p++ = vnd_local_bd_addr[4];
    *p++ = vnd_local_bd_addr[3];
    *p++ = vnd_local_bd_addr[2];
    *p++ = vnd_local_bd_addr[1];
    *p = vnd_local_bd_addr[0];
    p_buf->len = HCI_CMD_PREAMBLE_SIZE + BD_ADDR_LEN;
    hw_cfg_cb.state = HW_CFG_SET_BD_ADDR;
    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_WRITE_BD_ADDR, p_buf,
                                       hw_config_cback);
    return retval;
}

#if (USE_CONTROLLER_BDADDR == TRUE)
static uint8_t hw_config_read_bdaddr(HC_BT_HDR* p_buf) {
    uint8_t retval = STATUS_FAIL;
    uint8_t* p = p_buf->data;
    UINT16_TO_STREAM(p, HCI_READ_LOCAL_BDADDR);
    *p = 0; // parameter length
    p_buf->len = HCI_CMD_PREAMBLE_SIZE;
    hw_cfg_cb.state = HW_CFG_READ_BD_ADDR;
    retval = bt_vendor_cbacks->xmit_cb(HCI_READ_LOCAL_BDADDR, p_buf,
                                       hw_config_cback);
    return retval;
}
#endif // (USE_CONTROLLER_BDADDR == TRUE)

static uint8_t hw_download_firmware_helper(void) {
    uint8_t retval = STATUS_SUCCESS;
    bts_action_t* cur_action = (bts_action_t*)hw_cfg_cb.cur_action;

    while (hw_cfg_cb.len > 0 && hw_cfg_cb.cur_action &&
           cur_action->type != ACTION_SEND_COMMAND) {
        hw_cfg_cb.len =
            hw_cfg_cb.len - (long)(bts_action_t_size +
                             cur_action->size);
        hw_cfg_cb.cur_action =
            hw_cfg_cb.cur_action + bts_action_t_size +
            cur_action->size;
        cur_action = (bts_action_t*)hw_cfg_cb.cur_action;
    }

    if (cur_action->type == ACTION_SEND_COMMAND) {
        ALOGV(" action size %d, type %d ",
              cur_action->size,
              cur_action->type);
        hci_command_t* action_command = (hci_command_t*)(&(cur_action->data[0]));

        if (action_command->opcode == HCI_VSC_UPDATE_BAUDRATE) {
            // ignore remote change
            // baud rate HCI VS command
            ALOGW("change remote baud"
                  " rate command in firmware");
            skip_hci_command(&hw_cfg_cb.cur_action, &hw_cfg_cb.len);
            retval = STATUS_FAIL;
        } else if (action_command->opcode == HCI_VS_SLP_CFG) {
            // ignore remote change
            // sleep mode HCI VS command
            ALOGW("skipping"
                  " sleep mode configuration");
            skip_hci_command(&hw_cfg_cb.cur_action, &hw_cfg_cb.len);
            retval = STATUS_FAIL;
        } else {
            retval = STATUS_SUCCESS;
        }
    }

    return retval;
}

static uint8_t hw_download_firmware(HC_BT_HDR* p_buf) {
    uint8_t retval = STATUS_FAIL;

    while (hw_download_firmware_helper() != STATUS_SUCCESS) {
        ;
    }

    bts_action_t* cur_action = (bts_action_t*)hw_cfg_cb.cur_action;

    if (cur_action->type == ACTION_SEND_COMMAND) {
        ALOGV(" action size %d, type %d ",
              cur_action->size,
              cur_action->type);
        uint8_t* p = p_buf->data;
        hci_command_t* action_command = (hci_command_t*)(&(cur_action->data[0]));
        UINT16_TO_STREAM(p, action_command->opcode);
        *p++ = action_command->plen;
        memcpy(p, action_command->data,
               action_command->plen);
        hw_cfg_cb.len =
            hw_cfg_cb.len - (long)(bts_action_t_size +
                             cur_action->size);
        hw_cfg_cb.cur_action =
            hw_cfg_cb.cur_action + bts_action_t_size +
            cur_action->size;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE +
                     action_command->plen;
        retval = bt_vendor_cbacks->xmit_cb(action_command->opcode,
                                           p_buf, hw_config_cback);
    }

    return retval;
}


void hw_config_cback(void* p_mem) {
    HC_BT_HDR* p_evt_buf = (HC_BT_HDR*)p_mem;
    uint16_t opcode = 0;
    HC_BT_HDR* p_buf = NULL;
    uint8_t is_proceeding = 0;
    uint8_t status = *(p_evt_buf->data +
                       HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    uint8_t* p = p_evt_buf->data + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode, p);

    // Ask a new buffer big enough to hold any HCI commands sent in here
    if ((status == 0) && bt_vendor_cbacks) {
        p_buf = (HC_BT_HDR*)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE +
                HCI_CMD_MAX_LEN);
    }

    if (p_buf != NULL) {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = ZERO_OFFSET;
        p_buf->len = 0;
        p_buf->layer_specific = 0;
        p = p_buf->data;

        switch (hw_cfg_cb.state) {
        case HW_CFG_START: {
            ALOGD("Setting speed to %d", UART_TARGET_BAUD_RATE);
            // set controller's UART baud rate to 3M
            UINT16_TO_STREAM(p, HCI_VSC_UPDATE_BAUDRATE);
            *p++ = UPDATE_BAUDRATE_CMD_PARAM_SIZE; // parameter length
            UINT32_TO_STREAM(p, UART_TARGET_BAUD_RATE);
            p_buf->len = HCI_CMD_PREAMBLE_SIZE +
                         UPDATE_BAUDRATE_CMD_PARAM_SIZE;
            hw_cfg_cb.state = HW_CFG_SET_UART_BAUD;
            is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_VSC_UPDATE_BAUDRATE,
                            p_buf, hw_config_cback);
            break;
        }

        case HW_CFG_SET_UART_BAUD: {
            // update baud rate of host's UART port
            ALOGI("bt vendor lib: set UART baud %i", UART_TARGET_BAUD_RATE);
            userial_vendor_set_baud(UART_TARGET_BAUD_RATE, UART_TARGET_FLOW_CNTRL);
#if (USE_CONTROLLER_BDADDR == TRUE)

            if ((is_proceeding = hw_config_read_bdaddr(p_buf)) != STATUS_FAIL) {
                break;
            }

#else

            if ((is_proceeding = hw_config_set_bdaddr(p_buf)) != STATUS_FAIL) {
                break;
            }

#endif
        }

#if (USE_CONTROLLER_BDADDR == TRUE)

        case HW_CFG_READ_BD_ADDR: {
            uint8_t* p_tmp = p_evt_buf->data +
                             HCI_EVT_CMD_CMPL_LOCAL_BDADDR_ARRAY;
            const uint8_t null_bdaddr[BD_ADDR_LEN] = {0, 0, 0, 0, 0, 0};

            if (memcmp(p_tmp, null_bdaddr, BD_ADDR_LEN) == 0) {
                // Controller does not have a valid OTP BDADDR!
                // Set the BTIF initial BDADDR instead.
                if ((is_proceeding = hw_config_set_bdaddr(p_buf)) != STATUS_FAIL) {
                    break;
                }
            } else {
                ALOGI("Controller OTP bdaddr %02X:%02X:%02X:%02X:%02X:%02X",
                      p_tmp[5], p_tmp[4], p_tmp[3],
                      p_tmp[2], p_tmp[1], p_tmp[0]);
            }
        }

#endif

        case HW_CFG_SET_BD_ADDR: {
            is_proceeding = hw_config_read_local_version(p_buf);
            break;
        }

        // fall through intentionally
        case HW_CFG_READ_LOCAL_VER: {
            uint8_t* p_tmp = p_evt_buf->data;
            // the positions 12 & 13 in the response buffer provide with the
            //chip, major & minor numbers
            uint16_t version = MAKEWORD(p_tmp[12], p_tmp[13]);
            uint16_t chip = (version & MASK_CHIP) >> OFFSET_CHIP;
            uint16_t min_ver = (version & MASK_MIN_VER);
            uint16_t maj_ver = (version & MASK_MAJ_VER) >> OFFSET_MAJ_VER;

            if (version & MAJ_VER_CARRY_BIT) {
                maj_ver |= (1 << 3);
            }

            char bts_src_filename[FILENAME_MAX_LEN];
            sprintf(bts_src_filename,
                    "/vendor/etc/firmware/ti-connectivity/TIInit_%d.%d.%d.bts",
                    chip, maj_ver, min_ver);
            ALOGI("firmware file: %s", bts_src_filename);

            if ((is_proceeding = hw_config_load_bts(bts_src_filename)) == STATUS_FAIL) {
                ALOGE("BTS loading failed");
                break;
            }
        }

        case HW_CFG_DOWNLOAD: {
            ALOGV("HW_CFG_DOWNLOAD");

            if ((is_proceeding = hw_download_firmware(p_buf)) != STATUS_FAIL) {
                break;
            } else {
                hw_cfg_cb.state = HW_CFG_DOWNLOAD_END;
            }
        }

        case HW_CFG_DOWNLOAD_END: {
            ALOGV("HW_CFG_DOWNLOAD_END");
            bt_vendor_cbacks->dealloc(p_buf);
            free(hw_cfg_cb.fw_data);
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
            hw_cfg_cb.state = 0;
            is_proceeding = STATUS_SUCCESS;
            break;
        }
        } // switch(hw_cfg_cb.state)
    } // if (p_buf != NULL)

    // Free the RX event buffer
    if (bt_vendor_cbacks) {
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }

    if (is_proceeding == STATUS_FAIL) {
        ALOGE("vendor lib fwcfg aborted!!!");

        if (bt_vendor_cbacks) {
            if (p_buf != NULL) {
                bt_vendor_cbacks->dealloc(p_buf);
            }

            free(hw_cfg_cb.fw_data);
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }

        hw_cfg_cb.state = 0;
    }
}

void hw_configure_sco(void) {
    hci_vs_write_codec_configuration();
    hci_vs_write_codec_configuration_enhanced();
    hci_vs_write_sco_configuration();
    if (bt_vendor_cbacks) {
        bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);
    }
}

static void hci_vs_write_sco_configuration(void) {
    HC_BT_HDR* p_buf = hc_allocate_buffer();
    if (p_buf) {
        hc_fill_hci_cmd_preamble(p_buf);
        hc_fill_buffer_hci_vs_write_sco_config(p_buf);
        hci_send(HCI_VS_WRITE_SCO_CONFIG, p_buf, hci_free_buffer);
    } else {
        abort_sco_configuration();
    }
}

static void hci_vs_write_codec_configuration(void) {
    HC_BT_HDR* p_buf = hc_allocate_buffer();
    if (p_buf) {
        hc_fill_hci_cmd_preamble(p_buf);
        hc_fill_buffer_hci_vs_write_codec_config(p_buf);
        hci_send(HCI_VS_WRITE_CODEC_CONFIG, p_buf, hci_free_buffer);
    } else {
        abort_sco_configuration();
    }
}

static void hci_vs_write_codec_configuration_enhanced(void) {
    HC_BT_HDR* p_buf = hc_allocate_buffer();
    if (p_buf) {
        hc_fill_hci_cmd_preamble(p_buf);
        hc_fill_buffer_hci_vs_write_codec_config_enchanced(p_buf);
        hci_send(HCI_VS_WRITE_CODEC_CONFIG_ENCHANCED, p_buf, hci_free_buffer);
    } else {
        abort_sco_configuration();
    }
}

static HC_BT_HDR* hc_allocate_buffer(void) {
    HC_BT_HDR* buffer = NULL;
    if (bt_vendor_cbacks) {
        buffer = (HC_BT_HDR*)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_MAX_LEN);
    } else {
        ALOGE("Failed to allocate HC buffer");
    }
    return buffer;
}

static void hc_fill_hci_cmd_preamble(HC_BT_HDR* p_buf) {
    p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
    p_buf->offset = ZERO_OFFSET;
    p_buf->layer_specific = 0;
    p_buf->len = HCI_CMD_PREAMBLE_SIZE;
}

static void hc_fill_buffer_hci_vs_write_sco_config(HC_BT_HDR* p_buf) {
    p_buf->len += HCIC_PARAM_SIZE_SCO_CFG;
    uint8_t* p = p_buf->data;
    UINT16_TO_STREAM(p, HCI_VS_WRITE_SCO_CONFIG);
    *p++ = HCIC_PARAM_SIZE_SCO_CFG; // parameter length
    memset(p, 0, HCIC_PARAM_SIZE_SCO_CFG);
}

static void hc_fill_buffer_hci_vs_write_codec_config(HC_BT_HDR* p_buf) {
    p_buf->len += HCIC_PARAM_SIZE_CODEC_CFG;
    uint8_t* p = p_buf->data;
    UINT16_TO_STREAM(p, HCI_VS_WRITE_CODEC_CONFIG);
    *p++ = HCIC_PARAM_SIZE_CODEC_CFG; // parameter length

    UINT16_TO_STREAM(p, 2048);
    UINT8_TO_STREAM(p, 0x01);      // PCM slave
    UINT32_TO_STREAM(p, 16000);    // Frame-sync frequency
    UINT16_TO_STREAM(p, 0x0001);   // Frame-sync duty cycle
    UINT8_TO_STREAM(p, 0x00);      // Frame-sync edge
    UINT8_TO_STREAM(p, 0x00);      // Frame-sync polarity
    UINT8_TO_STREAM(p, 0x00);      // Reserved
    UINT16_TO_STREAM(p, 0x0010);   // Channel 1 data out size bits
    UINT16_TO_STREAM(p, 0x0001);   // Channel 1 data out offset
    UINT8_TO_STREAM(p, 0x01);      // Channel 1 data out edge
    UINT16_TO_STREAM(p, 0x0010);   // Channel 1 data in size bits
    UINT16_TO_STREAM(p, 0x0001);   // Channel 1 data in offset
    UINT8_TO_STREAM(p, 0x00);      // Channel 1 data in edge
    UINT8_TO_STREAM(p, 0x00);      // Reserved
    UINT16_TO_STREAM(p, 0x0010);   // Channel 2 data out size bits
    UINT16_TO_STREAM(p, 0x0011);   // Channel 2 data out offset
    UINT8_TO_STREAM(p, 0x01);      // Channel 2 data out edge
    UINT16_TO_STREAM(p, 0x0010);   // Channel 2 data in size bits
    UINT16_TO_STREAM(p, 0x0011);   // Channel 2 data in offset
    UINT8_TO_STREAM(p, 0x00);      // Channel 2 data in edge
    UINT8_TO_STREAM(p, 0x00);      // Reserved
}

static void hc_fill_buffer_hci_vs_write_codec_config_enchanced(HC_BT_HDR* p_buf) {
    p_buf->len += HCIC_PARAM_SIZE_CODEC_CFG_ENHANCED;
    uint8_t* p = p_buf->data;
    UINT16_TO_STREAM(p, HCI_VS_WRITE_CODEC_CONFIG_ENCHANCED);
    *p++ = HCIC_PARAM_SIZE_CODEC_CFG_ENHANCED; // parameter length

    UINT8_TO_STREAM(p, 0x00);        // PCM clock shutdown
    UINT16_TO_STREAM(p, 0x0000);     // PCM clock start
    UINT16_TO_STREAM(p, 0x0000);     // PCM clock stop
    UINT8_TO_STREAM(p, 0x00);        // Reserved
    UINT8_TO_STREAM(p, 0x04);        // Channel 1 data in order
    UINT8_TO_STREAM(p, 0x04);        // Channel 1 data out order
    UINT8_TO_STREAM(p, 0x01);        // Channel 1 data out mode
    UINT8_TO_STREAM(p, 0x00);        // Channel 1 data out duplication
    UINT32_TO_STREAM(p, 0x00000000); // Channel 1 TX_dup_value
    UINT8_TO_STREAM(p, 0x00);        // Channel 1 data quant
    UINT8_TO_STREAM(p, 0x00);        // Reserved
    UINT8_TO_STREAM(p, 0x04);        // Channel 2 data in order
    UINT8_TO_STREAM(p, 0x04);        // Channel 2 data out order
    UINT8_TO_STREAM(p, 0x01);        // Channel 2 data out mode
    UINT8_TO_STREAM(p, 0x00);        // Channel 2 data out duplication
    UINT32_TO_STREAM(p, 0x00000000); // Channel 2 TX_dup_value
    UINT8_TO_STREAM(p, 0x00);        // Channel data quant
    UINT8_TO_STREAM(p, 0x00);        // Reserved
}

static void hci_send(int command, HC_BT_HDR* p_buf, tINT_CMD_CBACK callback) {
    if (bt_vendor_cbacks) {
        bt_vendor_cbacks->xmit_cb(command, p_buf, callback);
    } else {
        ALOGE("Failed to send HCI command");
    }
}

static void hci_free_buffer(void* p_mem) {
    HC_BT_HDR* p_evt_buf = (HC_BT_HDR*)p_mem;
    if (bt_vendor_cbacks) {
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }
}

static void abort_sco_configuration(void) {
    ALOGE("vendor lib sco conf aborted");
    if (bt_vendor_cbacks) {
        bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_FAIL);
    }
}
