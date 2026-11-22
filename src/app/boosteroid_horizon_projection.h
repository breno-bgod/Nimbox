// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid -> Horizon presentation projection.
//
// The xCloud worker already publishes GameTile/GameRow objects to AppUi.
// Keep the Boosteroid engine, authentication and session launch untouched:
// this adapter normalizes ONLY non-secret catalog data into the same UI model.
//
// Not connected to the running app yet. Never publish these rows until
// actions are dispatched to the Boosteroid backend, not the Xbox worker.
#pragma once

#include "ui/app_ui.h"
#include "util/json.h"

#include <cstdint>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace xc::app::boosteroid_horizon {

struct CatalogSnapshot {
    std::vector<ui::GameTile> installed;
    std::vector<ui::GameTile> catalog;
    std::vector<ui::GameRow> rows;
};

inline std::string firstString(const json::Value& v, std::initializer_list<const char*> keys) {
    if (!v.isObject()) return {};
    for (const char* key : keys) {
        const json::Value& member = v[key];
        if (member.isString() && !member.asString().empty()) return member.asString();
    }
    return {};
}

inline int64_t firstId(const json::Value& v) {
    if (!v.isObject()) return 0;
    for (const char* key : {"id", "appId", "applicationId", "gameId"}) {
        const auto& id = v[key];
        if (id.isNumber() && id.asInt() > 0) return id.asInt();
    }
    return 0;
}

inline const json::Value& gameDetails(const json::Value& value) {
    if (value.isObject()) {
        for (const char* key : {"application", "app", "game", "item"}) {
            if (value[key].isObject() && !value[key].members().empty()) return value[key];
        }
    }
    return value;
}

inline std::string imageUrl(const json::Value& details, const json::Value& root) {
    // No cookies, access tokens or authenticated requests enter the image cache.
    const std::string direct = firstString(details,
        {"cover", "coverUrl", "image", "imageUrl", "poster", "posterUrl", "thumbnail", "thumbnailUrl"});
    const std::string fallback = direct.empty()
        ? firstString(root, {"cover", "coverUrl", "image", "imageUrl", "poster", "thumbnail"})
        : direct;
    // The Horizon image loader should only receive externally hosted public artwork.
    return fallback.rfind("https://", 0) == 0 ? fallback : std::string();
}

inline bool toTile(const json::Value& raw, ui::GameTile& tile) {
    const auto& details = gameDetails(raw);
    const int64_t id = firstId(details) > 0 ? firstId(details) : firstId(raw);
    if (id <= 0) return false;

    tile = ui::GameTile{};
    // Prefixing prevents collisions with the Xbox catalog and existing caches.
    tile.productId = "boosteroid:" + std::to_string(id);
    tile.name = firstString(details, {"name", "title", "displayName"});
    if (tile.name.empty()) tile.name = firstString(raw, {"name", "title", "displayName"});
    if (tile.name.empty()) tile.name = "Jogo " + std::to_string(id);
    tile.publisher = firstString(details, {"publisher", "developer", "company"});
    tile.description = firstString(details, {"description", "summary"});
    tile.tileUrl = imageUrl(details, raw);
    tile.heroUrl = firstString(details, {"heroUrl", "backgroundUrl", "bannerUrl"});
    if (tile.heroUrl.rfind("https://", 0) != 0) tile.heroUrl = tile.tileUrl;

    // Enabled only when the NimboPlay Core dispatches this prefix to the
    // existing Boosteroid player, never to the Xbox Gssv/stream worker.
    tile.playable = true;
    tile.purchasable = false;
    tile.homeConsole = false;
    tile.titleId.clear();
    tile.xboxTitleId.clear();
    return true;
}

inline std::vector<ui::GameTile> tilesFromResponse(const json::Value& response) {
    const json::Value& games = response.isArray() ? response : response["games"];
    std::vector<ui::GameTile> tiles;
    if (!games.isArray()) return tiles;
    std::set<std::string> seen;
    for (const auto& game : games.items()) {
        ui::GameTile tile;
        if (toTile(game, tile) && seen.insert(tile.productId).second)
            tiles.push_back(std::move(tile));
    }
    return tiles;
}

inline CatalogSnapshot project(const json::Value& installedResponse, const json::Value& catalogResponse) {
    CatalogSnapshot snapshot;
    snapshot.installed = tilesFromResponse(installedResponse);
    snapshot.catalog = tilesFromResponse(catalogResponse);

    ui::GameRow installed;
    installed.title = "Meus jogos";
    installed.tiles = snapshot.installed;
    installed.catalogId = "boosteroid:installed";
    installed.gamePassBadges = false;
    snapshot.rows.push_back(std::move(installed));

    if (!snapshot.catalog.empty()) {
        ui::GameRow catalog;
        catalog.title = "Catálogo";
        catalog.tiles = snapshot.catalog;
        catalog.catalogId = "boosteroid:catalog";
        catalog.gamePassBadges = false;
        snapshot.rows.push_back(std::move(catalog));
    }
    return snapshot;
}

}  // namespace xc::app::boosteroid_horizon
