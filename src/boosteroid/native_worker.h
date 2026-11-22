// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid worker stays independent of the Xbox worker and GssvClient.
#pragma once

#include "platform/platform.h"

#include <atomic>

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
    void requestStop() { stop_ = true; }
    void stop();
private:
    void run();
    bool pause(int milliseconds);
    ui::AppUi& ui_;
    platform::Thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> refresh_{false};
    std::atomic<bool> logout_{false};
};

} // namespace xc::boosteroid
