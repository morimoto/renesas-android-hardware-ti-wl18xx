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

#include <android-base/logging.h>
#include <sys/socket.h>
#include <linux/nl80211.h>      //NL80211 definitions
#include <stdbool.h>
#include <errno.h>
#include <net/if.h>

#include "hidl_return_util.h"
#include "wifi_ap_iface.h"
#include "wifi_status_util.h"
#include "wifi_iface_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace ti {
using hidl_return_util::validateAndCall;

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

WifiApIface::WifiApIface(
    const std::string& ifname,
    const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util,
    const std::weak_ptr<feature_flags::WifiFeatureFlags> feature_flags,
    nl_sock* control_socket, int _family_id)
    : ifname_(ifname),
      iface_util_(iface_util),
      feature_flags_(feature_flags),
      is_valid_(true),
      control_socket_(control_socket),
      family_id(_family_id) {
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    nl_socket_set_cb(control_socket_, cb);

    if (feature_flags_.lock()->isApMacRandomizationDisabled()) {
        LOG(INFO) << "AP MAC randomization disabled";
        return;
    }
    LOG(INFO) << "AP MAC randomization enabled";
    // Set random MAC address
    std::array<uint8_t, 6> randomized_mac =
        iface_util_.lock()->getOrCreateRandomMacAddress();
    bool status = iface_util_.lock()->setMacAddress(ifname_, randomized_mac);
    if (!status) {
        LOG(ERROR) << "Failed to set random mac address";
    }
}

void WifiApIface::invalidate() {
    is_valid_ = false;
    nl_cb_put(cb);
}

bool WifiApIface::isValid() {
    return is_valid_;
}

std::string WifiApIface::getName() {
    return ifname_;
}

Return<void> WifiApIface::getName(getName_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getNameInternal, hidl_status_cb);
}

Return<void> WifiApIface::getType(getType_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getTypeInternal, hidl_status_cb);
}

Return<void> WifiApIface::setCountryCode(const hidl_array<int8_t, 2>& code,
                                         setCountryCode_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::setCountryCodeInternal, hidl_status_cb,
                           code);
}

Return<void> WifiApIface::getValidFrequenciesForBand(
    WifiBand band, getValidFrequenciesForBand_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getValidFrequenciesForBandInternal,
                           hidl_status_cb, band);
}

std::pair<WifiStatus, std::string> WifiApIface::getNameInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiApIface::getTypeInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::AP};
}

WifiStatus WifiApIface::setCountryCodeInternal(
    const std::array<int8_t, 2>& code) {
    return iface_util_.lock()->setCountryCode(code, family_id, control_socket_);
}

std::pair<WifiStatus, std::vector<WifiChannelInMhz>>
WifiApIface::getValidFrequenciesForBandInternal(WifiBand band) {
    return iface_util_.lock()->getValidFrequenciesForBand(band, family_id, control_socket_, cb);
}

}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
