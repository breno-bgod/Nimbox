// SPDX-License-Identifier: GPL-3.0-or-later
#include "media/android_surface_decoder.h"
#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>
#include <atomic>
#include <climits>
#include <chrono>
#include <thread>
#include "util/log.h"

namespace xc::media {
namespace {
std::atomic<bool> gSurfaceActive{false};

bool clearJniError(JNIEnv* env, const char* where) {
    if (!env->ExceptionCheck()) return false;
    env->ExceptionDescribe();
    env->ExceptionClear();
    XC_LOGE("android_surface: JNI exception at %s", where);
    return true;
}
} // namespace

AndroidSurfaceDecoder::~AndroidSurfaceDecoder() { stop(); }

bool AndroidSurfaceDecoder::init(int width, int height) {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!env || !activity) return false;
    jclass local = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    if (!local || clearJniError(env, "getActivityClass")) return false;

    jmethodID start = env->GetStaticMethodID(local, "surfaceVideoStart", "(II)Z");
    queueMethod_ = env->GetStaticMethodID(local, "surfaceVideoQueue", "(Ljava/nio/ByteBuffer;JZ)I");
    statsMethod_ = env->GetStaticMethodID(local, "surfaceVideoStats", "()[J");
    stopMethod_ = env->GetStaticMethodID(local, "surfaceVideoStop", "()V");
    if (clearJniError(env, "lookupMethods") || !start || !queueMethod_ || !statsMethod_ || !stopMethod_) {
        env->DeleteLocalRef(local);
        return false;
    }
    activityClass_ = static_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);
    if (!activityClass_) return false;

    bool ok = env->CallStaticBooleanMethod(activityClass_, start, width, height) == JNI_TRUE;
    if (clearJniError(env, "start") || !ok) {
        stop();
        return false;
    }
    gSurfaceActive.store(true, std::memory_order_release);
    XC_LOGI("android_surface: initialized %dx%d", width, height);
    return true;
}

AndroidSurfaceDecoder::Submit AndroidSurfaceDecoder::submit(
        const uint8_t* data, size_t size, uint32_t rtpTimestamp, bool keyframe) {
    if (!activityClass_ || !queueMethod_ || !data || size == 0 || size > static_cast<size_t>(INT_MAX))
        return Submit::Error;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return Submit::Error;
    // The Java method copies the compressed H.264 AU synchronously.
    jobject packet = env->NewDirectByteBuffer(const_cast<uint8_t*>(data), static_cast<jlong>(size));
    if (!packet || clearJniError(env, "videoPacket")) return Submit::Error;
    const jlong ptsUs = static_cast<jlong>(rtpTimestamp) * 1000000LL / 90000LL;
    jint status = env->CallStaticIntMethod(activityClass_, queueMethod_, packet, ptsUs,
                                           keyframe ? JNI_TRUE : JNI_FALSE);
    env->DeleteLocalRef(packet);
    if (clearJniError(env, "submit")) return Submit::Error;
    if (status == 1) return Submit::Accepted;
    if (status == 0) return Submit::Busy;
    return Submit::Error;
}

AndroidSurfaceDecoder::Counters AndroidSurfaceDecoder::stats() const {
    Counters result{};
    if (!activityClass_ || !statsMethod_) return result;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return result;
    auto arr = static_cast<jlongArray>(env->CallStaticObjectMethod(activityClass_, statsMethod_));
    if (clearJniError(env, "stats") || !arr) return result;
    if (env->GetArrayLength(arr) >= 7) {
        jlong data[7] = {};
        env->GetLongArrayRegion(arr, 0, 7, data);
        result.submitted = static_cast<uint64_t>(data[0]);
        result.released = static_cast<uint64_t>(data[1]);
        result.rendered = static_cast<uint64_t>(data[2]);
        result.inputBusy = static_cast<uint64_t>(data[3]);
        result.errors = static_cast<uint64_t>(data[4]);
        result.outputWidth = static_cast<int>(data[5]);
        result.outputHeight = static_cast<int>(data[6]);
    }
    env->DeleteLocalRef(arr);
    return result;
}

void AndroidSurfaceDecoder::stop() {
    gSurfaceActive.store(false, std::memory_order_release);
    if (!activityClass_) return;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env) {
        if (stopMethod_) env->CallStaticVoidMethod(activityClass_, stopMethod_);
        clearJniError(env, "stop");
        env->DeleteGlobalRef(activityClass_);
    }
    activityClass_ = nullptr;
    queueMethod_ = nullptr;
    statsMethod_ = nullptr;
    stopMethod_ = nullptr;
}

bool AndroidSurfaceDecoder::isActive() {
    return gSurfaceActive.load(std::memory_order_acquire);
}

void AndroidSurfaceDecoder::mirrorOverlay(const uint32_t* pixels, int x, int y,
                                           int width, int height, uint8_t opacity) {
    if (!gSurfaceActive.load(std::memory_order_acquire)) return;
    if (width < 0 || height < 0 || width > 4096 || height > 2160) return;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return;
    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!activity) return;
    jclass clazz = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    if (!clazz || clearJniError(env, "overlayClass")) return;
    jmethodID method = env->GetStaticMethodID(clazz, "surfaceVideoOverlay", "(Ljava/nio/ByteBuffer;IIIII)V");
    if (clearJniError(env, "overlayMethod") || !method) {
        env->DeleteLocalRef(clazz);
        return;
    }
    jobject bitmap = nullptr;
    if (pixels && width && height) {
        bitmap = env->NewDirectByteBuffer(const_cast<uint32_t*>(pixels),
                       static_cast<jlong>(width) * height * sizeof(uint32_t));
        if (clearJniError(env, "overlayBuffer")) {
            env->DeleteLocalRef(clazz);
            return;
        }
    }
    // Java copies Bitmap pixels before we leave this method.
    env->CallStaticVoidMethod(clazz, method, bitmap, x, y, width, height, opacity);
    clearJniError(env, "overlayDraw");
    if (bitmap) env->DeleteLocalRef(bitmap);
    env->DeleteLocalRef(clazz);
}
} // namespace xc::media
#endif
