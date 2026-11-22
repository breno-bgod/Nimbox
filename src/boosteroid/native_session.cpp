// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid native queue/session management. Protocol behavior researched from
// ZERODROID (Apache-2.0), without Electron or Xbox GSSV.
#include "boosteroid/native_client.h"

#ifndef _WIN32
#include "boosteroid/wss_client.h"
#endif
#include "platform/platform.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <string_view>
#include <utility>

namespace xc::boosteroid {
namespace {
constexpr int kMaxQueueSeconds = 180;
constexpr int kMaxDetailsSeconds = 60;

bool canceled(const std::atomic<bool>* flag) { return flag && flag->load(); }

bool pauseUntil(int ms, const std::atomic<bool>* flag) {
    for (int i = 0; i < ms; i += 50) {
        if (canceled(flag)) return false;
        platform::sleepMs(static_cast<uint32_t>(std::min(50, ms - i)));
    }
    return !canceled(flag);
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string field(const json::Value& obj, std::initializer_list<const char*> keys) {
    for (auto key : keys) if (obj[key].isString() && !obj[key].str().empty()) return obj[key].str();
    return {};
}

bool isSafeSessionId(std::string_view text) {
    return !text.empty() && text.size() <= 128 &&
        std::all_of(text.begin(), text.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '-' || c == '_';
        });
}

std::string trim(std::string_view value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    const auto stop = value.find_last_not_of(" \t\r\n");
    return std::string(value.substr(start, stop - start + 1));
}

std::string normalizedQuery(std::string value) {
    if (value.size() > 8192 || value.find_first_of("\r\n") != std::string::npos) return {};
    const size_t question = value.find('?');
    if (question != std::string::npos) value.erase(0, question + 1);
    if (!value.empty() && value[0] == '?') value.erase(0, 1);
    const auto lowered = lowercase(value);
    if (value.find('=') == std::string::npos ||
        (lowered.find("sessionid=") == std::string::npos &&
         lowered.find("session=") == std::string::npos)) return {};
    return value;
}

std::string parameter(std::string_view query, std::string_view name) {
    const std::string wanted = lowercase(std::string(name));
    for (size_t p = 0; p < query.size();) {
        const size_t end = query.find('&', p);
        const auto piece = query.substr(p, end == std::string_view::npos
                                              ? std::string_view::npos : end - p);
        const size_t equals = piece.find('=');
        if (equals != std::string_view::npos &&
            lowercase(std::string(piece.substr(0, equals))) == wanted) {
            // Session ID values are expected to be plain opaque identifiers,
            // not full browser query strings or credentials.
            return std::string(piece.substr(equals + 1));
        }
        if (end == std::string_view::npos) break;
        p = end + 1;
    }
    return {};
}

std::string sessionIdFromQuery(const std::string& query) {
    auto id = parameter(query, "sessionId");
    if (id.empty()) id = parameter(query, "sessionid");
    if (id.empty()) id = parameter(query, "session");
    return isSafeSessionId(id) ? id : std::string();
}

void uniquePush(std::vector<std::string>& dst, const std::string& value) {
    if (!value.empty() && value.size() <= 8192 && dst.size() < 16 &&
        std::find(dst.begin(), dst.end(), value) == dst.end())
        dst.push_back(value);
}

struct Signals {
    std::vector<std::string> tokens, queries, ids;
    void collect(const json::Value& root, const std::string& key = {}, int depth = 0) {
        if (depth > 12) return;
        if (root.isString()) {
            auto value = root.str();
            const auto lower = lowercase(key);
            if ((lower == "token" || lower == "sessiontoken") && value.size() >= 8)
                uniquePush(tokens, value);
            if (lower == "sessionid" || lower == "sid") {
                if (isSafeSessionId(value)) uniquePush(ids, value);
            }
            auto query = normalizedQuery(std::move(value));
            if (!query.empty()) {
                uniquePush(queries, query);
                uniquePush(ids, sessionIdFromQuery(query));
            }
        } else if (root.isNumber()) {
            const auto lower = lowercase(key);
            if (lower == "sessionid" || lower == "sid") {
                const auto n = root.asInt();
                if (n > 0) uniquePush(ids, std::to_string(n));
            }
        } else if (root.isObject()) {
            for (const auto& pair : root.members()) collect(pair.second, pair.first, depth + 1);
        } else if (root.isArray()) {
            for (const auto& item : root.items()) collect(item, key, depth + 1);
        }
    }
    bool hasReadyQuery() const {
        return std::any_of(queries.begin(), queries.end(), [](const std::string& query) {
            return query.find('&') != std::string::npos && !sessionIdFromQuery(query).empty();
        });
    }
};

struct QueueEvent {
    std::string publicStatus, publicFailure;
    void extract(const json::Value& v, int64_t appId, Signals& signals, int depth = 0) {
        if (depth > 10) return;
        if (v.isArray()) {
            for (const auto& item : v.items()) extract(item, appId, signals, depth + 1);
        }
        if (!v.isObject()) return;
        if (lowercase(field(v, {"type"})) == "queues") {
            const std::string action = lowercase(field(v, {"action"}));
            const auto& body = v["value"].isObject() ? v["value"] : v;
            const auto eventApp = body["appId"].asInt(body["app_id"].asInt());
            if (eventApp > 0 && eventApp != appId) return;
            if (action == "start") {
                signals.collect(body);
                publicStatus = "Máquina Boosteroid atribuída; negociando sessão...";
            } else if (action == "state") {
                const int64_t position = body["position"].asInt();
                publicStatus = "Aguardando máquina Boosteroid";
                if (position > 0 && position < 100000)
                    publicStatus += " - posição " + std::to_string(position);
            } else if (action == "unavailable" || action == "removed" || action == "canceled") {
                publicFailure = "O Boosteroid cancelou a solicitação de máquina.";
            }
        }
        for (const auto& pair : v.members()) extract(pair.second, appId, signals, depth + 1);
    }
};

std::string cleanGateway(std::string host) {
    host = trim(host);
    if (host.rfind("https://", 0) == 0) host.erase(0, 8);
    if (host.rfind("wss://", 0) == 0) host.erase(0, 6);
    const size_t slash = host.find('/');
    if (slash != std::string::npos) host.resize(slash);
    if (host.empty() || host.size() > 255 || host.front() == '-' ||
        host.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:-") != std::string::npos)
        return {};
    return host;
}

int errorCode(const json::Value& v, int depth = 0) {
    if (depth > 10) return 0;
    if (v.isObject()) {
        for (const auto& pair : v.members()) {
            const auto key = lowercase(pair.first);
            if (key == "error_code" || key == "errorcode") {
                if (pair.second.isNumber()) return static_cast<int>(pair.second.asInt());
                if (pair.second.isString()) {
                    try { return std::stoi(pair.second.str()); } catch (...) { return 0; }
                }
            }
            const int nested = errorCode(pair.second, depth + 1);
            if (nested) return nested;
        }
    } else if (v.isArray()) {
        for (const auto& item : v.items()) {
            const int nested = errorCode(item, depth + 1);
            if (nested) return nested;
        }
    }
    return 0;
}

std::string realtimeToken(std::string token) {
    // The WSS URL requires the raw access token, never the Bearer prefix.
    if (token.size() >= 7 && lowercase(token.substr(0, 7)) == "bearer ")
        token.erase(0, 7);
    return trim(token);
}

} // namespace

bool NativeClient::stopStreamingSession() {
    if (!isAuthenticated()) { error_ = ApiError::NoSession; return false; }
    error_ = ApiError::None;
    const auto response = send("POST", "/api/v2/streaming/session/dequeue", "{}", true, 12000);
    if (!response.ok()) error_ = response.status == 0 ? ApiError::Network : ApiError::ServiceRejected;
    return response.ok();
}

bool NativeClient::startStreamingSession(
    int64_t gameId, NativeStreamSession& out, const std::atomic<bool>* cancel,
    const std::function<void(const std::string&)>& progress) {
    out = {};
    if (!isAuthenticated()) { error_ = ApiError::NoSession; return false; }
    if (gameId <= 0 || gameId > 2147483647LL) {
        error_ = ApiError::InvalidResponse;
        return false;
    }
    error_ = ApiError::None;
    auto status = [&](const char* text) { if (progress) progress(text); };
    auto enqueued = false;
    auto abort = [&]() {
        if (enqueued) stopStreamingSession();
        error_ = ApiError::NotReady;
        return false;
    };

    Profile profile;
    if (!getProfile(profile)) return false;

#ifndef _WIN32
    WssClient realtime;
    std::string wssError;
    const auto token = realtimeToken(accessToken_);
    if (!profile.id.empty() && !token.empty() && token.size() < 8192) {
        const std::string path = "/ws?uid=" + net::urlEncode(profile.id) +
                                 "&token=" + net::urlEncode(token);
        if (realtime.connect("cloud.boosteroid.com", path,
                             platform::caBundlePath(), wssError)) {
            status("Acompanhando fila Boosteroid em tempo real...");
        }
        // If the WSS gateway is unavailable, the authenticated REST polling
        // below is still used. Never log path, UID or the access token.
    }
#endif
    json::Value enqueueBody = json::Value::object();
    enqueueBody.set("appId", gameId);
    if (canceled(cancel)) return abort();
    status("Solicitando máquina Boosteroid...");
    const auto enqueue = send("POST", "/api/v2/streaming/session/enqueue",
                              enqueueBody.dump(), true, 18000);
    if (!enqueue.ok()) {
        error_ = enqueue.status == 0 ? ApiError::Network : ApiError::ServiceRejected;
        return false;
    }
    enqueued = true;
    Signals signals;
    if (const auto j = json::parse(enqueue.body)) signals.collect(*j);

    const auto queueDeadline = platform::nowMs() + kMaxQueueSeconds * 1000u;
    uint64_t nextRest = 0, nextPing = platform::nowMs() + 5000;
    while (!canceled(cancel) && signals.tokens.empty() && !signals.hasReadyQuery() &&
           platform::nowMs() < queueDeadline) {
#ifndef _WIN32
        if (realtime.connected()) {
            std::string message, error;
            if (realtime.read_text(message, 100, error)) {
                if (const auto j = json::parse(message)) {
                    QueueEvent event;
                    event.extract(*j, gameId, signals);
                    if (!event.publicStatus.empty() && progress) progress(event.publicStatus);
                    if (!event.publicFailure.empty()) {
                        error_ = ApiError::ServiceRejected;
                        stopStreamingSession();
                        return false;
                    }
                }
            }
        }
        if (realtime.connected() && platform::nowMs() >= nextPing) {
            realtime.send_text("{\"type\":\"ping\"}");
            nextPing = platform::nowMs() + 5000;
        }
#endif
        if (platform::nowMs() >= nextRest) {
            for (const char* path : {"/api/v1/streaming/user/last-session/live",
                                     "/api/v1/streaming/user/last-session",
                                     "/api/v1/streaming/user/active-sessions"}) {
                if (canceled(cancel)) return abort();
                const auto r = send("GET", path, {}, true, 7000);
                if (r.ok()) if (const auto j = json::parse(r.body)) signals.collect(*j);
                if (!signals.tokens.empty() || signals.hasReadyQuery()) break;
            }
            nextRest = platform::nowMs() + 2000;
        }
        if (signals.tokens.empty() && !signals.hasReadyQuery() && !pauseUntil(50, cancel))
            return abort();
    }
    if (canceled(cancel) || (!signals.hasReadyQuery() && signals.tokens.empty())) return abort();

    json::Value started = json::Value::object();
    bool success = false, tryLegacy = false;
    for (const auto& tokenValue : signals.tokens) {
        if (canceled(cancel)) return abort();
        if (tokenValue.empty() || tokenValue.size() > 8192) continue;
        json::Value body = json::Value::object();
        body.set("appId", gameId);
        body.set("sessionToken", tokenValue);
        status("Iniciando a máquina atribuída...");
        const auto response = send("POST", "/api/v2/streaming/session/start", body.dump(), true, 18000);
        if (const auto j = json::parse(response.body)) {
            if (response.ok()) started = *j;
            const int code = errorCode(*j);
            if (code == 340006) { tryLegacy = true; break; }
            if (code == 340007) continue;
        }
        if (response.ok()) { success = true; break; }
        if (response.status != 400) { error_ = ApiError::ServiceRejected; return abort(); }
    }
    if (!success && tryLegacy) {
        const auto response = send("POST", "/api/v1/streaming/session/start",
                                   enqueueBody.dump(), true, 18000);
        if (response.ok()) {
            if (const auto j = json::parse(response.body)) started = *j;
            success = true;
        }
    }
    signals.collect(started);
    if (!success && !signals.hasReadyQuery()) return abort();
    std::string sessionId;
    for (const auto& id : signals.ids) if (isSafeSessionId(id)) { sessionId = id; break; }
    if (sessionId.empty()) return abort();

    status("Obtendo gateway assinado do Boosteroid...");
    const auto detailsDeadline = platform::nowMs() + kMaxDetailsSeconds * 1000u;
    while (!canceled(cancel) && platform::nowMs() < detailsDeadline) {
        const auto path = "/api/v1/streaming/session/details?session=" + net::urlEncode(sessionId);
        const auto response = send("POST", path, {}, true, 12000);
        if (response.ok()) {
            if (const auto j = json::parse(response.body)) {
                const auto& details = (*j)["data"].isObject() ? (*j)["data"] : *j;
                const auto gateway = cleanGateway(field(details, {"gw"}));
                const auto query = field(details, {"queryString", "query_string"});
                if (!gateway.empty() && !query.empty() && query.size() < 8192 &&
                    query.find_first_of("\r\n") == std::string::npos) {
                    out.sessionId = sessionId;
                    out.assignedGateway = gateway;
                    out.signedQuery = query;
                    out.codec = "h264";
                    return true;
                }
            }
        }
        if (!pauseUntil(2000, cancel)) return abort();
    }
    return abort();
}

} // namespace xc::boosteroid
