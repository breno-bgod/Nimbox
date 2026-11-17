// SPDX-License-Identifier: GPL-3.0-or-later
// Android libwebrtc H264 video-only adapter. No PeerConnection or new session.
#include "media/android_webrtc_decoder.h"
#include "util/log.h"

#include <jni.h>
#include <dlfcn.h>
#include <climits>
#include <cstring>

namespace xc::media {
namespace {
JNIEnv* getEnv() {
    using Fn = JNIEnv* (*)();
    static auto* fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "SDL_GetAndroidJNIEnv"));
    return fn ? fn() : nullptr;
}
jobject getActivity() {
    using Fn = jobject (*)();
    static auto* fn = reinterpret_cast<Fn>(dlsym(RTLD_DEFAULT, "SDL_GetAndroidActivity"));
    return fn ? fn() : nullptr;
}
bool hasException(JNIEnv* env, const char* stage) {
    if (!env->ExceptionCheck()) return false;
    XC_LOGW("android webrtc video: Java exception during %s", stage);
    env->ExceptionClear();
    return true;
}
uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}
}
AndroidWebRtcDecoder::~AndroidWebRtcDecoder() {
    if (!activity_) return;
    if (JNIEnv* env = getEnv()) {
        if (stopMethod_) {
            env->CallVoidMethod(static_cast<jobject>(activity_),
                                reinterpret_cast<jmethodID>(stopMethod_));
            hasException(env, "stop");
        }
        env->DeleteGlobalRef(static_cast<jobject>(activity_));
    }
    activity_ = nullptr;
}
bool AndroidWebRtcDecoder::init(int width, int height) {
    JNIEnv* env = getEnv();
    if (!env || activity_) return false;
    jobject local = getActivity();
    if (!local || hasException(env, "activity")) return false;
    jclass cls = env->GetObjectClass(local);
    if (!cls || hasException(env, "class")) {
        env->DeleteLocalRef(local);
        return false;
    }
    jmethodID start = env->GetMethodID(cls, "startWebRtcVideo", "(II)Z");
    jmethodID decode = env->GetMethodID(cls, "decodeWebRtcVideo", "([BJZ)[B");
    jmethodID stop = env->GetMethodID(cls, "stopWebRtcVideo", "()V");
    bool methodsOk = start && decode && stop && !hasException(env, "methods");
    env->DeleteLocalRef(cls);
    if (!methodsOk) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(local);
        return false;
    }
    activity_ = env->NewGlobalRef(local);
    env->DeleteLocalRef(local);
    if (!activity_) return false;
    decodeMethod_ = reinterpret_cast<void*>(decode);
    stopMethod_ = reinterpret_cast<void*>(stop);
    jboolean started = env->CallBooleanMethod(static_cast<jobject>(activity_),
                                              start, width, height);
    if (hasException(env, "start") || !started) {
        XC_LOGW("android webrtc video: decoder unavailable, using existing MediaCodec path");
        return false;
    }
    XC_LOGI("android webrtc video: connected to Nimbox H264 video pipeline (%dx%d)", width, height);
    return true;
}

bool AndroidWebRtcDecoder::decode(const uint8_t* annexB, size_t length, uint32_t tag, Picture& out) {
    pending_ = false;
    failed_ = false;
    if (!activity_ || !decodeMethod_ || !annexB || !length || length > INT_MAX) {
        failed_ = true;
        return false;
    }
    JNIEnv* env = getEnv();
    if (!env) { failed_ = true; return false; }
    jbyteArray packet = env->NewByteArray(static_cast<jsize>(length));
    if (!packet || hasException(env, "allocate packet")) {
        failed_ = true;
        return false;
    }
    env->SetByteArrayRegion(packet, 0, static_cast<jsize>(length),
                            reinterpret_cast<const jbyte*>(annexB));
    if (hasException(env, "copy packet")) {
        env->DeleteLocalRef(packet);
        failed_ = true;
        return false;
    }
    // Mark as key only when an IDR slice (NAL 5) is present.
    bool idr = false;
    for (size_t i = 0; i + 4 < length; ++i) {
        if (annexB[i] == 0 && annexB[i+1] == 0) {
            size_t p = (annexB[i+2] == 1) ? i+3 :
                       (annexB[i+2] == 0 && annexB[i+3] == 1 ? i+4 : length);
            if (p < length && (annexB[p] & 0x1f) == 5) { idr = true; break; }
        }
    }
    jobject result = env->CallObjectMethod(static_cast<jobject>(activity_),
                   reinterpret_cast<jmethodID>(decodeMethod_), packet,
                   static_cast<jlong>(tag), idr ? JNI_TRUE : JNI_FALSE);
    env->DeleteLocalRef(packet);
    if (hasException(env, "decode")) {
        failed_ = true;
        return false;
    }
    if (!result) { pending_ = true; return false; }
    auto* arr = static_cast<jbyteArray>(result);
    jsize size = env->GetArrayLength(arr);
    if (size <= 16) {
        env->DeleteLocalRef(arr);
        failed_ = true;
        return false;
    }
    packed_.resize(static_cast<size_t>(size));
    env->GetByteArrayRegion(arr, 0, size, reinterpret_cast<jbyte*>(packed_.data()));
    env->DeleteLocalRef(arr);
    if (hasException(env, "copy decoded frame")) {
        failed_ = true;
        return false;
    }
    uint32_t width = le32(packed_.data()+4), height = le32(packed_.data()+8);
    if (width < 16 || height < 16 || width > 4096 || height > 4096) {
        failed_ = true;
        return false;
    }
    size_t cw = (width+1)/2, ch = (height+1)/2;
    size_t ySize = static_cast<size_t>(width)*height, uvSize = cw*ch;
    if (16+ySize+2*uvSize != packed_.size()) {
        failed_ = true;
        return false;
    }
    out.format = Picture::Format::Yuv420p;
    out.y = packed_.data()+16;
    out.u = out.y+ySize;
    out.v = out.u+uvSize;
    out.strideY = static_cast<int>(width);
    out.strideU = static_cast<int>(cw);
    out.strideV = static_cast<int>(cw);
    out.width = static_cast<int>(width);
    out.height = static_cast<int>(height);
    out.visibleHeight = static_cast<int>(height);
    out.tag = le32(packed_.data());
    out.uv = nullptr;
    out.uvPitchBytes = 0;
    out.nv21 = false;
    return true;
}
} // namespace xc::media
