// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone Boosteroid gateway, unrelated to the Xbox GSSV/StreamSession.
#pragma once

#include "boosteroid/native_client.h"
#include "boosteroid/native_contract.h"
#include "input/controller.h"
#include "util/json.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace xc::boosteroid {

// Safe, testable subset of the gateway's Android TV control protocol.
// No private gateway URLs, signed queries, account identifiers or tokens
// can be returned via ControlEvent.
struct ControlEvent {
    enum class Type { None, StartWebRtc, NativeUdpRequested, ControllerAssigned,
                      StreamKey, SessionEnded, ProtocolError };
    Type type = Type::None;
    int controllerId = 0;
};
ControlEvent parseControlEvent(const json::Value& msg);
std::string deviceStatusReply();

// Blocking native client. Operate only from a dedicated service thread.
// H264 frames are Annex-B AUs, Opus packets are 48kHz. The caller owns the
// decoder/display pipeline and is responsible for opening its audio output.
class NativeGateway {
public:
    struct Stats {
        bool connected = false;
        uint64_t h264Frames = 0;
        uint64_t opusPackets = 0;
        int controllerId = 0;
    };
    using PadSnapshot = std::function<bool(input::ControllerState&)>;
    using Progress = std::function<void(const std::string&)>;
    bool run(const NativeStreamSession& session, const EncodedMediaSink& sink,
             const std::atomic<bool>& cancel, PadSnapshot pad, Progress progress,
             std::string& error);
    const Stats& stats() const { return stats_; }
private:
    Stats stats_;
};

} // namespace xc::boosteroid
