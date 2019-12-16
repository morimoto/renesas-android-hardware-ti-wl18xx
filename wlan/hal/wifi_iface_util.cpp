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

#include <cstddef>
#include <iostream>
#include <limits>
#include <random>

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <private/android_filesystem_config.h>

#undef NAN
#include "wifi_iface_util.h"
#include "hidl_return_util.h"
#include "wifi_status_util.h"

namespace {
// Constants to set the local bit & clear the multicast bit.
constexpr uint8_t kMacAddressMulticastMask = 0x01;
constexpr uint8_t kMacAddressLocallyAssignedMask = 0x02;
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace ti {
namespace iface_util {
using namespace android::hardware::wifi::V1_0;

bool is_band_ok(uint32_t* band, uint32_t freq)
{
    /* see 802.11-2007 17.3.8.3.2 and Annex J */
    if (*band == 1 && freq <= 2484 && freq >=2407) {
        return true;
    }
    return false;
}

int getValidFrequenciesForBandInternalCallback(struct nl_msg* msg, void* arg)
{
    ap_handler_params* params = reinterpret_cast<ap_handler_params*>(arg);
    nlattr* tb_msg[NL80211_ATTR_MAX + 1];
    genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));

    nlattr* tb_band[NL80211_BAND_ATTR_MAX + 1];
    nlattr* tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
    static nla_policy freq_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
        [NL80211_FREQUENCY_ATTR_FREQ] = { .type = NLA_U32 },
        [NL80211_FREQUENCY_ATTR_DISABLED] = { .type = NLA_FLAG },
        [NL80211_FREQUENCY_ATTR_NO_IR] = { .type = NLA_FLAG },
        [__NL80211_FREQUENCY_ATTR_NO_IBSS] = { .type = NLA_FLAG },
        [NL80211_FREQUENCY_ATTR_RADAR] = { .type = NLA_FLAG },
        [NL80211_FREQUENCY_ATTR_MAX_TX_POWER] = { .type = NLA_U32 },
    };

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

    /* needed for split dump */
    if (tb_msg[NL80211_ATTR_WIPHY_BANDS]) {
        nlattr* nl_band = reinterpret_cast<nlattr*>(nla_data(tb_msg[NL80211_ATTR_WIPHY_BANDS]));
        int rem_band = nla_len(tb_msg[NL80211_ATTR_WIPHY_BANDS]);
        for (; nla_ok(nl_band, rem_band); nl_band = nla_next(nl_band, &(rem_band))) {
            nla_parse(tb_band, NL80211_BAND_ATTR_MAX,
                reinterpret_cast<nlattr*>(nla_data(nl_band)),
                    nla_len(nl_band), NULL);

            if (tb_band[NL80211_BAND_ATTR_FREQS]) {
                nlattr* nl_freq = reinterpret_cast<nlattr*>(nla_data(tb_band[NL80211_BAND_ATTR_FREQS]));
                int rem_freq = nla_len(tb_band[NL80211_BAND_ATTR_FREQS]);
                for (; nla_ok(nl_freq, rem_freq); nl_freq = nla_next(nl_freq, &(rem_freq))) {
                    nla_parse(tb_freq, NL80211_FREQUENCY_ATTR_MAX,
                        reinterpret_cast<nlattr*>(nla_data(nl_freq)),
                            nla_len(nl_freq), freq_policy);

                    if (!tb_freq[NL80211_FREQUENCY_ATTR_FREQ]) {
                        continue;
                    }

                    uint32_t freq = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);
                    if (!is_band_ok(params->band, freq)) {
                        break;
                    }

                    if (!tb_freq[NL80211_FREQUENCY_ATTR_DISABLED]) {
                        params->freqencies->push_back(freq);
                    }
                }
            }
        }
    }

    return NL_SKIP;
}

WifiIfaceUtil::WifiIfaceUtil(
    const std::weak_ptr<wifi_system::InterfaceTool> iface_tool)
    : iface_tool_(iface_tool),
      random_mac_address_(nullptr),
      event_handlers_map_() {}

std::array<uint8_t, 6> WifiIfaceUtil::getFactoryMacAddress(
    const std::string& iface_name) {
    return iface_tool_.lock()->GetFactoryMacAddress(iface_name.c_str());
}

