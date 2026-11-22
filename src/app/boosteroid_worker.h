// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid process adapter; mirrors the PSBox worker -> AppUi structure.
#pragma once
#include "ui/app_ui.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace xc::app {
class BoosteroidWorker {
public:
    BoosteroidWorker() = default;
    ~BoosteroidWorker() { stop(); }
    BoosteroidWorker(const BoosteroidWorker&) = delete;
    BoosteroidWorker& operator=(const BoosteroidWorker&) = delete;
    void start(std::string key);
    void stop();
    // Nonblocking request: switching services must never wait on I/O.
    void requestStop() { stopping_ = true; command_ = Command::None; }
    void refresh();
    void play(const ui::GameTile& tile);
    void cancelLaunch();

private:
    enum class Command { None, Refresh, Play, Cancel };
    std::atomic<bool> stopping_{false};
    std::atomic<Command> command_{Command::None};
    std::thread thread_;
    std::mutex tileMutex_;
    ui::GameTile tile_;
    void run(const std::string& key);
    bool loadCatalog(const std::string& key);
    bool login(const std::string& key);
    void launch(const std::string& key, const ui::GameTile& tile);
    bool active() const;
};
} // namespace xc::app
