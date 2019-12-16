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
#include <linux/nl80211.h>      //NL80211 definitions
#include <stdbool.h>
#include <errno.h>
#include <net/if.h>

#include "hidl_return_util.h"
#include "wifi_sta_iface.h"
#include "wifi_status_util.h"

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

WifiStaIface::WifiStaIface(
    const std::string& ifname,
    const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util,
    struct nl_sock* control_socket,
    int _family_id)
    : ifname_(ifname),
      iface_util_(iface_util),
      is_valid_(true),
      control_socket_(control_socket),
      family_id(_family_id) {
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    nl_socket_set_cb(control_socket_, cb);
}

void WifiStaIface::invalidate() {
    event_cb_handler_.invalidate();
    nl_cb_put(cb);
    is_valid_ = false;
}

bool WifiStaIface::isValid() {
    return is_valid_;
}

std::string WifiStaIface::getName() {
    return ifname_;
}

std::set<sp<IWifiStaIfaceEventCallback>> WifiStaIface::getEventCallbacks() {
    return event_cb_handler_.getCallbacks();
}

Return<void> WifiStaIface::getName(getName_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getNameInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getType(getType_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getTypeInternal, hidl_status_cb);
}

Return<void> WifiStaIface::registerEventCallback(
    const sp<IWifiStaIfaceEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::registerEventCallbackInternal,
        hidl_status_cb, callback);
}

Return<void> WifiStaIface::getCapabilities(getCapabilities_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getCapabilitiesInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getApfPacketFilterCapabilities(
    getApfPacketFilterCapabilities_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getApfPacketFilterCapabilitiesInternal, hidl_status_cb);
}

Return<void> WifiStaIface::installApfPacketFilter(
    uint32_t cmd_id, const hidl_vec<uint8_t>& program,
    installApfPacketFilter_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
    &WifiStaIface::installApfPacketFilterInternal,
    hidl_status_cb, cmd_id, program);
}

Return<void> WifiStaIface::readApfPacketFilterData(
    readApfPacketFilterData_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::readApfPacketFilterDataInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getBackgroundScanCapabilities(
    getBackgroundScanCapabilities_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getBackgroundScanCapabilitiesInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getValidFrequenciesForBand(
    WifiBand band, getValidFrequenciesForBand_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getValidFrequenciesForBandInternal, hidl_status_cb,
        band);
}

Return<void> WifiStaIface::startBackgroundScan(
    uint32_t cmd_id, const StaBackgroundScanParameters& params,
    startBackgroundScan_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::startBackgroundScanInternal, hidl_status_cb, cmd_id,
        params);
}

Return<void> WifiStaIface::stopBackgroundScan(
    uint32_t cmd_id, stopBackgroundScan_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::stopBackgroundScanInternal, hidl_status_cb, cmd_id);
}

Return<void> WifiStaIface::enableLinkLayerStatsCollection(
    bool debug, enableLinkLayerStatsCollection_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::enableLinkLayerStatsCollectionInternal, hidl_status_cb,
        debug);
}

Return<void> WifiStaIface::disableLinkLayerStatsCollection(
    disableLinkLayerStatsCollection_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::disableLinkLayerStatsCollectionInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getLinkLayerStats(
    getLinkLayerStats_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiStaIface::getLinkLayerStatsInternal,
                           hidl_status_cb);
}

Return<void> WifiStaIface::getLinkLayerStats_1_3(
    getLinkLayerStats_1_3_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiStaIface::getLinkLayerStatsInternal_1_3,
                           hidl_status_cb);
}

Return<void> WifiStaIface::startRssiMonitoring(
    uint32_t cmd_id, int32_t max_rssi, int32_t min_rssi,
    startRssiMonitoring_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::startRssiMonitoringInternal, hidl_status_cb, cmd_id,
        max_rssi, min_rssi);
}

Return<void> WifiStaIface::stopRssiMonitoring(
    uint32_t cmd_id, stopRssiMonitoring_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::stopRssiMonitoringInternal, hidl_status_cb, cmd_id);
}

Return<void> WifiStaIface::getRoamingCapabilities(
    getRoamingCapabilities_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getRoamingCapabilitiesInternal, hidl_status_cb);
}

Return<void> WifiStaIface::configureRoaming(
    const StaRoamingConfig& config, configureRoaming_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::configureRoamingInternal, hidl_status_cb, config);
}

Return<void> WifiStaIface::setRoamingState(StaRoamingState state,
                                           setRoamingState_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::setRoamingStateInternal, hidl_status_cb, state);
}

Return<void> WifiStaIface::enableNdOffload(bool enable,
    enableNdOffload_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::enableNdOffloadInternal, hidl_status_cb, enable);
}

Return<void> WifiStaIface::startSendingKeepAlivePackets(
    uint32_t cmd_id, const hidl_vec<uint8_t>& ip_packet_data,
    uint16_t ether_type, const hidl_array<uint8_t, 6>& src_address,
    const hidl_array<uint8_t, 6>& dst_address, uint32_t period_in_ms,
    startSendingKeepAlivePackets_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::startSendingKeepAlivePacketsInternal,
        hidl_status_cb, cmd_id, ip_packet_data, ether_type,
        src_address, dst_address, period_in_ms);
}

Return<void> WifiStaIface::stopSendingKeepAlivePackets(
    uint32_t cmd_id, stopSendingKeepAlivePackets_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::stopSendingKeepAlivePacketsInternal, hidl_status_cb,
        cmd_id);
}

Return<void> WifiStaIface::setScanningMacOui(
    const hidl_array<uint8_t, 3>& oui, setScanningMacOui_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::setScanningMacOuiInternal, hidl_status_cb, oui);
}

