// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#include "xcloud/social.h"
#include <algorithm>

#include "net/http.h"
#include "util/json.h"
#include "util/log.h"

namespace xc::xcloud {

namespace {
bool conversationRequest(net::Request& req, const std::string& auth, const std::string& peer) {
    if (auth.empty() || peer.empty() || peer.size() > 20 || peer.find_first_not_of("0123456789") != std::string::npos) return false;
    req.url = "https://xblmessaging.xboxlive.com/network/Xbox/users/me/conversations/users/xuid(" + peer + ")";
    req.headers = {{"Authorization", auth}, {"x-xbl-contract-version", "1"}};
    req.timeoutMs = 6000;
    return true;
}
}

bool readConversation(const std::string& auth, const std::string& peer, std::vector<ChatMessage>& messages) {
    net::Request req;
    if (!conversationRequest(req, auth, peer)) return false;
    req.url += "?maxItems=24";
    auto response = net::perform(req);
    if (response.status == 404) { messages.clear(); return true; }
    if (!response.ok()) return false;
    auto body = json::parse(response.body);
    if (!body || !body->isObject() || !body->has("conversationId")) return false;
    std::vector<ChatMessage> result;
    for (const auto& message : (*body)["messages"].items()) {
        if (message["isDeleted"].asBool()) continue;
        ChatMessage line{message["sender"].str(), {}, message["timestamp"].str()};
        for (const auto& part : message["contentPayload"]["content"]["parts"].items()) {
            if (part["contentType"].str() == "text" && part["unsuitableFor"].size() == 0)
                line.text += part["text"].str();
        }
        if (!line.text.empty()) result.push_back(std::move(line));
    }
    std::stable_sort(result.begin(), result.end(), [](const ChatMessage& a, const ChatMessage& b) {
        return a.timestamp < b.timestamp;
    });
    messages = std::move(result);
    return true;
}

bool sendChatMessage(const std::string& auth, const std::string& peer, const std::string& text) {
    net::Request req;
    if (!conversationRequest(req, auth, peer) || text.size() > 256 || text.find_first_not_of(' ') == std::string::npos)
        return false;
    req.method = "POST";
    req.headers.emplace_back("Content-Type", "application/json");
    auto part = json::Value::object();
    part.set("contentType","text").set("version",0).set("text",text);
    auto parts = json::Value::array();
    parts.push(std::move(part));
    req.body = json::Value::object().set("parts",std::move(parts)).dump();
    return net::perform(req).ok();
}

std::string fetchOwnPresence(const std::string& auth) {
    if (auth.empty()) return {};
    net::Request req;
    req.method = "GET";
    req.url = "https://userpresence.xboxlive.com/users/me";
    req.headers = {{"Authorization", auth}, {"x-xbl-contract-version", "3"}};
    const auto response = net::perform(req);
    if (!response.ok()) return {};
    const auto record = json::parse(response.body);
    return record ? (*record)["state"].str() : std::string();
}

bool fetchFriendsPresence(const std::string& xblAuth, std::vector<FriendPresence>& out, std::string& err,
                          const std::string& language) {
    if (xblAuth.empty()) {
        err = "xblAuth empty";
        return false;
    }
    net::Request req;
    req.method = "GET";
    req.url = "https://peoplehub.xboxlive.com/users/me/people/social/decoration/detail,presencedetail,preferredcolor";
    req.headers = {
        {"Authorization", xblAuth},
        {"x-xbl-contract-version", "2"},
        {"Accept-Language", language.empty() ? "en-US" : language}
    };

    auto r = net::perform(req);
    auto j = json::parse(r.body);
    if (!r.ok() || !j) {
        err = "peoplehub: " + (r.status ? "HTTP " + std::to_string(r.status) : r.error);
        XC_LOGW("%s", err.c_str());
        return false;
    }

    for (const auto& p : (*j)["people"].items()) {
        FriendPresence fp;
        fp.xuid = p["xuid"].str();
        fp.gamertag = p["gamertag"].str();
        fp.gamerpicUrl = p["displayPicRaw"].str();
        fp.presenceState = p["presenceState"].str();
        fp.presenceText = p["presenceText"].str();

        // Examine presenceDetails for the active game title
        for (const auto& d : p["presenceDetails"].items()) {
            std::string tid;
            if (d["TitleId"].isString()) {
                tid = d["TitleId"].str();
            } else if (d["TitleId"].isNumber()) {
                tid = std::to_string(d["TitleId"].asInt());
            }

            // Exclude Xbox dashboard / Home shell ("750323071", "1022622766")
            if (tid.empty() || tid == "750323071" || tid == "1022622766") continue;

            bool isGame = d["IsGame"].asBool();
            std::string state = d["State"].str();

            if (isGame || state == "Active") {
                fp.titleId = tid;
                fp.device = d["Device"].str();
                fp.isGame = isGame;
                if (isGame) break;  // Prefer explicit game entry
            }
        }

        out.push_back(std::move(fp));
    }

    XC_LOGI("peoplehub: retrieved %zu friends", out.size());
    return true;
}

}  // namespace xc::xcloud
