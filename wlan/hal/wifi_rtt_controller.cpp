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

#include "hidl_return_util.h"
#include "wifi_rtt_controller.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace ti {
using hidl_return_util::validateAndCall;

WifiRttController::WifiRttController(
    const std::string& iface_name, const sp<IWifiIface>& bound_iface)
    : ifname_(iface_name),
      bound_iface_(bound_iface),
      is_valid_(true) {}

void WifiRttController::invalidate() {
    event_callbacks_.clear();
    is_valid_ = false;
}

bool WifiRttController::isValid() {
    return is_valid_;
}

std::vector<sp<IWifiRttControllerEventCallback>>
WifiRttController::getEventCallbacks() {
    return event_callbacks_;
}

std::string WifiRttController::getIfaceName() {
    return ifname_;
}

Return<void> WifiRttController::getBoundIface(getBoundIface_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getBoundIfaceInternal, hidl_status_cb);
}

Return<void> WifiRttController::registerEventCallback(
    const sp<IWifiRttControllerEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this,
        WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::registerEventCallbackInternal,
        hidl_status_cb, callback);
}

Return<void> WifiRttController::rangeRequest(
    uint32_t cmd_id, const hidl_vec<RttConfig>& rtt_configs,
    rangeRequest_cb hidl_status_cb) {
    return validateAndCall(this,
        WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::rangeRequestInternal,
        hidl_status_cb, cmd_id, rtt_configs);
}

Return<void> WifiRttController::rangeCancel(
    uint32_t cmd_id, const hidl_vec<hidl_array<uint8_t, 6>>& addrs,
    rangeCancel_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::rangeCancelInternal, hidl_status_cb, cmd_id, addrs);
}

Return<void> WifiRttController::getCapabilities(
    getCapabilities_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getCapabilitiesInternal, hidl_status_cb);
}

Return<void> WifiRttController::setLci(uint32_t cmd_id,
    const RttLciInformation& lci, setLci_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::setLciInternal, hidl_status_cb, cmd_id, lci);
}

Return<void> WifiRttController::setLcr(uint32_t cmd_id,
    const RttLcrInformation& lcr, setLcr_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::setLcrInternal, hidl_status_cb, cmd_id, lcr);
}

Return<void> WifiRttController::getResponderInfo(
    getResponderInfo_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getResponderInfoInternal, hidl_status_cb);
}

Return<void> WifiRttController::enableResponder(
    uint32_t cmd_id, const WifiChannelInfo& channel_hint,
    uint32_t max_duration_seconds, const RttResponder& info,
    enableResponder_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::enableResponderInternal, hidl_status_cb, cmd_id,
        channel_hint, max_duration_seconds, info);
}

Return<void> WifiRttController::disableResponder(
    uint32_t cmd_id, disableResponder_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::disableResponderInternal, hidl_status_cb, cmd_id);
}

std::pair<WifiStatus, sp<IWifiIface>>
WifiRttController::getBoundIfaceInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), bound_iface_};
}

WifiStatus WifiRttController::registerEventCallbackInternal(
    const sp<IWifiRttControllerEventCallback>& callback) {
    // TODO(b/31632518): remove the callback when the client is destroyed
    event_callbacks_.emplace_back(callback);
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiRttController::rangeRequestInternal(
    uint32_t /*cmd_id*/, const std::vector<RttConfig>& /*rtt_configs*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiRttController::rangeCancelInternal(
    uint32_t /*cmd_id*/, const std::vector<hidl_array<uint8_t, 6>>& /*addrs*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<WifiStatus, RttCapabilities>
WifiRttController::getCapabilitiesInternal() {
    RttCapabilities hidl_caps;
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), hidl_caps};
}

WifiStatus WifiRttController::setLciInternal(uint32_t /*cmd_id*/,
    const RttLciInformation& /*lci*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiRttController::setLcrInternal(uint32_t /*cmd_id*/,
    const RttLcrInformation& /*lcr*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<WifiStatus, RttResponder>
WifiRttController::getResponderInfoInternal() {
    RttResponder hidl_responder;
    return {createWifiStatus(WifiStatusCode::SUCCESS), hidl_responder};
}

WifiStatus WifiRttController::enableResponderInternal(
    uint32_t /*cmd_id*/, const WifiChannelInfo& /*channel_hint*/,
    uint32_t /*max_duration_seconds*/, const RttResponder& /*info*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiRttController::disableResponderInternal(uint32_t /*cmd_id*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
