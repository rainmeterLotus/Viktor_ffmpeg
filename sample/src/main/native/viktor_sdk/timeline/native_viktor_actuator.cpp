//
// Created by rainmeterLotus on 2021/1/19.
//

#include "native_viktor_actuator.h"
#include "clang/CTimeline.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>


extern "C" {
#include "util/jni_help.h"

JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeLinkTimelineWithWindow(JNIEnv *env, jobject job, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->init(env,job);
    }
}

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeTimeline(JNIEnv *env, jobject thiz) {
    auto *timeline{new (std::nothrow)CTimeline()};
    if (timeline){
        return toJavaAny(env,"com/viktor/sdk/timeline/ViktorTimeline",reinterpret_cast<jlong>(timeline));
    }
    return nullptr;
}

JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeReleaseTimeline(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->release();
        delete cTimeline;
    }
}

JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeSeek(JNIEnv *env, jobject thiz, jlong timeline_address, jlong microSecond) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->seek(microSecond);
    }
}

JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativePlay(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->startPlay();
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativePause(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->pausePlay();
    }
}


JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeStop(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->stopPlay();
    }
}

JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeSetSurface(JNIEnv *env, jobject thiz, jlong timeline_address, jobject surface){
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (cTimeline){
        cTimeline->setSurface(env,surface);
    }
}

}