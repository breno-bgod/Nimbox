// SPDX-License-Identifier: GPL-3.0-or-later
// Separate Boosteroid playback pipeline, using PSBox's native decoders and
// display/audio backends. Xbox StreamPlayer remains unchanged.
#pragma once

#include "boosteroid/native_contract.h"
#include "platform/platform.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace xc::boosteroid {

class NativePlayback {
public:
    NativePlayback() = default;
    NativePlayback(const NativePlayback&) = delete;
    NativePlayback& operator=(const NativePlayback&) = delete;
    ~NativePlayback() { stop(); }
    bool start();
    void stop();
    EncodedMediaSink mediaSink();
    bool firstFramePresented() const { return firstFrame_.load(); }
    bool failed() const { return failed_.load(); }
    std::string error() const;
private:
    struct Packet {
        bool video = false;
        uint32_t timestamp = 0;
        std::vector<uint8_t> bytes;
    };
    void queue(bool video, const uint8_t* bytes, size_t n, uint32_t timestamp);
    void run();
    std::atomic<bool> stop_{false}, firstFrame_{false}, failed_{false};
    platform::Thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Packet> pending_;
    std::string error_;
    bool audioStarted_ = false;
};

} // namespace xc::boosteroid
