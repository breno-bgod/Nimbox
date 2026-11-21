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

ChatResponse decodeChatHistory(int status, const std::string& body) {
    ChatResponse result;
    result.status = status;
    if (status < 200 || status >= 300) return result;
    const auto jsonBody = json::parse(body);
    if (!jsonBody || !jsonBody->isObject()) return result;
    const auto& record = *jsonBody;
    result.conversationId = record["conversationId"].str();
    if (result.conversationId.empty() || !record["messages"].isArray()) return result;
    auto items = record["messages"].items();
    if (items.empty() && record["lastMessage"].isObject()) items.push_back(record["lastMessage"]);
    for (const auto& m : items) {
        if (m["type"].str("ContentMessage") != "ContentMessage") continue;
        ChatMessage line{m["sender"].str(), {}, m["timestamp"].str(), m["messageId"].str(), m["isDeleted"].asBool()};
        if (line.id.empty() || (!line.deleted && line.sender.empty())) return result;
        if (!line.deleted) {
            for (const auto& part : m["contentPayload"]["content"]["parts"].items()) {
                const auto type = part["contentType"].str();
                if ((type == "text" || type == "weblink") && part["unsuitableFor"].size() == 0)
                    line.text += part["text"].str();
            }
        }
        result.messages.push_back(std::move(line));
    }
    std::stable_sort(result.messages.begin(),result.messages.end(),[](const ChatMessage& a, const ChatMessage& b) {
        return a.timestamp < b.timestamp;
    });
    result.ok = true;
    return result;
}

ChatResponse decodeChatReceipt(int status, const std::string& body, const std::string& sender, const std::string& text) {
    ChatResponse result;
    result.status = status;
    if (status < 200 || status >= 300) return result;
    const auto receipt = json::parse(body);
    if (!receipt) return result;
    const auto id = (*receipt)["messageId"].str();
    result.conversationId = (*receipt)["conversationId"].str();
    if (id.empty() || result.conversationId.empty()) return result;
    result.messages.push_back({sender,text,{},id});
    result.ok = true;
    return result;
}

void ChatHistory::merge(const ChatResponse& update) {
    if (!update.ok) return;
    conversationId = update.conversationId;
    for (const auto& msg : update.messages) {
        auto known = std::find_if(messages.begin(),messages.end(),[&](const ChatMessage& item){ return item.id == msg.id; });
        if (known != messages.end()) {
            if (!msg.deleted && msg.timestamp.empty()) continue;
            messages.erase(known);
        }
        if (msg.deleted) continue;
        auto position = std::find_if(messages.begin(),messages.end(),[&](const ChatMessage& item){
            if (!msg.timestamp.empty() && !item.timestamp.empty()) return item.timestamp > msg.timestamp;
            // The receipt has no timestamp. Decimal Xbox message IDs preserve send order.
            if (msg.id.find_first_not_of("0123456789") != std::string::npos ||
                item.id.find_first_not_of("0123456789") != std::string::npos) return false;
            return item.id.size() == msg.id.size() ? item.id > msg.id : item.id.size() > msg.id.size();
        });
        messages.insert(position,msg);
    }
    constexpr int limit = 100;
    if (messages.size() > limit) messages.erase(messages.begin(),messages.end()-limit);
}

ChatResponse readConversation(const std::string& auth, const std::string& peer, const std::string& conversationId) {
    net::Request req;
    if (!conversationRequest(req,auth,peer)) return {};
    req.headers.emplace_back("Cache-Control","no-cache");
    const auto readById = [&](const std::string& id) {
        if (id.size() != 36 || id.find_first_not_of("0123456789abcdefABCDEF-") != std::string::npos) return ChatResponse{};
        auto byId = req;
        byId.url = "https://xblmessaging.xboxlive.com/network/Xbox/users/me/conversations/OneToOne/" + id + "?maxItems=64";
        byId.headers = {{"Authorization",auth},{"x-xbl-contract-version","2"},{"Cache-Control","no-cache"}};
        const auto r = net::perform(byId);
        return decodeChatHistory(r.status,r.body);
    };
    if (!conversationId.empty()) {
        auto page = readById(conversationId);
        if (page.ok) return page;
    }
    req.url += "?maxItems=64";
    const auto r = net::perform(req);
    auto result = decodeChatHistory(r.status,r.body);
    if (!result.ok && !result.conversationId.empty() && result.conversationId != conversationId)
        return readById(result.conversationId);
    if (r.status == 404 && conversationId.empty()) {
        // Resolve existing threads by their two participants when the peer route misses.
        req.url = "https://xblmessaging.xboxlive.com/network/Xbox/users/me/inbox?maxItems=100";
        const auto inbox = net::perform(req);
        if (inbox.ok()) {
            const auto listing = json::parse(inbox.body);
            if (listing) for (const auto& thread : (*listing)["primary"]["conversations"].items()) {
                if (thread["type"].str() != "OneToOne" || thread["participants"].size() != 2) continue;
                for (const auto& participant : thread["participants"].items())
                    if (participant.str() == peer) return readById(thread["conversationId"].str());
            }
        }
    }
    return result;
}

ChatResponse sendChatMessage(const std::string& auth, const std::string& peer, const std::string& sender, const std::string& text) {
    net::Request req;
    if (!conversationRequest(req,auth,peer) || text.size() > 256 || text.find_first_not_of(' ') == std::string::npos)
        return {};
    req.method = "POST";
    req.headers.emplace_back("Content-Type","application/json");
    auto part = json::Value::object();
    part.set("contentType","text").set("version",0).set("text",text);
    auto parts = json::Value::array();
    parts.push(std::move(part));
    req.body = json::Value::object().set("parts",std::move(parts)).dump();
    const auto r = net::perform(req);
    return decodeChatReceipt(r.status,r.body,sender,text);
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
