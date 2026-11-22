// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid adapter over OpenStroid's existing auth, catalog and player.
// Does not read tokens, use Xbox APIs or implement any stream protocol.
#include "app/boosteroid_worker.h"
#include "app/boosteroid_control.h"
#include "app/boosteroid_horizon_projection.h"
#include "app/app_state.h"
#include "platform/platform.h"
#include "util/log.h"

#include <cstdlib>
#include <utility>

namespace xc::app {

bool BoosteroidWorker::active() const {
    return !stopping_.load() && g_ui && g_ui->boosteroidActive();
}

void BoosteroidWorker::start(std::string key) {
    stop();
    stopping_ = false;
    command_ = Command::None;
    thread_ = std::thread([this, key = std::move(key)] { run(key); });
}

void BoosteroidWorker::stop() {
    stopping_ = true;
    command_ = Command::None;
    if (thread_.joinable()) thread_.join();
}

void BoosteroidWorker::refresh() { command_ = Command::Refresh; }

void BoosteroidWorker::play(const ui::GameTile& tile) {
    {
        std::lock_guard<std::mutex> lock(tileMutex_);
        tile_ = tile;
    }
    command_ = Command::Play;
}

void BoosteroidWorker::cancelLaunch() { command_ = Command::Cancel; }

bool BoosteroidWorker::login(const std::string& key) {
    auto start = boosteroid_control::request(key, "POST", "/login/start", "{}");
    const std::string url = start.data["validationUrl"].str();
    if (!start.ok() || url.rfind("https://", 0) != 0) {
        if (active()) g_ui->showError("Não foi possível iniciar o login Boosteroid.");
        return false;
    }
    if (active()) g_ui->showBoosteroidSignIn(url);
    for (int i = 0; i < 300 && active(); ++i) {
        for (int j = 0; j < 15 && active(); ++j) platform::sleepMs(100);
        if (!active()) break;
        auto qr = boosteroid_control::request(key, "GET", "/login/status");
        if (!qr.ok()) continue;
        const std::string status = qr.data["status"].str();
        if (qr.data["authenticated"].asBool()) return true;
        if (status == "cancelled" || status == "timed_out") {
            if (active()) g_ui->showError("O login Boosteroid expirou. Tente novamente.");
            return false;
        }
    }
    if (active()) g_ui->showError("O login Boosteroid não foi confirmado.");
    return false;
}

bool BoosteroidWorker::loadCatalog(const std::string& key) {
    auto response = boosteroid_control::request(key, "GET", "/catalog", {}, 28000);
    if (!response.ok()) {
        if (active()) g_ui->showError("Não foi possível carregar o catálogo Boosteroid.");
        return false;
    }
    auto catalog = boosteroid_horizon::project(response.data["installed"], response.data["catalog"]);
    if (!active()) return false;
    g_ui->setBoosteroidCatalog(std::move(catalog.rows), std::move(catalog.installed),
                              std::move(catalog.catalog));
    return true;
}

void BoosteroidWorker::launch(const std::string& key, const ui::GameTile& tile) {
    constexpr std::string_view prefix = "boosteroid:";
    if (!active() || tile.productId.rfind(prefix, 0) != 0) return;
    const std::string suffix = tile.productId.substr(prefix.size());
    char* end = nullptr;
    const long long id = std::strtoll(suffix.c_str(), &end, 10);
    if (id <= 0 || !end || *end != '\0') {
        if (active()) g_ui->showPlayError("Jogo inválido.", tile);
        return;
    }
    json::Value body = json::Value::object();
    body.set("appId", static_cast<int64_t>(id));
    auto result = boosteroid_control::request(key, "POST", "/play", body.dump());
    if (!result.ok()) {
        if (active()) g_ui->showPlayError("Não foi possível iniciar o jogo no Boosteroid.", tile);
        return;
    }
    for (int i = 0; i < 380 && active(); ++i) {
        if (command_ == Command::Cancel) {
            (void)boosteroid_control::request(key, "POST", "/cancel", "{}");
            command_ = Command::None;
            if (active()) g_ui->showBoosteroidHome();
            return;
        }
        platform::sleepMs(500);
        auto status = boosteroid_control::request(key, "GET", "/status");
        if (!status.ok()) continue;
        if (!status.data["launching"].asBool()) {
            const std::string error = status.data["error"].str();
            if (active()) {
                if (error.empty()) g_ui->showBoosteroidHome();
                else g_ui->showPlayError(error, tile);
            }
            return;
        }
    }
    if (active()) g_ui->showPlayError("A inicialização do Boosteroid demorou demais.", tile);
}

void BoosteroidWorker::run(const std::string& key) {
    platform::setThreadAffinity(platform::ThreadRole::Worker, "boosteroid_ui");
    bool started = false;
    for (int i = 0; i < 70 && active(); ++i) {
        const auto response = boosteroid_control::request(key, "GET", "/health", {}, 1000);
        if (response.ok() && response.data["ok"].asBool()) { started = true; break; }
        platform::sleepMs(350);
    }
    if (!active()) return;
    if (!started) {
        g_ui->showError("O serviço Boosteroid não iniciou. Tente novamente.");
        return;
    }
    auto status = boosteroid_control::request(key, "GET", "/status");
    if (!status.ok()) {
        if (active()) g_ui->showError("O Boosteroid não respondeu.");
        return;
    }
    if (!status.data["authenticated"].asBool() && !login(key)) return;
    if (!active()) return;
    status = boosteroid_control::request(key, "GET", "/status");
    if (status.ok()) g_ui->setBoosteroidProfile(status.data["name"].str());
    if (!loadCatalog(key) || !active()) return;
    g_ui->showBoosteroidHome();
    while (active()) {
        Command cmd = command_.exchange(Command::None);
        if (cmd == Command::Refresh) {
            g_ui->setLoading(true, 0, 0);
            loadCatalog(key);
            if (active()) g_ui->setLoading(false, 0, 0);
        } else if (cmd == Command::Play) {
            ui::GameTile tile;
            { std::lock_guard<std::mutex> lock(tileMutex_); tile = tile_; }
            launch(key, tile);
        } else if (cmd == Command::Cancel) {
            (void)boosteroid_control::request(key, "POST", "/cancel", "{}");
            if (active()) g_ui->showBoosteroidHome();
        }
        platform::sleepMs(100);
    }
}
} // namespace xc::app
