// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NimboPlay contributors
// Independent Boosteroid client on the PSBox portable HTTPS/platform layers.
// Android TV protocol behavior researched from ZERODROID (Apache-2.0).
#include "boosteroid/native_client.h"

#include "auth/account_seal.h"
#include "platform/platform.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <initializer_list>
#include <cstdio>
#include <limits>
#include <string_view>
#include <utility>

namespace xc::boosteroid {
namespace {
constexpr const char* kBase = "https://cloud.boosteroid.com";
constexpr const char* kQrSync = "/api/v1/auth/login/qr-code/sync";
constexpr const char* kQrVerify = "/api/v1/auth/login/qr-code/validate?auth-code=";
constexpr int kTvClientId = 6;

bool safeHeader(std::string_view text) {
    return text.size() <= 8192 && std::none_of(text.begin(), text.end(),
        [](unsigned char c) { return c < 32 || c == 127; });
}

int fromHex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string decodedToken(std::string_view source) {
    if (source.size() > 8192) return {};
    std::string result;
    result.reserve(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '%' && i + 2 < source.size()) {
            const int a = fromHex(source[i + 1]), b = fromHex(source[i + 2]);
            if (a >= 0 && b >= 0) {
                result.push_back(static_cast<char>((a << 4) | b));
                i += 2;
                continue;
            }
        }
        result.push_back(source[i] == '+' ? ' ' : source[i]);
    }
    const size_t from = result.find_first_not_of(" \t\r\n");
    if (from == std::string::npos) return {};
    const size_t to = result.find_last_not_of(" \t\r\n");
    return result.substr(from, to - from + 1);
}

std::string authorizationValue(const std::string& token) {
    const std::string value = decodedToken(token);
    if (value.empty() || !safeHeader(value)) return {};
    if (value.size() >= 7) {
        std::string prefix = value.substr(0, 7);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (prefix == "bearer ") return "Bearer " + value.substr(7);
    }
    const size_t a = value.find('.'), b = a == std::string::npos
        ? std::string::npos : value.find('.', a + 1);
    return b == std::string::npos ? value : "Bearer " + value;
}

const json::Value& payloadOf(const json::Value& root) {
    if (root["data"].isObject()) return root["data"];
    if (root["result"].isObject()) return root["result"];
    return root;
}

std::string idString(const json::Value& v) {
    if (v.isString()) return v.str();
    if (v.isNumber()) {
        const auto n = v.asInt();
        if (n > 0) return std::to_string(n);
    }
    return {};
}

std::string firstString(const json::Value& v, std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (v[key].isString() && !v[key].str().empty()) return v[key].str();
    }
    return {};
}

std::string authDataFrom(const json::Value& payload) {
    const auto direct = firstString(payload,
        {"authorization_data", "authorizationData", "boosteroid_auth", "boosteroidAuth"});
    if (!direct.empty()) return direct;
    const auto& user = payload["user_data"];
    if (user.isString()) return user.str();
    if (user.isObject()) {
        return firstString(user, {"authorization_data", "authorizationData",
                                  "boosteroid_auth", "boosteroidAuth"});
    }
    return {};
}

bool validUuid(std::string_view uuid) {
    if (uuid.size() != 36) return false;
    for (size_t i = 0; i < uuid.size(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid[i] != '-') return false;
        } else if (fromHex(uuid[i]) < 0) return false;
    }
    return true;
}

std::string newUuid() {
    std::array<uint8_t, 16> bytes{};
    if (!platform::randomBytes(bytes.data(), bytes.size())) return {};
    bytes[6] = (bytes[6] & 0x0fU) | 0x40U;
    bytes[8] = (bytes[8] & 0x3fU) | 0x80U;
    char text[37];
    const int rc = std::snprintf(text, sizeof text,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],
        bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]);
    return rc == 36 ? text : std::string();
}

ApiError responseError(const net::Response& r) {
    if (r.status == 0) return ApiError::Network;
    if (r.status == 401 || r.status == 403) return ApiError::Unauthorized;
    if (r.status == 429) return ApiError::RateLimited;
    return ApiError::ServiceRejected;
}

} // namespace

NativeClient::NativeClient(std::string sessionPath, bool persist)
    : sessionPath_(sessionPath.empty()
          ? platform::dataDir() + "/boosteroid_account.json"
          : std::move(sessionPath)), persist_(persist) {}

