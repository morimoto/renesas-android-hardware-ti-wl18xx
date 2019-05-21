/*
* Copyright (C) 2019 GlobalLogic
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "driver_nl80211.h"
#include "linux_ioctl.h"


int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf, size_t buf_len)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_nl80211_data *drv = bss->drv;
	int ret = -1;
	if(os_strncasecmp(cmd, "MACADDR", 7) == 0) {
		u8 macaddr[ETH_ALEN] = {};
		ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
		if (!ret)
			ret = os_snprintf(buf, buf_len,
				"Macaddr = " MACSTR "\n", MAC2STR(macaddr));
	} else {
		wpa_printf(MSG_ERROR, "nl80211: unhandled command: %s", cmd);
	}
	return ret;
}


int wpa_driver_set_p2p_noa(
		__attribute__((__unused__)) void* priv,
		__attribute__((__unused__)) u8 count,
		__attribute__((__unused__)) int start,
		__attribute__((__unused__)) int duration) {
	wpa_printf(MSG_DEBUG, "%s: called.", __FUNCTION__);
	return 0;
}


int wpa_driver_get_p2p_noa(
		__attribute__((__unused__)) void* priv,
		__attribute__((__unused__)) u8* buf,
		__attribute__((__unused__)) size_t len) {
	wpa_printf(MSG_DEBUG, "%s: called.", __FUNCTION__);
	return 0;
}


int wpa_driver_set_p2p_ps(
		__attribute__((__unused__)) void* priv,
		__attribute__((__unused__)) int legacy_ps,
		__attribute__((__unused__)) int opp_ps,
		__attribute__((__unused__)) int ctwindow) {
	wpa_printf(MSG_DEBUG, "%s: called.", __FUNCTION__);
	return -1;
}


int wpa_driver_set_ap_wps_p2p_ie(
		__attribute__((__unused__)) void* priv,
		__attribute__((__unused__)) const struct wpabuf* beacon,
		__attribute__((__unused__)) const struct wpabuf* proberesp,
		__attribute__((__unused__)) const struct wpabuf* assocresp) {
	wpa_printf(MSG_DEBUG, "%s: called.", __FUNCTION__);
	return 0;
}
