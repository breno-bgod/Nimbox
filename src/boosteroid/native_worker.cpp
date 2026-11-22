// SPDX-License-Identifier: GPL-3.0-or-later
// Native Boosteroid QR/catalog worker. No Electron and no Xbox worker changes.
#include "boosteroid/native_worker.h"

#include "boosteroid/native_client.h"
#include "ui/app_ui.h"
#include "util/log.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace xc::boosteroid {
namespace {
const char* errorText(ApiError error) {
    switch (error) {
        case ApiError::NoSession: return "Faça login na sua conta Boosteroid.";
        case ApiError::Network: return "Não foi possível conectar ao Boosteroid.";
        case ApiError::InvalidResponse: return "O Boosteroid retornou dados não reconhecidos.";
        case ApiError::Unauthorized: return "Sua sessão Boosteroid expirou.";
        case ApiError::RateLimited: return "O Boosteroid pediu para aguardar antes de tentar novamente.";
        case ApiError::Storage: return "Não foi possível salvar a sessão com segurança.";
        case ApiError::ServiceRejected: return "O Boosteroid recusou esta operação.";
        case ApiError::InvalidDevice: return "Não foi possível criar a identidade do dispositivo.";
        case ApiError::NotReady: return "O serviço Boosteroid ainda não está pronto.";
        case ApiError::None: return "Falha desconhecida no serviço.";
    }
    return "Erro de serviço.";
}

ui::GameTile toTile(const Game& game) {
    ui::GameTile tile;
    tile.productId = "boosteroid:" + std::to_string(game.id);
    tile.name = game.title;
    tile.publisher = game.publisher;
    tile.tileUrl = game.coverUrl;
    tile.heroUrl = game.coverUrl;
    // This provider cannot launch gameplay until the native transport exists.
    // Never route its ID to the Xbox kPlay worker or GSSV client.
    tile.playable = false;
    tile.purchasable = false;
    return tile;
}
} // namespace

bool NativeWorker::start() {
    if (thread_.joinable()) return true;
    stop_ = false;
    refresh_ = false;
    logout_ = false;
    if (!platform::startThread(thread_, [this] { run(); }, 8u << 20)) {
        ui_.setBoosteroidNativeError("Não foi possível iniciar o worker Boosteroid.");
        return false;
    }
    return true;
}

void NativeWorker::stop() {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
}

bool NativeWorker::pause(int milliseconds) {
    for (int t = 0; t < milliseconds && !stop_; t += 100)
        platform::sleepMs(static_cast<uint32_t>(std::min(100, milliseconds - t)));
    return !stop_;
}

void NativeWorker::run() {
    platform::setThreadAffinity(platform::ThreadRole::Worker, "boosteroid_native");
    NativeClient client;
    bool authenticated = client.restoreSession();
    if (authenticated) {
        Profile profile;
        ui_.setBoosteroidNativeStatus("Validando sessão Boosteroid...");
        if (!client.getProfile(profile)) {
            if (client.error() == ApiError::Unauthorized) {
                client.logout();
                authenticated = false;
            } else {
                ui_.setBoosteroidNativeError(errorText(client.error()));
                authenticated = false;
            }
        }
    }

    while (!stop_) {
        if (!authenticated) {
            DeviceAuthorization qr;
            ui_.setBoosteroidNativeStatus("Preparando autenticação QR...");
            if (!client.beginDeviceLogin(qr)) {
                ui_.setBoosteroidNativeError(errorText(client.error()));
                if (!pause(3500)) return;
                continue;
            }
            ui_.setBoosteroidNativeLogin(qr.validationUrl);
            // The Android TV device authorization expires after roughly 5 min.
            // Ask at the server's pace and never busy-loop or block the Horizon.
            const uint64_t endAt = platform::nowMs() + 300000;
            while (!stop_ && platform::nowMs() < endAt) {
                if (logout_.exchange(false)) {
                    client.logout();
                    break;
                }
                if (refresh_.exchange(false)) break;
                if (!pause(qr.pollIntervalMs)) return;
                const LoginResult result = client.pollDeviceLogin();
                if (result == LoginResult::Authorized) {
                    authenticated = true;
                    break;
                }
                if (result == LoginResult::Expired) break;
                if (result == LoginResult::RateLimited) {
                    ui_.setBoosteroidNativeStatus("Aguarde: limite temporário do Boosteroid.");
                    if (!pause(7000)) return;
                } else if (result == LoginResult::NetworkError) {
                    ui_.setBoosteroidNativeStatus("Sem conexão; tentando novamente...");
                    if (!pause(4000)) return;
                } else if (result == LoginResult::Rejected) {
                    ui_.setBoosteroidNativeError(errorText(client.error()));
                    if (!pause(4500)) return;
                    break;
                }
            }
            if (!authenticated) {
                if (!stop_) ui_.setBoosteroidNativeStatus("Gerando novo QR Code...");
                if (!pause(1000)) return;
                continue;
            }
        }
        if (stop_) break;

        Profile profile;
        ui_.setBoosteroidNativeStatus("Consultando o perfil Boosteroid...");
        if (!client.getProfile(profile)) {
            const auto why = client.error();
            if (why == ApiError::Unauthorized) {
                client.logout();
                authenticated = false;
                continue;
            }
            ui_.setBoosteroidNativeError(errorText(why));
            if (!pause(3000)) return;
            authenticated = false;
            continue;
        }

        const auto load = [&]() -> bool {
            ui_.setBoosteroidNativeStatus("Carregando biblioteca Boosteroid...");
            std::vector<ui::GameTile> tiles;
            // Small pages keep the Android/PS5 cache manageable and support
            // accounts with libraries larger than one upstream page.
            for (int page = 1; page <= 20 && !stop_; ++page) {
                std::vector<Game> games;
                if (!client.getInstalledGames(games, page, 50)) {
                    ui_.setBoosteroidNativeError(errorText(client.error()));
                    return false;
                }
                for (const auto& game : games) tiles.push_back(toTile(game));
                if (games.size() < 50) break;
            }
            if (stop_) return false;
            ui_.setBoosteroidNativeCatalog(std::move(tiles),
                profile.nickname.empty() ? "Boosteroid" : profile.nickname);
            return true;
        };

        if (!load()) {
            if (client.error() == ApiError::Unauthorized) {
                client.logout();
                authenticated = false;
                continue;
            }
            if (!pause(3500)) return;
        }
        while (!stop_) {
            if (logout_.exchange(false)) {
                client.logout();
                authenticated = false;
                break;
            }
            if (refresh_.exchange(false)) {
                if (!load() && client.error() == ApiError::Unauthorized) {
                    client.logout();
                    authenticated = false;
                    break;
                }
            }
            if (!pause(100)) return;
        }
    }
}

} // namespace xc::boosteroid
