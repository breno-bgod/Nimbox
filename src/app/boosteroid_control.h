// SPDX-License-Identifier: GPL-3.0-or-later
// NimboPlay -> Boosteroid control plane. Independent from xCloud HTTPS.
// Only public metadata is returned; the OpenStroid process holds tokens.
#pragma once
#include "util/json.h"
#include <string>
namespace xc::app::boosteroid_control {
struct Result {
    int status = 0;
    json::Value data;
    std::string error;
    bool ok() const { return status >= 200 && status < 300; }
};
// Random per-process bearer key. Never persist it or write it to logs.
std::string newKey();
// Connects only to 127.0.0.1:3037. Never called on the SDL render thread.
Result request(const std::string& key, const char* method, const char* path,
               const std::string& jsonBody = {}, int timeoutMs = 12000);
} // namespace xc::app::boosteroid_control