bool NativeClient::ensureDeviceId() {
    if (validUuid(deviceId_)) return true;
    deviceId_ = newUuid();
    if (deviceId_.empty()) error_ = ApiError::InvalidDevice;
    return !deviceId_.empty();
}

bool NativeClient::beginDeviceLogin(DeviceAuthorization& qr) {
    qr = {};
    error_ = ApiError::None;
    lastHttpStatus_ = 0;
    pendingCode_.clear();
    if (!ensureDeviceId()) return false;
    pendingCode_ = newUuid();
    if (pendingCode_.empty()) {
        error_ = ApiError::InvalidDevice;
        return false;
    }
    json::Value authorization = json::Value::object();
    authorization.set("validationUrl", std::string(kBase) + kQrVerify + pendingCode_);
    authorization.set("status", "pending");
    authorization.set("pollIntervalMs", 3000);
    return publicAuthorization(authorization, qr);
}

net::Response NativeClient::send(const char* method, const std::string& relativePath,
                                 const std::string& body, bool authenticated, int timeoutMs) {
    if (relativePath.empty() || relativePath[0] != '/' ||
        relativePath.find_first_of("\r\n#") != std::string::npos ||
        relativePath.rfind("//", 0) == 0 || !ensureDeviceId()) {
        error_ = ApiError::InvalidDevice;
        return {};
    }
    net::Request request;
    request.method = method;
    request.url = std::string(kBase) + relativePath;
    request.timeoutMs = timeoutMs;
    request.followRedirects = false;
    request.body = body;
    request.headers = {
        {"Accept","application/json"},
        {"User-Agent","BoosteroidAndroidTVClient v.2.5.10.tv; Android 12; NimboPlay"},
        {"Device-Name","NimboPlay"},
        {"Device-Uniq-Id",deviceId_},
        {"Nonce","0"},
        {"Cookie","boosteroid_entrypoint_source=1;boosteroid_entrypoint_page=1"}
    };
    if (request.method == "POST") request.headers.emplace_back("Content-Type","application/json");
    if (authenticated) {
        const std::string auth = authorizationValue(accessToken_);
        if (auth.empty()) {
            error_ = ApiError::NoSession;
            return {};
        }
        request.headers.emplace_back("Authorization",auth);
        if (!authorizationData_.empty() && safeHeader(authorizationData_))
            request.headers.emplace_back("Authorization-Data",authorizationData_);
    }
    // net::perform implements certificate/hostname validation on PS5+Android.
    // It is never called on the UI/render thread.
    auto response = net::perform(request);
    lastHttpStatus_ = response.status;
    if (response.status == 0) error_ = ApiError::Network;
    return response;
}

bool NativeClient::parseAccount(const json::Value& response) {
    const auto& v = payloadOf(response);
    const std::string access = firstString(v, {"access_token", "accessToken"});
    const std::string refresh = firstString(v, {"refresh_token", "refreshToken"});
    const std::string authData = authDataFrom(v);
    if (access.empty() || refresh.empty() ||
        !safeHeader(authorizationValue(access)) ||
        (!authData.empty() && !safeHeader(authData))) return false;
    accessToken_ = access;
    refreshToken_ = refresh;
    authorizationData_ = authData;
    return true;
}

LoginResult NativeClient::pollDeviceLogin() {
    if (!validUuid(pendingCode_)) {
        error_ = ApiError::NotReady;
        return LoginResult::Rejected;
    }
    json::Value data = json::Value::object();
    data.set("auth-code", pendingCode_);
    data.set("clientId", kTvClientId);
    error_ = ApiError::None;
    auto r = send("POST", kQrSync, data.dump(), false, 15000);
    if (r.status == 401 || r.status == 202 || r.status == 204)
        return LoginResult::Pending;
    if (r.status == 429) {
        error_ = ApiError::RateLimited;
        return LoginResult::RateLimited;
    }
    if (r.status == 422) {
        error_ = ApiError::ServiceRejected;
        pendingCode_.clear();
        return LoginResult::Expired;
    }
    if (r.status == 0) {
        error_ = ApiError::Network;
        return LoginResult::NetworkError;
    }
    if (r.status != 200) {
        error_ = responseError(r);
        return LoginResult::Rejected;
    }
    const auto json = json::parse(r.body);
    if (!json || !parseAccount(*json)) {
        error_ = ApiError::InvalidResponse;
        return LoginResult::Rejected;
    }
    pendingCode_.clear();
    if (!persistSession()) error_ = ApiError::Storage;
    return LoginResult::Authorized;
}

