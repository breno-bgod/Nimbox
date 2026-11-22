// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid worker stays independent of the Xbox worker and GssvClient.
#pragma once

#include "platform/platform.h"
#include "input/controller.h"

#include <atomic>
#include <cstdint>
#include <mutex>

namespace xc::ui { class AppUi; }

namespace xc::boosteroid {

class NativeWorker {
public:
    explicit NativeWorker(ui::AppUi& ui) : ui_(ui) {}
    NativeWorker(const NativeWorker&) = delete;
    NativeWorker& operator=(const NativeWorker&) = delete;
    ~NativeWorker() { stop(); }
    bool start();
    void requestRefresh() { refresh_ = true; }
    void requestLogout() { logout_ = true; }
    void requestStop() { stop_ = true; cancelStream_ = true; }
    void requestLaunch(int64_t gameId) { if (gameId > 0) launchGame_ = gameId; }
    void requestCancelStream() { cancelStream_ = true; }
    void setPad(const input::ControllerState& pad) {
        std::lock_guard<std::mutex> lock(padMutex_);
        pad_ = pad;
    }
    void stop();
private:
    void run();
    bool pause(int milliseconds);
    bool padSnapshot(input::ControllerState& pad);
    ui::AppUi& ui_;
    platform::Thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> refresh_{false};
    std::atomic<bool> logout_{false};
    std::atomic<int64_t> launchGame_{0};
    std::atomic<bool> cancelStream_{false};
    std::mutex padMutex_;
    input::ControllerState pad_;
};

} // namespace xc::boosteroid
