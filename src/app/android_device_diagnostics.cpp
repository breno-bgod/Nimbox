// SPDX-License-Identifier: GPL-3.0-or-later
// Android-only JNI bridge for the experimental NX device diagnostics.
// No network calls or authentication are performed on Android's UI thread.
#include <jni.h>
#include <string>

#include "app/app_state.h"

extern "C" JNIEXPORT jboolean JNICALL
Java_com_rafaelngp_psbox_MainActivity_nativeRequestDeviceDiagnosis(
    JNIEnv* env, jclass, jstring title, jboolean probeSession) {
    if (!title) return JNI_FALSE;
    const char* chars = env->GetStringUTFChars(title, nullptr);
    if (!chars) return JNI_FALSE;
    std::string query(chars);
    env->ReleaseStringUTFChars(title, chars);
    // This diagnostic intentionally supports only the predefined test cases.
    // No arbitrary requests, IDs, injected headers or credentials from Java.
    if (query != "Killer Instinct 2 Classic" &&
        query != "Grand Theft Auto VI" &&
        query != "Fortnite") return JNI_FALSE;
    return xc::app::requestDeviceDiagnosis(query, probeSession == JNI_TRUE)
        ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_rafaelngp_psbox_MainActivity_nativeDeviceDiagnosisReport(
    JNIEnv* env, jclass) {
    const std::string result = xc::app::readDeviceDiagnosis();
    return env->NewStringUTF(result.c_str());
}
