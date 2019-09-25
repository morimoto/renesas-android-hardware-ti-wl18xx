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
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <android-base/logging.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>
#include <linux/pkt_sched.h>

#include "hidl_return_util.h"
#include "wifi.h"
#include "wifi_status_util.h"

/*
 BUGBUG: normally, libnl allocates ports for all connections it makes; but
 being a static library, it doesn't really know how many other netlink connections
 are made by the same process, if connections come from different shared libraries.
 These port assignments exist to solve that problem - temporarily. We need to fix
 libnl to try and allocate ports across the entire process.
 */

#define WIFI_HAL_CMD_SOCK_PORT   644
#define WIFI_HAL_EVENT_SOCK_PORT 645

namespace {
// Chip ID to use for the only supported chip.
static constexpr android::hardware::wifi::V1_0::ChipId kChipId = 0;
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace ti {
using hidl_return_util::validateAndCall;
using hidl_return_util::validateAndCallWithLock;

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

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

Wifi::Wifi(
    const std::shared_ptr<wifi_system::InterfaceTool> iface_tool,
    const std::shared_ptr<mode_controller::WifiModeController> mode_controller,
    const std::shared_ptr<iface_util::WifiIfaceUtil> iface_util,
    const std::shared_ptr<feature_flags::WifiFeatureFlags> feature_flags)
    : iface_tool_(iface_tool),
      mode_controller_(mode_controller),
      iface_util_(iface_util),
      feature_flags_(feature_flags),
      run_state_(RunState::STOPPED) {}

bool Wifi::isValid() {
    // This object is always valid.
    return true;
}

Return<void> Wifi::registerEventCallback(
    const sp<IWifiEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
        &Wifi::registerEventCallbackInternal, hidl_status_cb,
        event_callback);
}

Return<bool> Wifi::isStarted() {
    return run_state_ != RunState::STOPPED;
}

Return<void> Wifi::start(start_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
       &Wifi::startInternal, hidl_status_cb);
}

Return<void> Wifi::stop(stop_cb hidl_status_cb) {
    return validateAndCallWithLock(this, WifiStatusCode::ERROR_UNKNOWN,
        &Wifi::stopInternal, hidl_status_cb);
}

Return<void> Wifi::getChipIds(getChipIds_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
        &Wifi::getChipIdsInternal, hidl_status_cb);
}

Return<void> Wifi::getChip(ChipId chip_id, getChip_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
        &Wifi::getChipInternal, hidl_status_cb, chip_id);
}

Return<void> Wifi::debug(const hidl_handle& handle,
                         const hidl_vec<hidl_string>&) {
    LOG(INFO) << "-----------Debug is called----------------";
    if (!chip_.get()) {
        return Void();
    }
    return chip_->debug(handle, {});
}

WifiStatus Wifi::registerEventCallbackInternal(
    const sp<IWifiEventCallback>& event_callback) {
    if (!event_cb_handler_.addCallback(event_callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus Wifi::startInternal() {
    if (run_state_ == RunState::STARTED) {
        return createWifiStatus(WifiStatusCode::SUCCESS);
    } else if (run_state_ == RunState::STOPPING) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                "HAL is stopping");
    }

    srand(getpid());
    LOG(INFO) << "Initializing wifi";
    control_socket = nl_socket_alloc();
    if (!control_socket) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                "Failed to allocate netlink socket.");
    }

    nl_socket_set_buffer_size(control_socket, 8192, 8192);

    if (genl_connect(control_socket)) {
        nl_close(control_socket);
        nl_socket_free(control_socket);
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                "Failed to connect to netlink socket.");
    }

    id = genl_ctrl_resolve(control_socket, "nl80211");
    if (id < 0) {
        nl_close(control_socket);
        nl_socket_free(control_socket);
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                "Nl80211 interface not found.\n");
    }

    WifiStatus wifi_status = initializeModeController();
    if (wifi_status.code == WifiStatusCode::SUCCESS) {
        // Create the chip instance once the HAL is started.
        chip_ = new WifiChip(kChipId, control_socket, mode_controller_,
            iface_util_, feature_flags_, id);
        run_state_ = RunState::STARTED;
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onStart().isOk()) {
                LOG(ERROR) << "Failed to invoke onStart callback";
            }
        }
        LOG(INFO) << "Wifi HAL started";
    } else {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onFailure(wifi_status).isOk()) {
                LOG(ERROR) << "Failed to invoke onFailure callback";
            }
        }
        LOG(ERROR) << "Wifi HAL start failed";
    }
    return wifi_status;
}

WifiStatus Wifi::stopInternal(
    /* NONNULL */ std::unique_lock<std::recursive_mutex>* lock) {
    if (run_state_ == RunState::STOPPED) {
        return createWifiStatus(WifiStatusCode::SUCCESS);
    } else if (run_state_ == RunState::STOPPING) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
            "HAL is stopping");
    }
    // Clear the chip object and its child objects since the HAL is now
    // stopped.
    if (chip_.get()) {
        chip_->invalidate();
        chip_.clear();
    }

    WifiStatus wifi_status = stopAndDeinitializeModeController(lock);
    if (wifi_status.code == WifiStatusCode::SUCCESS) {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onStop().isOk()) {
                LOG(ERROR) << "Failed to invoke onStop callback";
            }
        }
        LOG(INFO) << "Wifi HAL stopped";
    } else {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onFailure(wifi_status).isOk()) {
                LOG(ERROR) << "Failed to invoke onFailure callback";
            }
        }
        LOG(ERROR) << "Wifi HAL stop failed";
    }
    return wifi_status;
}

std::pair<WifiStatus, std::vector<ChipId>> Wifi::getChipIdsInternal() {
    std::vector<ChipId> chip_ids;
    if (chip_.get()) {
        chip_ids.emplace_back(kChipId);
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), std::move(chip_ids)};
}

std::pair<WifiStatus, sp<IWifiChip>> Wifi::getChipInternal(ChipId chip_id) {
    if (!chip_.get()) {
        return {createWifiStatus(WifiStatusCode::ERROR_NOT_STARTED), nullptr};
    }
    if (chip_id != kChipId) {
        return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), chip_};
}

WifiStatus Wifi::initializeModeController() {
    if (!mode_controller_->initialize()) {
        LOG(ERROR) << "Failed to initialize firmware mode controller";
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus Wifi::stopAndDeinitializeModeController(
    /* NONNULL */ std::unique_lock<std::recursive_mutex>* /*lock*/) {
    run_state_ = RunState::STOPPING;
    nl_close(control_socket);
    nl_socket_free(control_socket);
    run_state_ = RunState::STOPPED;
    if (!mode_controller_->deinitialize()) {
        LOG(ERROR) << "Failed to deinitialize firmware mode controller";
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}
}  // namespace ti
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
