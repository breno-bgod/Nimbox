// SPDX-License-Identifier: GPL-3.0-or-later
#include "media/android_surface_decoder.h"
#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>
#include "util/log.h"
namespace xc::media {
namespace {
bool clearJniError(JNIEnv* env, const char* step) {
    if (!env->ExceptionCheck()) return false;
    env->ExceptionDescribe();
    env->ExceptionClear();
    XC_LOGE("android_surface: JNI failed at %s", step);
    return true;
}
}
AndroidSurfaceDecoder::~AndroidSurfaceDecoder() { stop(); }

bool AndroidSurfaceDecoder::init(int width, int height) {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!env || !activity) return false;
    jclass local = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    if (!local || clearJniError(env, "activity")) return false;
    jmethodID start = env->GetStaticMethodID(local, "surfaceVideoStart", "(II)Z");
    feed_ = env->GetStaticMethodID(local, "surfaceVideoQueue", "(Ljava/nio/ByteBuffer;JZ)Z");
    stop_ = env->GetStaticMethodID(local, "surfaceVideoStop", "()V");
    if (!start || !feed_ || !stop_ || clearJniError(env, "methods")) {
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
    XC_LOGI("android_surface: third decoder active, %dx%d", width, height);
    return true;
}

bool AndroidSurfaceDecoder::decode(const uint8_t* data, size_t size, uint32_t rtpTimestamp, bool keyframe) {
    if (!activityClass_ || !feed_ || !data || size == 0) return false;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return false;
    jobject packet = env->NewDirectByteBuffer(const_cast<uint8_t*>(data), static_cast<jlong>(size));
    if (!packet || clearJniError(env, "packet")) return false;
    const jlong ptsUs = static_cast<jlong>(rtpTimestamp) * 1000000LL / 90000LL;
    bool ok = env->CallStaticBooleanMethod(activityClass_, feed_, packet, ptsUs,
                                           keyframe ? JNI_TRUE : JNI_FALSE) == JNI_TRUE;
    env->DeleteLocalRef(packet);
    if (clearJniError(env, "queue")) return false;
    return ok;
}

void AndroidSurfaceDecoder::stop() {
    if (!activityClass_) return;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env) {
        if (stop_) env->CallStaticVoidMethod(activityClass_, stop_);
        clearJniError(env, "stop");
        env->DeleteGlobalRef(activityClass_);
    }
    activityClass_ = nullptr;
    feed_ = nullptr;
    stop_ = nullptr;
}
}
#endif
