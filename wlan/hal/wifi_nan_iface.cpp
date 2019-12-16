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
#include "wifi_nan_iface.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace ti {
using hidl_return_util::validateAndCall;

WifiNanIface::WifiNanIface(
    const std::string& ifname,
    const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util,
    struct nl_sock* control_socket __unused)
    : ifname_(ifname),
      iface_util_(iface_util),
      is_valid_(true) {
    // Register all the callbacks here. these should be valid for the lifetime
    // of the object. Whenever the mode changes legacy HAL will remove
    // all of these callbacks.
    android::wp<WifiNanIface> weak_ptr_this(this);
    // Callback for response.
}

void WifiNanIface::invalidate() {
    // send commands to HAL to actually disable and destroy interfaces
    event_cb_handler_.invalidate();
    event_cb_handler_1_2_.invalidate();
    is_valid_ = false;
}

bool WifiNanIface::isValid() {
    return is_valid_;
}

std::string WifiNanIface::getName() {
    return ifname_;
}

std::set<sp<V1_0::IWifiNanIfaceEventCallback>>
WifiNanIface::getEventCallbacks() {
    return event_cb_handler_.getCallbacks();
}

std::set<sp<V1_2::IWifiNanIfaceEventCallback>>
WifiNanIface::getEventCallbacks_1_2() {
    return event_cb_handler_1_2_.getCallbacks();
}

Return<void> WifiNanIface::getName(getName_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::getNameInternal, hidl_status_cb);
}

Return<void> WifiNanIface::getType(getType_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::getTypeInternal, hidl_status_cb);
}

Return<void> WifiNanIface::registerEventCallback(
    const sp<V1_0::IWifiNanIfaceEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::registerEventCallbackInternal, hidl_status_cb, callback);
}

Return<void> WifiNanIface::getCapabilitiesRequest(
    uint16_t cmd_id, getCapabilitiesRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::getCapabilitiesRequestInternal, hidl_status_cb, cmd_id);
}

Return<void> WifiNanIface::enableRequest(uint16_t cmd_id, const NanEnableRequest& msg,
    enableRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::enableRequestInternal, hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::configRequest(uint16_t cmd_id,
    const NanConfigRequest& msg, configRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::configRequestInternal, hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::disableRequest(uint16_t cmd_id,
    disableRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::disableRequestInternal, hidl_status_cb, cmd_id);
}

Return<void> WifiNanIface::startPublishRequest(
    uint16_t cmd_id, const NanPublishRequest& msg,
    startPublishRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::startPublishRequestInternal, hidl_status_cb, cmd_id,
        msg);
}

Return<void> WifiNanIface::stopPublishRequest(
    uint16_t cmd_id, uint8_t sessionId, stopPublishRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::stopPublishRequestInternal, hidl_status_cb, cmd_id,
        sessionId);
}

Return<void> WifiNanIface::startSubscribeRequest(
    uint16_t cmd_id, const NanSubscribeRequest& msg,
    startSubscribeRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
    &WifiNanIface::startSubscribeRequestInternal, hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::stopSubscribeRequest(
    uint16_t cmd_id, uint8_t sessionId, stopSubscribeRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::stopSubscribeRequestInternal, hidl_status_cb, cmd_id,
        sessionId);
}

Return<void> WifiNanIface::transmitFollowupRequest(
    uint16_t cmd_id, const NanTransmitFollowupRequest& msg,
    transmitFollowupRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::transmitFollowupRequestInternal, hidl_status_cb, cmd_id,
        msg);
}

Return<void> WifiNanIface::createDataInterfaceRequest(
    uint16_t cmd_id, const hidl_string& iface_name,
    createDataInterfaceRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::createDataInterfaceRequestInternal, hidl_status_cb,
        cmd_id, iface_name);
}

Return<void> WifiNanIface::deleteDataInterfaceRequest(
    uint16_t cmd_id, const hidl_string& iface_name,
    deleteDataInterfaceRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::deleteDataInterfaceRequestInternal, hidl_status_cb,
        cmd_id, iface_name);
}

Return<void> WifiNanIface::initiateDataPathRequest(
    uint16_t cmd_id, const NanInitiateDataPathRequest& msg,
    initiateDataPathRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::initiateDataPathRequestInternal, hidl_status_cb, cmd_id,
        msg);
}

