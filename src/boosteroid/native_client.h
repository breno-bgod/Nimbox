// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NimboPlay contributors
// Native Boosteroid Android TV-compatible account and catalog client.
//
// Protocol behavior researched against ZERODROID (Apache-2.0):
// https://github.com/elzerdodata/ZERODROID
// No browser/Electron, no Xbox GSSV, no service secrets in the UI.
#pragma once

#include "boosteroid/native_contract.h"
#include "net/http.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace xc::boosteroid {

enum class LoginResult { Pending, Authorized, Expired, RateLimited, NetworkError, Rejected };
enum class ApiError {
    None, NoSession, Network, InvalidResponse, Unauthorized, RateLimited,
    Storage, ServiceRejected, InvalidDevice, NotReady
};

struct Profile {
    std::string id;
    std::string nickname;
    std::string avatarUrl;
};

// Kept in the provider session only. Signed gateway URL and tokens must never
// be copied into UI model, logs, bug reports or the Xbox stream.
struct NativeStreamSession {
    std::string sessionId;
    std::string assignedGateway;
    std::string signedQuery;
    std::string codec = "h264";
};

// Use from a worker thread, never SDL/UI render thread.
// A client instance belongs to one Boosteroid account/device identity.
// No session key/token is returned through public methods.
class NativeClient {
public:
    explicit NativeClient(std::string sessionPath = {}, bool persist = true);
    NativeClient(const NativeClient&) = delete;
    NativeClient& operator=(const NativeClient&) = delete;

    // A Boosteroid Android TV device authorization is a local UUID and an
    // official HTTPS QR validation URL. The service issues tokens on polling.
    bool beginDeviceLogin(DeviceAuthorization& qr);
    LoginResult pollDeviceLogin();
    bool restoreSession();
    bool isAuthenticated() const { return !accessToken_.empty(); }

    bool getProfile(Profile& profile);
    bool getInstalledGames(std::vector<Game>& out, int page = 1, int pageSize = 50);
    // Calls native Android TV enqueue/queue/start/session-details operations.
    // Only call after the native media transport can consume the session.
    // This is currently NOT invoked by Horizon's disabled Play action.
    bool startStreamingSession(int64_t gameId, NativeStreamSession& session,
                               const std::atomic<bool>* cancel = nullptr,
                               const std::function<void(const std::string&)>& progress = {});
    bool stopStreamingSession();
    // Explicit sign-out; also clears the encrypted account cache.
    void logout();
    ApiError error() const { return error_; }
    int httpStatus() const { return lastHttpStatus_; }
    const std::string& deviceId() const { return deviceId_; }
    bool hasPendingAuthorization() const { return !pendingCode_.empty(); }

private:
    net::Response send(const char* method, const std::string& relativePath,
                       const std::string& body = {}, bool authenticated = false,
                       int timeoutMs = 15000);
    bool ensureDeviceId();
    bool persistSession() const;
    bool parseAccount(const json::Value& body);
    void clearSecrets();

    std::string sessionPath_;
    bool persist_ = true;
    std::string deviceId_;
    std::string pendingCode_;
    std::string accessToken_, refreshToken_, authorizationData_;
    ApiError error_ = ApiError::None;
    int lastHttpStatus_ = 0;
};

} // namespace xc::boosteroid
