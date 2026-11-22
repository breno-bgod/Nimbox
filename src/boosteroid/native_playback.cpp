// SPDX-License-Identifier: GPL-3.0-or-later
// Boosteroid-native decoding/output. Reuses PSBox media + display backends,
// without an Xbox GSSV/StreamPlayer dependency.
#include "boosteroid/native_playback.h"

#include "display/display.h"
#include "media/audio_out.h"
#include "media/decoder.h"

#include <algorithm>
#include <utility>

namespace xc::boosteroid {

bool NativePlayback::start() {
    if (thread_.joinable()) return false;
    stop_ = false;
    failed_ = false;
    firstFrame_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.clear();
        error_.clear();
    }
    audioStarted_ = media::audioStart();
    if (!platform::startThread(thread_, [this] { run(); }, 8u << 20)) {
        if (audioStarted_) {
            media::audioStop();
            audioStarted_ = false;
        }
        return false;
    }
    return true;
}

void NativePlayback::stop() {
    stop_ = true;
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
    if (audioStarted_) {
        media::audioStop();
        audioStarted_ = false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.clear();
}

std::string NativePlayback::error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_;
}

EncodedMediaSink NativePlayback::mediaSink() {
    EncodedMediaSink sink;
    sink.h264AnnexB = [this](const uint8_t* data, size_t n, uint32_t timestamp) {
        queue(true, data, n, timestamp);
    };
    sink.opus = [this](const uint8_t* data, size_t n, uint32_t timestamp) {
        queue(false, data, n, timestamp);
    };
    return sink;
}

void NativePlayback::queue(bool video, const uint8_t* bytes, size_t size,
                           uint32_t timestamp) {
    if (stop_ || !bytes || !size || size > (4u << 20)) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) return;
    // Avoid building up seconds of latency under a temporary CPU/GPU stall.
    if (pending_.size() >= 80) {
        auto old = std::find_if(pending_.begin(), pending_.end(),
                                [](const Packet& p) { return p.video; });
        if (old != pending_.end()) pending_.erase(old);
        else pending_.pop_front();
    }
    Packet packet;
    packet.video = video;
    packet.timestamp = timestamp;
    packet.bytes.assign(bytes, bytes + size);
    pending_.push_back(std::move(packet));
    cv_.notify_one();
}

void NativePlayback::run() {
    platform::setThreadAffinity(platform::ThreadRole::VideoDecode,
                                "boosteroid_decode");
    media::VideoDecoder video;
    media::AudioDecoder audio;
    if (!video.init(1, true) || !audio.init()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_ = "Não foi possível iniciar os decodificadores H264/Opus.";
        }
        failed_ = true;
        return;
    }
    while (!stop_) {
        Packet packet;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(20), [this] {
                return stop_.load() || !pending_.empty();
            });
            if (stop_) break;
            if (pending_.empty()) continue;
            packet = std::move(pending_.front());
            pending_.pop_front();
        }
        if (!packet.video) {
            std::vector<float> samples;
            if (audioStarted_ && audio.decode(packet.bytes.data(),
                                               packet.bytes.size(), samples) &&
                !samples.empty()) {
                media::audioPush(samples.data(), samples.size() / 2);
            }
            continue;
        }

        media::Picture picture;
        if (!video.decode(packet.bytes.data(), packet.bytes.size(), picture))
            continue;
        bool drawn = false;
        {
            std::lock_guard<std::recursive_mutex> lock(display::frameMutex());
            if (picture.format == media::Picture::Format::Nv12) {
                drawn = display::drawNv12(picture.y, picture.u, picture.strideY,
                                          picture.strideU, picture.width,
                                          picture.height, false, picture.nv21);
            } else {
                drawn = display::drawYuv420(picture.y, picture.u, picture.v,
                                             picture.strideY, picture.strideU,
                                             picture.strideV, picture.width,
                                             picture.height, false);
            }
            if (drawn) display::present();
        }
        if (drawn) firstFrame_ = true;
    }
}

} // namespace xc::boosteroid
