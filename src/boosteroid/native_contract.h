// SPDX-License-Identifier: GPL-3.0-or-later
// NimboPlay: portable Boosteroid presentation/media boundary.
// No dependency on Xbox GSSV, Electron, Android Java, or PS5 SDK APIs.
#pragma once

#include "util/json.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace xc::boosteroid {

// Transport selection is a future negotiation decision, not a guarantee that
// the Boosteroid service accepts it on the target platform.
enum class Transport { WebRtcH264Opus, AndroidTvUdpH264Opus };
enum class State {
    Disabled, Authorizing, Authorized, CatalogReady, Launching, Streaming, Failed
};
enum class Failure { None, Authorization, Catalog, Transport, Playback, Unsupported };

struct DeviceAuthorization {
    std::string validationUrl; // Public HTTPS URL for a device-code/QR flow.
    std::string status;
    int pollIntervalMs = 2000;
    bool approved = false;
};

struct Game {
    int64_t id = 0;             // Boosteroid's app id, never an Xbox title id.
    std::string title;
    std::string coverUrl;
    std::string publisher;
};

// Only non-secret fields are retained. The original JSON responses can include
// session handoffs, cookies, account data or private launch information: never
// persist or display the raw response in the common Horizon UI.
bool publicAuthorization(const json::Value& response, DeviceAuthorization& output);
std::vector<Game> publicGames(const json::Value& response);
// Distinguishes a valid empty installed library from an unknown API response.
bool catalogHasGameArray(const json::Value& response);

// This is a provider-neutral encoded media output contract: existing
// VideoDecoder accepts H264 Annex-B; existing AudioDecoder accepts Opus.
// Neither transport nor packet format is conflated with the Xbox GSSV protocol.
struct EncodedMediaSink {
    // Payload valid only during callback, not beyond.
    std::function<void(const uint8_t*, size_t, uint32_t)> h264AnnexB;
    std::function<void(const uint8_t*, size_t, uint32_t)> opus;
};

bool deliverH264(const EncodedMediaSink& sink, const uint8_t* packet,
                 size_t size, uint32_t timestamp);
bool deliverOpus(const EncodedMediaSink& sink, const uint8_t* packet,
                 size_t size, uint32_t timestamp);

// Pure state transition logic. The adapter owning real authentication/network
// drives transitions on its worker thread; no fake login or stream is performed.
class SessionState {
public:
    State state() const { return state_; }
    Failure failure() const { return failure_; }
    bool beginAuthorization();
    bool authorizationApproved();
    bool catalogLoaded();
    bool beginLaunch();
    bool streamAttached();
    bool streamStopped();
    void fail(Failure reason);
    void reset();
private:
    State state_ = State::Disabled;
    Failure failure_ = Failure::None;
};

} // namespace xc::boosteroid