Return<void> WifiNanIface::respondToDataPathIndicationRequest(
    uint16_t cmd_id, const NanRespondToDataPathIndicationRequest& msg,
    respondToDataPathIndicationRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::respondToDataPathIndicationRequestInternal,
        hidl_status_cb, cmd_id, msg);
}

Return<void> WifiNanIface::terminateDataPathRequest(
    uint16_t cmd_id, uint32_t ndpInstanceId,
    terminateDataPathRequest_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::terminateDataPathRequestInternal, hidl_status_cb,
        cmd_id, ndpInstanceId);
}

Return<void> WifiNanIface::registerEventCallback_1_2(
    const sp<V1_2::IWifiNanIfaceEventCallback>& callback,
    registerEventCallback_1_2_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::registerEventCallback_1_2Internal, hidl_status_cb,
        callback);
}

Return<void> WifiNanIface::enableRequest_1_2(
    uint16_t cmd_id, const NanEnableRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2,
    enableRequest_1_2_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::enableRequest_1_2Internal, hidl_status_cb,
        cmd_id, msg1, msg2);
}

Return<void> WifiNanIface::configRequest_1_2(
    uint16_t cmd_id, const NanConfigRequest& msg1,
    const V1_2::NanConfigRequestSupplemental& msg2,
    configRequest_1_2_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiNanIface::configRequest_1_2Internal, hidl_status_cb,
        cmd_id, msg1, msg2);
}

std::pair<WifiStatus, std::string> WifiNanIface::getNameInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiNanIface::getTypeInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::NAN};
}

WifiStatus WifiNanIface::registerEventCallbackInternal(
    const sp<V1_0::IWifiNanIfaceEventCallback>& callback) {
    if (!event_cb_handler_.addCallback(callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiNanIface::getCapabilitiesRequestInternal(uint16_t /*cmd_id*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::enableRequestInternal(
    uint16_t /* cmd_id */, const NanEnableRequest& /* msg */) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::configRequestInternal(
    uint16_t /* cmd_id */, const NanConfigRequest& /* msg */) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::disableRequestInternal(uint16_t /*cmd_id*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::startPublishRequestInternal(
    uint16_t /*cmd_id*/, const NanPublishRequest& /*msg*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::stopPublishRequestInternal(uint16_t /*cmd_id*/,
    uint8_t /*sessionId*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::startSubscribeRequestInternal(
    uint16_t /*cmd_id*/, const NanSubscribeRequest& /*msg*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::stopSubscribeRequestInternal(uint16_t /*cmd_id*/,
    uint8_t /*sessionId*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::transmitFollowupRequestInternal(
    uint16_t /*cmd_id*/, const NanTransmitFollowupRequest& /*msg*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::createDataInterfaceRequestInternal(
    uint16_t /*cmd_id*/, const std::string& /*iface_name*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}
WifiStatus WifiNanIface::deleteDataInterfaceRequestInternal(
    uint16_t /*cmd_id*/, const std::string& /*iface_name*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}
WifiStatus WifiNanIface::initiateDataPathRequestInternal(
    uint16_t /*cmd_id*/, const NanInitiateDataPathRequest& /*msg*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}
WifiStatus WifiNanIface::respondToDataPathIndicationRequestInternal(
    uint16_t /*cmd_id*/, const NanRespondToDataPathIndicationRequest& /*msg*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}
WifiStatus WifiNanIface::terminateDataPathRequestInternal(
    uint16_t /*cmd_id*/, uint32_t /*ndpInstanceId*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::registerEventCallback_1_2Internal(
    const sp<V1_2::IWifiNanIfaceEventCallback>& callback) {
    sp<V1_0::IWifiNanIfaceEventCallback> callback_1_0 = callback;
    if (!event_cb_handler_.addCallback(callback_1_0)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    if (!event_cb_handler_1_2_.addCallback(callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiNanIface::enableRequest_1_2Internal(
    uint16_t /*cmd_id*/, const NanEnableRequest& /*msg1*/,
    const V1_2::NanConfigRequestSupplemental& /*msg2*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiNanIface::configRequest_1_2Internal(
    uint16_t /*cmd_id*/, const NanConfigRequest& /*msg1*/,
    const V1_2::NanConfigRequestSupplemental& /*msg2*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
