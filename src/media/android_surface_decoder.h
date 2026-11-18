// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#if defined(__ANDROID__)
#include <jni.h>
namespace xc::media {

// Independent Android-only decoder, adapted from XStreaming's AGPLv3 Surface path.
// Neither the pre-existing ByteBuffer decoder nor Vulkan/SDL renderer is modified.
class AndroidSurfaceDecoder {
public:
    enum class Submit { Error = -1, Busy = 0, Accepted = 1 };
    struct Counters {
        uint64_t submitted = 0;
        uint64_t released = 0;
        uint64_t rendered = 0; // Android MediaCodec.OnFrameRenderedListener
        uint64_t inputBusy = 0;
        uint64_t errors = 0;
        int outputWidth = 0, outputHeight = 0;
    };
    AndroidSurfaceDecoder() = default;
    ~AndroidSurfaceDecoder();
    AndroidSurfaceDecoder(const AndroidSurfaceDecoder&) = delete;
    AndroidSurfaceDecoder& operator=(const AndroidSurfaceDecoder&) = delete;
    bool init(int width, int height);
    Submit submit(const uint8_t* accessUnit, size_t size, uint32_t rtpTimestamp, bool keyframe);
    Counters stats() const;
    void stop();
    // UI composition: mirror the SAME pre-rendered RGBA HUD/menu over the Surface.
    static bool isActive();
    static void mirrorOverlay(const uint32_t* pixels, int x, int y,
                              int width, int height, uint8_t opacity);
private:
    jclass activityClass_ = nullptr;
    jmethodID queueMethod_ = nullptr;
    jmethodID statsMethod_ = nullptr;
    jmethodID stopMethod_ = nullptr;
};
}
#endif
