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

struct ap_handler_params
{
    uint32_t* band;
    std::vector<uint32_t>* freqencies;
};

static bool is_band_ok(uint32_t* band, uint32_t freq)
{
    /* see 802.11-2007 17.3.8.3.2 and Annex J */
    if (*band == 1 && freq <= 2484 && freq >=2407) {
        return true;
    }
    return false;
}

static int getValidFrequenciesForBandInternalCallback(struct nl_msg* msg, void* arg)
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

std::pair<WifiStatus, std::vector<WifiChannelInMhz>>
WifiApIface::getValidFrequenciesForBandInternal(WifiBand band) {
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

}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
