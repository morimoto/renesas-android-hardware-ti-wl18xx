/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef WIFI_H_
#define WIFI_H_

#include <functional>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <android-base/macros.h>
#include <android/hardware/wifi/1.3/IWifi.h>
#include <utils/Looper.h>
#include <wifi_hal.h>

#include "hidl_callback_util.h"
#include "wifi_chip.h"
#include "wifi_feature_flags.h"

#include "wifi_mode_controller.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace ti {

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <linux/nl80211.h>

static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
    [NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
    [NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
    [NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
    [NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
    [NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
    [NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
    [NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
    [NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
    [NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
    [NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
};

static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
    [NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
    [NL80211_RATE_INFO_MCS] = { .type = NLA_U8 },
    [NL80211_RATE_INFO_40_MHZ_WIDTH] = { .type = NLA_FLAG },
    [NL80211_RATE_INFO_SHORT_GI] = { .type = NLA_FLAG },
};

/**
 * Root HIDL interface object used to control the Wifi HAL.
 */
class Wifi : public V1_3::IWifi {
public:
    Wifi(const std::shared_ptr<wifi_system::InterfaceTool> iface_tool,
         const std::shared_ptr<mode_controller::WifiModeController>
                     mode_controller,
         const std::shared_ptr<iface_util::WifiIfaceUtil> iface_util,
         const std::shared_ptr<feature_flags::WifiFeatureFlags> feature_flags);

    bool isValid();

    // HIDL methods exposed.
    Return<void> registerEventCallback(
        const sp<IWifiEventCallback>& event_callback,
        registerEventCallback_cb hidl_status_cb) override;
    Return<bool> isStarted() override;
    Return<void> start(start_cb hidl_status_cb) override;
    Return<void> stop(stop_cb hidl_status_cb) override;
    Return<void> getChipIds(getChipIds_cb hidl_status_cb) override;
    Return<void> getChip(ChipId chip_id, getChip_cb hidl_status_cb) override;
    Return<void> debug(const hidl_handle& handle,
                       const hidl_vec<hidl_string>& options) override;

private:
    enum class RunState { STOPPED, STARTED, STOPPING };

    // Corresponding worker functions for the HIDL methods.
    WifiStatus registerEventCallbackInternal(
        const sp<IWifiEventCallback>& event_callback);
    WifiStatus startInternal();
    WifiStatus stopInternal(std::unique_lock<std::recursive_mutex>* lock);
    std::pair<WifiStatus, std::vector<ChipId>> getChipIdsInternal();
    std::pair<WifiStatus, sp<IWifiChip>> getChipInternal(ChipId chip_id);

    WifiStatus initializeModeController();
    WifiStatus stopAndDeinitializeModeController(
        std::unique_lock<std::recursive_mutex>* lock);

    // Instance is created in this root level |IWifi| HIDL interface object
    // and shared with all the child HIDL interface objects.
    std::shared_ptr<wifi_system::InterfaceTool> iface_tool_;
    std::shared_ptr<mode_controller::WifiModeController> mode_controller_;
    std::shared_ptr<iface_util::WifiIfaceUtil> iface_util_;
    std::shared_ptr<feature_flags::WifiFeatureFlags> feature_flags_;
    RunState run_state_;
    sp<WifiChip> chip_;
    hidl_callback_util::HidlCallbackHandler<IWifiEventCallback>
        event_cb_handler_;

    struct nl_sock* control_socket;
    std::vector<struct nl_cb*> nl_callbacks;
    int id;

    DISALLOW_COPY_AND_ASSIGN(Wifi);
};

}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_H_