bool NativeClient::persistSession() const {
    if (!persist_) return true;
    if (accessToken_.empty() || refreshToken_.empty() || !validUuid(deviceId_)) return false;
    json::Value data = json::Value::object();
    data.set("version", 1);
    data.set("device_id", deviceId_);
    data.set("access_token", accessToken_);
    data.set("refresh_token", refreshToken_);
    data.set("authorization_data", authorizationData_);
    const std::string sealed = auth::sealAccount(data.dump());
    if (sealed.empty() || !platform::writeFileAtomic(sessionPath_, sealed)) return false;
    platform::makePrivate(sessionPath_);
    return true;
}

bool NativeClient::restoreSession() {
    error_ = ApiError::NoSession;
    if (!persist_) return false;
    std::string ciphertext;
    if (!platform::readFile(sessionPath_, ciphertext) || ciphertext.empty()) return false;
    std::string decrypted;
    if (!auth::unsealAccount(ciphertext, decrypted)) {
        error_ = ApiError::Storage;
        return false;
    }
    const auto data = json::parse(decrypted);
    if (!data || !data->isObject() || (*data)["version"].asInt() != 1 ||
        !validUuid((*data)["device_id"].str())) {
        error_ = ApiError::InvalidResponse;
        return false;
    }
    deviceId_ = (*data)["device_id"].str();
    if (!parseAccount(*data)) {
        error_ = ApiError::InvalidResponse;
        return false;
    }
    pendingCode_.clear();
    error_ = ApiError::None;
    return true;
}

bool NativeClient::getProfile(Profile& out) {
    out = {};
    error_ = ApiError::None;
    if (!isAuthenticated()) { error_ = ApiError::NoSession; return false; }
    const auto r = send("GET", "/api/v1/user", {}, true);
    if (!r.ok()) { error_ = responseError(r); return false; }
    const auto data = json::parse(r.body);
    if (!data || !data->isObject()) { error_ = ApiError::InvalidResponse; return false; }
    const auto& user = payloadOf(*data);
    out.id = idString(user["id"]);
    out.nickname = firstString(user, {"nickname", "name", "displayName"});
    if (out.nickname.size() > 256) out.nickname.resize(256);
    const auto avatar = firstString(user, {"avatar_url", "avatar"});
    if (avatar.rfind("https://", 0) == 0 && avatar.find_first_of("?#") == std::string::npos &&
        avatar.size() <= 2048) out.avatarUrl = avatar;
    return true;
}

bool NativeClient::getInstalledGames(std::vector<Game>& out, int page, int pageSize) {
    out.clear();
    error_ = ApiError::None;
    if (!isAuthenticated()) { error_ = ApiError::NoSession; return false; }
    if (page < 1 || page > 100 || pageSize < 1 || pageSize > 100) {
        error_ = ApiError::InvalidResponse;
        return false;
    }
    const std::string path = "/api/v1/boostore/applications/installed?page=" +
        std::to_string(page) + "&paginate=" + std::to_string(pageSize);
    const auto r = send("GET", path, {}, true, 25000);
    if (!r.ok()) { error_ = responseError(r); return false; }
    if (r.body.size() > 8u * 1024u * 1024u) { error_ = ApiError::InvalidResponse; return false; }
    const auto data = json::parse(r.body);
    if (!data) { error_ = ApiError::InvalidResponse; return false; }
    if (!catalogHasGameArray(*data)) { error_ = ApiError::InvalidResponse; return false; }
    out = publicGames(*data);
    return true;
}

void NativeClient::clearSecrets() {
    accessToken_.clear();
    refreshToken_.clear();
    authorizationData_.clear();
    pendingCode_.clear();
}

void NativeClient::logout() {
    clearSecrets();
    error_ = ApiError::None;
    if (persist_) std::remove(sessionPath_.c_str());
}

} // namespace xc::boosteroid
