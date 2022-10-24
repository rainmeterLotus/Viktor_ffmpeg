//
// Created by rainmeterLotus on 2021/1/19.
//

#ifndef VIKTOR_FFMPEG_NATIVE_TIMELINE_H
#define VIKTOR_FFMPEG_NATIVE_TIMELINE_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAppendVideoTrack(JNIEnv *env, jobject thiz, jlong timeline_address);


JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAppendAudioTrack(JNIEnv *env, jobject thiz, jlong timeline_address);

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeInsertVideoTrack(JNIEnv *env, jobject thiz, jlong timeline_address, jint index);

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeInsertAudioTrack(JNIEnv *env, jobject thiz, jlong timeline_address, jint index);

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeVideoTrackByIndex(JNIEnv *env, jobject thiz, jlong timeline_address, jint index);

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAudioTrackByIndex(JNIEnv *env, jobject thiz, jlong timeline_address, jint index);

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeVideoTrackCount(JNIEnv *env, jobject thiz, jlong timeline_address);

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeAudioTrackCount(JNIEnv *env, jobject thiz, jlong timeline_address);

JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_timeline_ViktorTimeline_nativeDurationUs(JNIEnv *env, jobject thiz, jlong timeline_address);

#ifdef __cplusplus
}
#endif

#endif //VIKTOR_FFMPEG_NATIVE_TIMELINE_H
