// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#if defined(__ANDROID__)
#include <jni.h>
namespace xc::media {
// Android-only third playback path; CPU/GPU decoders are separate and untouched.
// JNI bridge connects Nimbox's existing H.264 access units to XStreaming-derived
// MediaCodec -> Surface decoder (AGPLv3, see third_party/XStreaming/NOTICE.md).
class AndroidSurfaceDecoder {
public:
    AndroidSurfaceDecoder() = default;
    ~AndroidSurfaceDecoder();
    AndroidSurfaceDecoder(const AndroidSurfaceDecoder&) = delete;
    AndroidSurfaceDecoder& operator=(const AndroidSurfaceDecoder&) = delete;
    bool init(int width, int height);
    bool decode(const uint8_t* accessUnit, size_t size, uint32_t rtpTimestamp, bool keyframe);
    void stop();
private:
    jclass activityClass_ = nullptr; // global ref held only during selected session
    jmethodID feed_ = nullptr;
    jmethodID stop_ = nullptr;
};
}
#endif
