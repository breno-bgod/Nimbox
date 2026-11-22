// SPDX-License-Identifier: GPL-3.0-or-later
// Native Boosteroid gateway signalling; uses the same libdatachannel
// H264 jitter receiver and Opus depacketizer as PSBox, never Xbox GSSV.
#include "boosteroid/native_gateway.h"

#include "boosteroid/control_input.h"
#include "boosteroid/native_udp.h"
#include "boosteroid/wss_client.h"
#include "net/http.h"
#include "platform/platform.h"
#include "stream/h264_receiver.h"
#include "stream/video_stats.h"

#include <rtc/rtc.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <exception>
#include <initializer_list>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace xc::boosteroid {
namespace {
std::string field(const json::Value& val, std::initializer_list<const char*> keys) {
    for (const auto* key : keys)
        if (val[key].isString()) return val[key].str();
    return {};
}

void findNestedStrings(const json::Value& value, const char* name,
                       std::vector<std::string>& found, int depth = 0) {
    if (depth > 9 || found.size() >= 32) return;
    if (value.isObject()) {
        for (const auto& [key, child] : value.members()) {
            if (key == name && child.isString()) {
                const auto text = child.str();
                if (text.size() < 65536) found.push_back(text);
            } else {
                findNestedStrings(child, name, found, depth + 1);
            }
        }
    } else if (value.isArray()) {
        for (const auto& item : value.items())
            findNestedStrings(item, name, found, depth + 1);
    }
}

std::string findSdp(const json::Value& data) {
    std::vector<std::string> values;
    findNestedStrings(data, "sdp", values);
    for (const auto& s : values) if (s.rfind("v=0", 0) == 0) return s;
    return {};
}

bool validHost(const std::string& host) {
    return !host.empty() && host.size() <= 253 &&
        host.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-") ==
                std::string::npos &&
        host != "localhost" && host != "127.0.0.1";
}

std::string hostOnly(const std::string& gateway) {
    const auto colon = gateway.find(':');
    return colon == std::string::npos ? gateway : gateway.substr(0, colon);
}

std::string makeUuid() {
    uint8_t b[16]{};
    if (!platform::randomBytes(b, sizeof b)) return {};
    b[6] = (b[6] & 0x0f) | 0x40;
    b[8] = (b[8] & 0x3f) | 0x80;
    char uuid[37]{};
    const int n = std::snprintf(uuid, sizeof uuid,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
        b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    return n == 36 ? uuid : std::string();
}

net::Response signalRequest(const std::string& host, const std::string& path,
                            const std::string& method = "GET",
                            const std::string& body = {}) {
    net::Request request;
    request.url = "https://" + host + "/webrtc" + path;
    request.method = method;
    request.timeoutMs = 12000;
    request.followRedirects = false;
    request.body = body;
    request.headers = {{"Accept", "application/json"}};
    if (method == "POST")
        request.headers.emplace_back("Content-Type", "application/json");
    return net::perform(request);
}

void collectCandidates(const json::Value& value, std::vector<std::string>& out,
                       int depth = 0) {
    if (depth > 8 || out.size() >= 48) return;
    if (value.isString()) {
        const auto s = value.str();
        if (s.size() <= 4096 && s.find("candidate:") != std::string::npos)
            out.push_back(s);
    } else if (value.isArray()) {
        for (const auto& entry : value.items()) collectCandidates(entry, out, depth + 1);
    } else if (value.isObject()) {
        for (const auto& entry : value.members())
            collectCandidates(entry.second, out, depth + 1);
    }
}
}

ControlEvent parseControlEvent(const json::Value& msg) {
    ControlEvent ev{};
    if (!msg.isObject()) return ev;
    const auto type = field(msg, {"type"});
    const auto action = field(msg, {"action"});
    if (type == "settings" && action == "webrtc")
        ev.type = ControlEvent::Type::StartWebRtc;
    else if (type == "settings" && action == "udpforward")
        ev.type = ControlEvent::Type::NativeUdpRequested;
    else if (type == "stream" && action == "key")
        ev.type = ControlEvent::Type::StreamKey;
    else if (type == "stream" && action == "stop")
        ev.type = ControlEvent::Type::SessionEnded;
    else if (type == "controller" && action == "connected") {
        const int64_t id = msg["id"].asInt();
        if (id > 0 && id <= 8) {
            ev.type = ControlEvent::Type::ControllerAssigned;
            ev.controllerId = static_cast<int>(id);
        }
    }
    return ev;
}

std::string deviceStatusReply() {
    json::Value msg = json::Value::object();
    msg.set("type", "stream");
    msg.set("action", "status");
    msg.set("value", "ok");
    json::Value params = json::Value::object();
    params.set("type", "androidTV");
    params.set("ver", "v.2.5.10.tv");
    params.set("gpu", "NimboPlay");
    params.set("proto", 2);
    params.set("codec", "h264");
    params.set("framerate_max", 60);
    params.set("bitrate_max", 0);
    params.set("hdr", false);
    msg.set("params", std::move(params));
    return msg.dump();
}

bool NativeGateway::run(const NativeStreamSession& session, const EncodedMediaSink& sink,
                        const std::atomic<bool>& cancel, PadSnapshot pad,
                        Progress progress, std::string& error, PictureReady pictureReady) {
    stats_ = {};
    error.clear();
    if (!sink.h264AnnexB || !sink.opus) {
        error = "Reprodutor H264/Opus não conectado ao Boosteroid.";
        return false;
    }
    const std::string host = hostOnly(session.assignedGateway);
    if (!validHost(host) || session.signedQuery.empty() ||
        session.signedQuery.size() > 8192 || session.sessionId.empty()) {
        error = "Sessão Boosteroid sem gateway assinado válido.";
        return false;
    }

    const std::string query = session.signedQuery.front() == '?'
                                ? session.signedQuery.substr(1) : session.signedQuery;
    if (query.find_first_of("\r\n") != std::string::npos) {
        error = "A assinatura do gateway contém caracteres inválidos.";
        return false;
    }
    const std::string path = "/native?" + query +
        "&x=1920&y=1080&lang=pt&tv=1&clientType=native&devType=tv&os=atv";
    WssClient control;
    if (!control.connect(session.assignedGateway, path,
                         platform::caBundlePath(), error)) {
        if (error.empty()) error = "Falha ao conectar no canal Boosteroid.";
        return false;
    }
    if (progress) progress("Gateway Boosteroid conectado; negociando WebRTC...");
    std::shared_ptr<rtc::PeerConnection> peer;
    std::shared_ptr<rtc::Track> video, audio;
    struct LocalCandidates {
        std::mutex mutex;
        std::vector<rtc::Candidate> values;
    };
    const auto gathered = std::make_shared<LocalCandidates>();
    auto receiveStats = std::make_shared<stream::VideoReceiveStats>();
    struct PeerSignals {
        std::atomic<bool> connected{false}, failed{false}, gatheringDone{false};
    };
    const auto signals = std::make_shared<PeerSignals>();
    struct MediaGuard {
        std::mutex mutex;
        bool active = true;
        EncodedMediaSink sink;
        std::atomic<uint64_t> frames{0}, opus{0};
    };
    const auto mediaGuard = std::make_shared<MediaGuard>();
    mediaGuard->sink = sink;
    EncodedMediaSink nativeSink;
    nativeSink.h264AnnexB = [mediaGuard](const uint8_t* data, size_t len,
                                         uint32_t timestamp) {
        std::lock_guard<std::mutex> lock(mediaGuard->mutex);
        if (mediaGuard->active &&
            deliverH264(mediaGuard->sink, data, len, timestamp))
            ++mediaGuard->frames;
    };
    nativeSink.opus = [mediaGuard](const uint8_t* data, size_t len,
                                  uint32_t timestamp) {
        std::lock_guard<std::mutex> lock(mediaGuard->mutex);
        if (mediaGuard->active &&
            deliverOpus(mediaGuard->sink, data, len, timestamp))
            ++mediaGuard->opus;
    };
    NativeUdp udp(std::move(nativeSink));
    ControllerMapper mapper;
    int controllerId = 0;
    const std::string peerId = makeUuid();
    if (peerId.empty()) {
        error = "Falha ao gerar identificador WebRTC.";
        control.close();
        return false;
    }

    const auto endpoint = [&](const char* route) {
        return std::string(route) + "?peerid=" + net::urlEncode(peerId) +
               "&sessionId=" + net::urlEncode(session.sessionId);
    };
    const auto makePeer = [&]() -> bool {
        try {
            rtc::Configuration config;
            config.disableAutoNegotiation = true;
            // The service provides ICE/TURN servers; fall back to STUN for
            // accounts whose gateway returns no ICE configuration.
            const auto response = signalRequest(host,
                "/api/getIceServers?sessionId=" + net::urlEncode(session.sessionId));
            if (response.ok()) {
                if (const auto json = json::parse(response.body)) {
                    std::vector<std::string> urls;
                    findNestedStrings(*json, "urls", urls);
                    findNestedStrings(*json, "url", urls);
                    for (const auto& url : urls) {
                        if (url.rfind("stun:", 0) == 0 ||
                            url.rfind("turn:", 0) == 0 ||
                            url.rfind("turns:", 0) == 0) {
                            try { config.iceServers.emplace_back(url); }
                            catch (...) {}
                        }
                    }
                }
            }
            if (config.iceServers.empty())
                config.iceServers.emplace_back("stun:stun.l.google.com:19302");

            peer = std::make_shared<rtc::PeerConnection>(config);
            peer->onLocalCandidate([gathered](rtc::Candidate candidate) {
                std::lock_guard<std::mutex> lock(gathered->mutex);
                gathered->values.push_back(std::move(candidate));
            });
            peer->onGatheringStateChange([signals](rtc::PeerConnection::GatheringState state) {
                if (state == rtc::PeerConnection::GatheringState::Complete)
                    signals->gatheringDone = true;
            });
            peer->onStateChange([signals](rtc::PeerConnection::State state) {
                if (state == rtc::PeerConnection::State::Connected) signals->connected = true;
                if (state == rtc::PeerConnection::State::Failed ||
                    state == rtc::PeerConnection::State::Closed) signals->failed = true;
            });
            rtc::Description::Audio a("0", rtc::Description::Direction::RecvOnly);
            a.addOpusCodec(111);
            audio = peer->addTrack(a);
            audio->setMediaHandler(std::make_shared<rtc::OpusRtpDepacketizer>());
            audio->onFrame([mediaGuard](rtc::binary data, rtc::FrameInfo info) {
                std::lock_guard<std::mutex> lock(mediaGuard->mutex);
                if (!mediaGuard->active) return;
                if (deliverOpus(mediaGuard->sink,
                                reinterpret_cast<const uint8_t*>(data.data()),
                                data.size(), info.timestamp)) ++mediaGuard->opus;
            });

            rtc::Description::Video v("1", rtc::Description::Direction::RecvOnly);
            v.addH264Codec(100, "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=64002a");
            v.addH264Codec(102, "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f");
            video = peer->addTrack(v);
            // The jitter receiver may outlive the PeerConnection's onFrame
            // callback, so its statistics must outlive the receiver itself.
            auto receiver = std::shared_ptr<stream::H264Receiver>(
                new stream::H264Receiver([] {}, *receiveStats),
                [receiveStats](stream::H264Receiver* p) { delete p; });
            video->setMediaHandler(receiver);
            video->onFrame([mediaGuard, receiver](rtc::binary data, rtc::FrameInfo info) {
                std::lock_guard<std::mutex> lock(mediaGuard->mutex);
                if (!mediaGuard->active) return;
                if (deliverH264(mediaGuard->sink,
                                reinterpret_cast<const uint8_t*>(data.data()),
                                data.size(), info.timestamp)) ++mediaGuard->frames;
            });

            peer->setLocalDescription(rtc::Description::Type::Offer);
            const uint64_t gatherDeadline = platform::nowMs() + 12000;
            while (!signals->gatheringDone && !cancel && platform::nowMs() < gatherDeadline)
                platform::sleepMs(30);
            if (cancel) return false;
            auto local = peer->localDescription();
            if (!local) { error = "WebRTC não gerou SDP local."; return false; }
            json::Value body = json::Value::object();
            body.set("type", "offer");
            body.set("sdp", std::string(*local));
            auto answerResp = signalRequest(host, endpoint("/api/call"), "POST", body.dump());
            if (!answerResp.ok()) {
                error = "Gateway recusou a oferta WebRTC.";
                return false;
            }
            const auto answerData = json::parse(answerResp.body);
            const auto answer = answerData ? findSdp(*answerData) : std::string();
            if (answer.empty()) {
                error = "Gateway não retornou SDP WebRTC válido.";
                return false;
            }
            peer->setRemoteDescription(rtc::Description(answer, rtc::Description::Type::Answer));
            // Some gateway versions ignore ICE candidates embedded in the SDP
            // offer; send the same local candidates through its REST trickle
            // route. Keep signed gateway details out of logs and UI.
            std::vector<rtc::Candidate> localIce;
            {
                std::lock_guard<std::mutex> lock(gathered->mutex);
                localIce = gathered->values;
            }
            for (const auto& candidate : localIce) {
                if (cancel) break;
                json::Value trickle = json::Value::object();
                trickle.set("candidate", std::string(candidate));
                trickle.set("sdpMid", candidate.mid());
                trickle.set("sdpMLineIndex", candidate.mid() == "1" ? 1 : 0);
                signalRequest(host, endpoint("/api/addIceCandidate"), "POST", trickle.dump());
            }
            if (progress) progress("WebRTC negociado. Aguardando vídeo e áudio...");
            return true;
        } catch (const std::exception& e) {
            error = std::string("Falha na negociação WebRTC: ") + e.what();
            return false;
        }
    };

    const uint64_t deadline = platform::nowMs() + 60000;
    uint64_t nextPing = platform::nowMs() + 2000;
    uint64_t nextCandidatePoll = platform::nowMs() + 1000;
    uint64_t nextInput = platform::nowMs();
    uint64_t nextControllerHello = 0;
    bool readyAnnounced = false;
    bool offerSubmitted = false;
    bool sawNativeUdp = false;
    bool completed = false;
    std::set<std::string> remoteIceSeen;
    // Once decoded media begins, stay connected until the user leaves.
    // The 60-second deadline applies only to initial start, not playback.
    while (!cancel && control.connected() &&
           (platform::nowMs() < deadline || completed)) {
        std::string message, readError;
        if (control.read_text(message, 120, readError)) {
            const auto parsed = json::parse(message);
            if (parsed && parsed->isObject()) {
                if (field(*parsed, {"type"}) == "stream" &&
                    field(*parsed, {"action"}) == "getstatus")
                    control.send_text(deviceStatusReply());
                const auto event = parseControlEvent(*parsed);
                if (event.type == ControlEvent::Type::ControllerAssigned) {
                    controllerId = event.controllerId;
                    mapper.reset();
                } else if (event.type == ControlEvent::Type::NativeUdpRequested) {
                    sawNativeUdp = true;
                    const auto ip = field(*parsed, {"ip"});
                    const int videoPort = static_cast<int>((*parsed)["videoport"].asInt());
                    const int audioPort = static_cast<int>((*parsed)["audioport"].asInt());
                    if (!udp.running()) {
                        std::string udpError;
                        if (!udp.start(ip, videoPort, audioPort, udpError)) {
                            error = udpError.empty()
                                ? "Falha ao abrir mídia UDP nativa Boosteroid."
                                : udpError;
                            break;
                        }
                        if (progress) progress("Transporte UDP nativo ativo. Aguardando vídeo...");
                    }
                } else if (event.type == ControlEvent::Type::StreamKey) {
                    // AES key stays inside the UDP transport, never Horizon or
                    // logs. The TV gateway expects a receipt before video.
                    udp.setKeyHex(field(*parsed, {"value"}));
                    control.send_text("{\"type\":\"stream\",\"action\":\"key\",\"value\":\"ok\"}");
                } else if (event.type == ControlEvent::Type::StartWebRtc && !offerSubmitted) {
                    offerSubmitted = true;
                    if (!makePeer()) break;
                } else if (event.type == ControlEvent::Type::SessionEnded) {
                    error = "A sessão Boosteroid foi encerrada pelo gateway.";
                    break;
                }
            }
        } else if (!readError.empty()) {
            error = "Conexão de controle Boosteroid interrompida.";
            break;
        }
        const uint64_t now = platform::nowMs();
        if (now >= nextPing) {
            control.send_ping(); // RFC 6455 ping/pong, as the native TV client.
            nextPing = now + 2000;
        }
        if (controllerId == 0 && now >= nextControllerHello) {
            control.send_text("{\"type\":\"controller\",\"action\":\"connected\",\"name\":\"1\"}");
            nextControllerHello = now + 2000;
        }
        if (signals->connected && !readyAnnounced) {
            readyAnnounced = true;
            control.send_text("{\"type\":\"settings\",\"action\":\"ready\"}");
            control.send_text("{\"type\":\"stream\",\"action\":\"page\",\"is_visible\":true}");
            if (progress) progress("Conectado ao servidor. Aguardando vídeo...");
        }
        if (peer && now >= nextCandidatePoll) {
            const auto candidates = signalRequest(host, endpoint("/api/getIceCandidate"));
            if (candidates.ok()) {
                if (const auto parsed = json::parse(candidates.body)) {
                    std::vector<std::string> values;
                    collectCandidates(*parsed, values);
                    for (auto candidate : values) {
                        if (candidate.rfind("a=", 0) == 0) candidate.erase(0, 2);
                        if (candidate.rfind("candidate:", 0) != 0 ||
                            !remoteIceSeen.insert(candidate).second) continue;
                        try { peer->addRemoteCandidate(rtc::Candidate(candidate, "0")); }
                        catch (...) {}
                    }
                }
            }
            nextCandidatePoll = now + 1500;
        }
        if (controllerId && pad && now >= nextInput) {
            input::ControllerState state;
            if (pad(state)) {
                for (const auto& command : mapper.encode(state, controllerId))
                    control.send_text(command.dump());
            }
            nextInput = now + 33;
        }
        if (signals->failed) { error = "Falha na conexão WebRTC do gateway."; break; }
        if (!completed &&
            (pictureReady ? pictureReady() : mediaGuard->frames > 0)) {
            completed = true;
            if (progress) progress("Primeiro quadro Boosteroid exibido.");
        }
    }
    if (!completed && error.empty())
        error = sawNativeUdp ? "O gateway UDP conectou, mas não produziu um quadro válido."
              : mediaGuard->frames > 0 ? "Vídeo H264 recebido, mas nenhum quadro pôde ser exibido."
                               : "Nenhum quadro Boosteroid chegou antes do limite.";

    // Tell the gateway we're leaving the game, independently from Xbox.
    if (control.connected()) {
        if (controllerId > 0) {
            json::Value bye = json::Value::object();
            bye.set("type", "controller");
            bye.set("action", "disconnected");
            bye.set("id", controllerId);
            control.send_text(bye.dump());
        }
        control.send_text("{\"type\":\"settings\",\"action\":\"terminating\"}");
    }
    // Drain native UDP threads before destroying their H264/Opus destination.
    udp.stop();
    // Wait for any in-flight frame delivery before the NativePlayback sink
    // can be destroyed. RTC threads retain only a shared guard, never
    // references to this stack or to the Xbox player.
    {
        std::lock_guard<std::mutex> lock(mediaGuard->mutex);
        mediaGuard->active = false;
    }
    if (video) video->onFrame(nullptr);
    if (audio) audio->onFrame(nullptr);
    if (peer) peer->close();
    control.close();
    stats_.connected = completed && (signals->connected || sawNativeUdp);
    stats_.h264Frames = mediaGuard->frames;
    stats_.opusPackets = mediaGuard->opus;
    stats_.controllerId = controllerId;
    return completed;
}

} // namespace xc::boosteroid
