// SPDX-License-Identifier: GPL-3.0-or-later
// NimboPlay portable Boosteroid provider contract; offline data projection.
#include "boosteroid/native_contract.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <string_view>

namespace xc::boosteroid {
namespace {
constexpr size_t kMaxGames = 2000;
constexpr size_t kMaxVideoAu = 4u * 1024u * 1024u;
constexpr size_t kMaxOpusPacket = 64u * 1024u;

std::string field(const json::Value& value, std::initializer_list<const char*> names) {
    if (!value.isObject()) return {};
    for (const char* name : names) {
        const auto& entry = value[name];
        if (entry.isString() && !entry.asString().empty()) return entry.asString();
    }
    return {};
}

bool isSafeHttps(std::string_view url) {
    if (url.size() < 10 || url.size() > 4096 || url.substr(0, 8) != "https://") return false;
    const size_t authorityEnd = url.find_first_of("/?#", 8);
    const std::string_view authority = url.substr(8, authorityEnd == std::string_view::npos
                                                ? std::string_view::npos : authorityEnd - 8);
    if (authority.empty() || authority.find('@') != std::string_view::npos) return false;
    return std::none_of(url.begin(), url.end(), [](unsigned char c) { return c <= 32 || c == 127; });
}

bool isSafePublicArt(std::string_view url) {
    // Keep query-signed art URLs inside the provider, not Horizon's image cache.
    return isSafeHttps(url) && url.find_first_of("?#") == std::string_view::npos;
}

int64_t appId(const json::Value& value) {
    if (value.isNumber()) {
        double n = value.asNumber();
        // JSON numbers are stored as doubles. Reject fractional/unsafe ids.
        if (std::isfinite(n) && n >= 1 && n <= 9007199254740991.0 && std::floor(n) == n)
            return static_cast<int64_t>(n);
    } else if (value.isString()) {
        std::string_view text = value.asString();
        int64_t parsed = 0;
        auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (ec == std::errc{} && end == text.data() + text.size() && parsed > 0 &&
            parsed <= 9007199254740991LL)
            return parsed;
    }
    return 0;
}

const json::Value& inner(const json::Value& game) {
    if (!game.isObject()) return game;
    for (const char* key : {"application", "app", "game", "item"}) {
        if (game[key].isObject() && !game[key].members().empty()) return game[key];
    }
    return game;
}

Game project(const json::Value& raw) {
    Game result;
    const auto& details = inner(raw);
    for (const auto& candidate : {std::cref(details), std::cref(raw)}) {
        const auto& entry = candidate.get();
        if (!result.id) {
            for (const char* key : {"id", "appId", "applicationId", "gameId"}) {
                result.id = appId(entry[key]);
                if (result.id) break;
            }
        }
        if (result.title.empty()) result.title = field(entry, {"name", "title", "displayName"});
        if (result.publisher.empty()) result.publisher = field(entry, {"publisher", "developer"});
        if (result.coverUrl.empty()) {
            auto art = field(entry, {"cover", "coverUrl", "image", "imageUrl", "poster", "posterUrl", "thumbnail"});
            if (isSafePublicArt(art)) result.coverUrl = std::move(art);
        }
    }
    if (result.title.size() > 256) result.title.resize(256);
    if (result.publisher.size() > 256) result.publisher.resize(256);
    return result;
}

const json::Value& gameArray(const json::Value& response) {
    if (response.isArray()) return response;
    if (response["games"].isArray()) return response["games"];
    if (response["result"].isArray()) return response["result"];
    if (response["result"]["games"].isArray()) return response["result"]["games"];
    if (response["data"]["games"].isArray()) return response["data"]["games"];
    return response["games"]; // null => no results
}
} // namespace

bool publicAuthorization(const json::Value& response, DeviceAuthorization& output) {
    output = {};
    const std::string url = field(response, {"validationUrl", "verificationUrl"});
    if (!isSafeHttps(url)) return false;
    output.validationUrl = url;
    output.status = field(response, {"status"});
    if (const auto& interval = response["pollIntervalMs"]; interval.isNumber()) {
        const double ms = interval.asNumber();
        if (std::isfinite(ms) && ms >= 1000 && ms <= 15000)
            output.pollIntervalMs = static_cast<int>(ms);
    }
    output.approved = output.status == "succeeded" && response["sessionEstablished"].asBool();
    return true;
}

std::vector<Game> publicGames(const json::Value& response) {
    const auto& source = gameArray(response);
    std::vector<Game> games;
    if (!source.isArray()) return games;
    std::set<int64_t> found;
    for (const auto& item : source.items()) {
        Game game = project(item);
        if (game.id <= 0 || game.title.empty() || !found.insert(game.id).second) continue;
        games.push_back(std::move(game));
        if (games.size() >= kMaxGames) break;
    }
    return games;
}

bool deliverH264(const EncodedMediaSink& sink, const uint8_t* payload, size_t size,
                 uint32_t timestamp) {
    if (!sink.h264AnnexB || !payload || size < 5 || size > kMaxVideoAu) return false;
    const bool shortStart = payload[0] == 0 && payload[1] == 0 && payload[2] == 1;
    const bool longStart = payload[0] == 0 && payload[1] == 0 && payload[2] == 0 && payload[3] == 1;
    if (!shortStart && !longStart) return false;
    sink.h264AnnexB(payload, size, timestamp);
    return true;
}

bool deliverOpus(const EncodedMediaSink& sink, const uint8_t* payload, size_t size,
                 uint32_t timestamp) {
    if (!sink.opus || !payload || !size || size > kMaxOpusPacket) return false;
    sink.opus(payload, size, timestamp);
    return true;
}

bool SessionState::beginAuthorization() {
    if (state_ != State::Disabled && state_ != State::Failed) return false;
    failure_ = Failure::None;
    state_ = State::Authorizing;
    return true;
}
bool SessionState::authorizationApproved() {
    if (state_ != State::Authorizing) return false;
    state_ = State::Authorized;
    return true;
}
bool SessionState::catalogLoaded() {
    if (state_ != State::Authorized && state_ != State::CatalogReady) return false;
    state_ = State::CatalogReady;
    return true;
}
bool SessionState::beginLaunch() {
    if (state_ != State::CatalogReady) return false;
    state_ = State::Launching;
    return true;
}
bool SessionState::streamAttached() {
    if (state_ != State::Launching) return false;
    state_ = State::Streaming;
    return true;
}
bool SessionState::streamStopped() {
    if (state_ != State::Launching && state_ != State::Streaming) return false;
    state_ = State::CatalogReady;
    return true;
}
void SessionState::fail(Failure reason) {
    failure_ = reason == Failure::None ? Failure::Unsupported : reason;
    state_ = State::Failed;
}
void SessionState::reset() {
    failure_ = Failure::None;
    state_ = State::Disabled;
}
} // namespace xc::boosteroid
