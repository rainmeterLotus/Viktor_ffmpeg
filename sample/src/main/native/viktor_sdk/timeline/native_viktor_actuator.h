//
// Created by rainmeterLotus on 2021/1/19.
//

#ifndef VIKTOR_FFMPEG_NATIVE_VIKTOR_ACTUATOR_H
#define VIKTOR_FFMPEG_NATIVE_VIKTOR_ACTUATOR_H
#include <jni.h>

extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeLinkTimelineWithWindow(JNIEnv *env, jobject thiz, jlong timeline_address);

extern "C"
JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeTimeline(JNIEnv *env, jobject thiz);

extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeReleaseTimeline(JNIEnv *env, jobject thiz, jlong timeline_address);


extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeSeek(JNIEnv *env, jobject thiz, jlong timeline_address, jlong position);

extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativePlay(JNIEnv *env, jobject thiz, jlong timeline_address);

extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeStop(JNIEnv *env, jobject thiz, jlong timeline_address);

extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativeSetSurface(JNIEnv *env, jobject thiz, jlong timeline_address, jobject surface);

extern "C"
JNIEXPORT void JNICALL
Java_com_viktor_sdk_timeline_ViktorActuator_nativePause(JNIEnv *env, jobject thiz, jlong timeline_address);
#endif //VIKTOR_FFMPEG_NATIVE_VIKTOR_ACTUATOR_H