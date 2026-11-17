// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "media/decoder.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace xc::media {

// The libwebrtc Android VIDEO CODEC ONLY. Video bitstream and the rest of
// xCloud continue to originate from Nimbox's existing StreamSession.
class AndroidWebRtcDecoder {
public:
    AndroidWebRtcDecoder() = default;
    ~AndroidWebRtcDecoder();
    AndroidWebRtcDecoder(const AndroidWebRtcDecoder&) = delete;
    AndroidWebRtcDecoder& operator=(const AndroidWebRtcDecoder&) = delete;

    bool init(int width, int height);
    bool decode(const uint8_t* annexB, size_t length, uint32_t tag, Picture& out);
    bool pending() const { return pending_; }
    bool failed() const { return failed_; }
private:
    void* activity_ = nullptr; // JNI GlobalRef
    void* decodeMethod_ = nullptr; // jmethodID
    void* stopMethod_ = nullptr; // jmethodID
    std::vector<uint8_t> packed_;
    bool pending_ = false;
    bool failed_ = false;
};
}