bool WifiIfaceUtil::setMacAddress(const std::string& iface_name,
                                  const std::array<uint8_t, 6>& mac) {
    if (!iface_tool_.lock()->SetUpState(iface_name.c_str(), false)) {
        LOG(ERROR) << "SetUpState(false) failed.";
        return false;
    }
    if (!iface_tool_.lock()->SetMacAddress(iface_name.c_str(), mac)) {
        LOG(ERROR) << "SetMacAddress failed.";
        return false;
    }
    if (!iface_tool_.lock()->SetUpState(iface_name.c_str(), true)) {
        LOG(ERROR) << "SetUpState(true) failed.";
        return false;
    }
    IfaceEventHandlers event_handlers = {};
    const auto it = event_handlers_map_.find(iface_name);
    if (it != event_handlers_map_.end()) {
        event_handlers = it->second;
    }
    if (event_handlers.on_state_toggle_off_on != nullptr) {
        event_handlers.on_state_toggle_off_on(iface_name);
    }
    LOG(DEBUG) << "Successfully SetMacAddress.";
    return true;
}

std::array<uint8_t, 6> WifiIfaceUtil::getOrCreateRandomMacAddress() {
    if (random_mac_address_) {
        return *random_mac_address_.get();
    }
    random_mac_address_ =
        std::make_unique<std::array<uint8_t, 6>>(createRandomMacAddress());
    return *random_mac_address_.get();
}

void WifiIfaceUtil::registerIfaceEventHandlers(const std::string& iface_name,
                                               IfaceEventHandlers handlers) {
    event_handlers_map_[iface_name] = handlers;
}

void WifiIfaceUtil::unregisterIfaceEventHandlers(
    const std::string& iface_name) {
    event_handlers_map_.erase(iface_name);
}

std::array<uint8_t, 6> WifiIfaceUtil::createRandomMacAddress() {
    std::array<uint8_t, 6> address = {};
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_int_distribution<uint8_t> dist(
        std::numeric_limits<uint8_t>::min(),
        std::numeric_limits<uint8_t>::max());
    for (size_t i = 0; i < address.size(); i++) {
        address[i] = dist(engine);
    }
    // Set the local bit and clear the multicast bit.
    address[0] |= kMacAddressLocallyAssignedMask;
    address[0] &= ~kMacAddressMulticastMask;
    return address;
}

std::pair<WifiStatus, std::vector<WifiChannelInMhz>>
WifiIfaceUtil::getValidFrequenciesForBand(WifiBand band, int family_id,
                                        nl_sock* control_socket_, nl_cb* cb) {
    static_assert(sizeof(WifiChannelInMhz) == sizeof(uint32_t),
                         "Size mismatch");
    std::vector<uint32_t> valid_frequencies;
    std::pair<WifiStatus, std::vector<WifiChannelInMhz>> retval
        {createWifiStatus(WifiStatusCode::ERROR_UNKNOWN), {}};

    ap_handler_params param {reinterpret_cast<uint32_t*>(&band),
        &valid_frequencies};
    nl_msg* msg = nlmsg_alloc();
    if (msg == nullptr) {
        goto ALLOC_ERROR;
    }

    if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, NLM_F_DUMP,
                     NL80211_CMD_GET_WIPHY, 0)) {
        goto ERROR;
    }

    if (nl_send_auto(control_socket_, msg) < 0) {
        goto ERROR;
    }
    (void)getValidFrequenciesForBandInternalCallback;
    if (nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
              getValidFrequenciesForBandInternalCallback,
              reinterpret_cast<void*>(&param)) != 0) {
        goto ERROR;
    }
    if (nl_recvmsgs(control_socket_, cb) < 0) {
        goto ERROR;
    }

    retval = {createWifiStatus(WifiStatusCode::SUCCESS), valid_frequencies};

ERROR:
    nlmsg_free(msg);
ALLOC_ERROR:
    return retval;
}

WifiStatus WifiIfaceUtil::setCountryCode(
        const std::array<int8_t, 2>& code, int family_id,
        nl_sock* control_socket_) {
    WifiStatus retval = createWifiStatus(WifiStatusCode::SUCCESS);
    std::string code_str(code.data(), code.data() + code.size());
    nl_msg* msg = nlmsg_alloc();
    if (msg == nullptr) {
        goto ALLOC_ERROR;
    }

    if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, NLM_F_DUMP,
                NL80211_CMD_REQ_SET_REG, 0)) {
        goto ERROR;
    }

    if (nla_put_string(msg, NL80211_ATTR_REG_ALPHA2, code_str.c_str()) != 0) {
        goto ERROR;
    }

    if (nl_send_auto(control_socket_, msg) < 0) {
        goto ERROR;
    }

    retval = createWifiStatus(WifiStatusCode::SUCCESS);
ERROR:
    nlmsg_free(msg);
ALLOC_ERROR:
    return retval;
}

}  // namespace iface_util
}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
