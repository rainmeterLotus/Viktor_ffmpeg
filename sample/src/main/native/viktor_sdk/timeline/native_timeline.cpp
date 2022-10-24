//
// Created by rainmeterLotus on 2021/1/19.
//

#include "native_timeline.h"
#include "clang/CTimeline.h"
#include "clang/CAudioTrack.h"

extern "C" {
#include "util/jni_help.h"

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAppendVideoTrack(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return nullptr;
    }
    CVideoTrack *videoTrack = cTimeline->insertVideoTrack(cTimeline->getVideoTrackCount());
    if (videoTrack){
        return toJavaAny(env,"com/viktor/sdk/track/ViktorVideoTrack",reinterpret_cast<jlong>(videoTrack));
    }
    return nullptr;
}


JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAppendAudioTrack(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return nullptr;
    }
    CAudioTrack *audioTrack = cTimeline->insertAudioTrack(cTimeline->getAudioTrackCount());
    if (audioTrack){
        return toJavaAny(env,"com/viktor/sdk/track/ViktorAudioTrack",reinterpret_cast<jlong>(audioTrack));
    }
    return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeInsertVideoTrack(JNIEnv *env, jobject thiz, jlong timeline_address, jint index){
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return nullptr;
    }
    CVideoTrack *videoTrack = cTimeline->insertVideoTrack(index);
    if (videoTrack){
        return toJavaAny(env,"com/viktor/sdk/track/ViktorVideoTrack",reinterpret_cast<jlong>(videoTrack));
    }
    return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeInsertAudioTrack(JNIEnv *env, jobject thiz, jlong timeline_address, jint index) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return nullptr;
    }
    CAudioTrack *audioTrack = cTimeline->insertAudioTrack(index);
    if (audioTrack){
        return toJavaAny(env,"com/viktor/sdk/track/ViktorAudioTrack",reinterpret_cast<jlong>(audioTrack));
    }
    return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeVideoTrackByIndex(JNIEnv *env, jobject thiz, jlong timeline_address, jint index) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return nullptr;
    }
    CVideoTrack *videoTrack = cTimeline->getVideoTrackByIndex(index);
    if (videoTrack){
        return toJavaAny(env,"com/viktor/sdk/track/ViktorVideoTrack",reinterpret_cast<jlong>(videoTrack));
    }
    return nullptr;
}

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAudioTrackByIndex(JNIEnv *env, jobject thiz, jlong timeline_address, jint index) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return nullptr;
    }
    CAudioTrack *audioTrack = cTimeline->getAudioTrackByIndex(index);
    if (audioTrack){
        return toJavaAny(env,"com/viktor/sdk/track/ViktorAudioTrack",reinterpret_cast<jlong>(audioTrack));
    }
    return nullptr;
}

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeVideoTrackCount(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto *cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return 0;
    }
    return cTimeline->getVideoTrackCount();
}

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAudioTrackCount(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return 0;
    }
    return cTimeline->getAudioTrackCount();
}

JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeDurationUs(JNIEnv *env, jobject thiz, jlong timeline_address) {
    auto cTimeline = reinterpret_cast<CTimeline *>(timeline_address);
    if (!cTimeline){
        return 0;
    }
    return cTimeline->getDurationUs();
}

}