Return<void> WifiStaIface::startDebugPacketFateMonitoring(
    startDebugPacketFateMonitoring_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::startDebugPacketFateMonitoringInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getDebugTxPacketFates(
    getDebugTxPacketFates_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getDebugTxPacketFatesInternal, hidl_status_cb);
}

Return<void> WifiStaIface::getDebugRxPacketFates(
    getDebugRxPacketFates_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::getDebugRxPacketFatesInternal, hidl_status_cb);
}

Return<void> WifiStaIface::setMacAddress(const hidl_array<uint8_t, 6>& mac,
    setMacAddress_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
        &WifiStaIface::setMacAddressInternal, hidl_status_cb, mac);
}

Return<void> WifiStaIface::getFactoryMacAddress(
    getFactoryMacAddress_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiStaIface::getFactoryMacAddressInternal,
                           hidl_status_cb);
}

std::pair<WifiStatus, std::string> WifiStaIface::getNameInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiStaIface::getTypeInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::STA};
}

WifiStatus WifiStaIface::registerEventCallbackInternal(
    const sp<IWifiStaIfaceEventCallback>& callback) {
    if (!event_cb_handler_.addCallback(callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, uint32_t> WifiStaIface::getCapabilitiesInternal() {
    uint32_t hidl_caps{0};
    hidl_caps |= StaIfaceCapabilityMask::STA_5G;
    hidl_caps |= StaIfaceCapabilityMask::KEEP_ALIVE;
    return {createWifiStatus(WifiStatusCode::SUCCESS), hidl_caps};
}

std::pair<WifiStatus, StaApfPacketFilterCapabilities>
WifiStaIface::getApfPacketFilterCapabilitiesInternal() {
    StaApfPacketFilterCapabilities hidl_caps;
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), hidl_caps};
}

WifiStatus WifiStaIface::installApfPacketFilterInternal(
    uint32_t /* cmd_id */, const std::vector<uint8_t>& /*program*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<WifiStatus, std::vector<uint8_t>>
WifiStaIface::readApfPacketFilterDataInternal() {
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), {}};
}

std::pair<WifiStatus, StaBackgroundScanCapabilities>
WifiStaIface::getBackgroundScanCapabilitiesInternal() {
    StaBackgroundScanCapabilities hidl_caps;
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), hidl_caps};
}

std::pair<WifiStatus, std::vector<WifiChannelInMhz>>
WifiStaIface::getValidFrequenciesForBandInternal(WifiBand band) {
    return iface_util_.lock()->getValidFrequenciesForBand(band,
        family_id, control_socket_, cb);
}

WifiStatus WifiStaIface::startBackgroundScanInternal(
    uint32_t /*cmd_id*/, const StaBackgroundScanParameters& /*params*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::stopBackgroundScanInternal(uint32_t /*cmd_id*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::enableLinkLayerStatsCollectionInternal(bool /*debug*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::disableLinkLayerStatsCollectionInternal() {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<WifiStatus, V1_0::StaLinkLayerStats>
WifiStaIface::getLinkLayerStatsInternal() {
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), {}};
}

std::pair<WifiStatus, V1_3::StaLinkLayerStats>
WifiStaIface::getLinkLayerStatsInternal_1_3() {
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), {}};
}

WifiStatus WifiStaIface::startRssiMonitoringInternal(uint32_t /*cmd_id*/,
    int32_t /*max_rssi*/, int32_t /*min_rssi*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::stopRssiMonitoringInternal(uint32_t /*cmd_id*/)
{
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<WifiStatus, StaRoamingCapabilities>
WifiStaIface::getRoamingCapabilitiesInternal() {
    StaRoamingCapabilities hidl_caps;
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), hidl_caps};
}

WifiStatus WifiStaIface::configureRoamingInternal(
    const StaRoamingConfig& /*config*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::setRoamingStateInternal(StaRoamingState /*state*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::enableNdOffloadInternal(bool /*enable*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::startSendingKeepAlivePacketsInternal(
    uint32_t /*cmd_id*/, const std::vector<uint8_t>& /*ip_packet_data*/,
    uint16_t /* ether_type */, const std::array<uint8_t, 6>& /*src_address*/,
    const std::array<uint8_t, 6>& /*dst_address*/, uint32_t /*period_in_ms*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::stopSendingKeepAlivePacketsInternal(uint32_t /*cmd_id*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::setScanningMacOuiInternal(
    const std::array<uint8_t, 3>& /*oui*/) {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiStaIface::startDebugPacketFateMonitoringInternal() {
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<WifiStatus, std::vector<WifiDebugTxPacketFateReport>>
WifiStaIface::getDebugTxPacketFatesInternal() {
    std::vector<WifiDebugTxPacketFateReport> hidl_fates;
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), hidl_fates};
}

std::pair<WifiStatus, std::vector<WifiDebugRxPacketFateReport>>
WifiStaIface::getDebugRxPacketFatesInternal() {
    std::vector<WifiDebugRxPacketFateReport> hidl_fates;
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), hidl_fates};
}

WifiStatus WifiStaIface::setMacAddressInternal(
    const std::array<uint8_t, 6>& mac) {
    bool status = iface_util_.lock()->setMacAddress(ifname_, mac);
    if (!status) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, std::array<uint8_t, 6>>
WifiStaIface::getFactoryMacAddressInternal() {
    std::array<uint8_t, 6> mac =
        iface_util_.lock()->getFactoryMacAddress(ifname_);
    return {createWifiStatus(WifiStatusCode::SUCCESS), mac};
}

}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
