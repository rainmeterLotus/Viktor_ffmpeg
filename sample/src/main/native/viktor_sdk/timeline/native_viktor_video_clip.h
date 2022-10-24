//
// Created by rainmeterLotus on 2021/1/20.
//

#ifndef VIKTOR_FFMPEG_NATIVE_VIKTOR_VIDEO_CLIP_H
#define VIKTOR_FFMPEG_NATIVE_VIKTOR_VIDEO_CLIP_H
#include <jni.h>

extern "C"
JNIEXPORT jint JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeOriginalWidth(JNIEnv *env, jobject thiz, jlong object_address);


extern "C"
JNIEXPORT jint JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeOriginalHeight(JNIEnv *env, jobject thiz, jlong object_address);


extern "C"
JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeOriginalDuration(JNIEnv *env, jobject thiz, jlong object_address);

extern "C"
JNIEXPORT jstring JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeFilePath(JNIEnv *env, jobject thiz, jlong object_address);

extern "C"
JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeStartTime(JNIEnv *env, jobject thiz, jlong object_address);

extern "C"
JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeEndTime(JNIEnv *env, jobject thiz, jlong object_address);

#endif //VIKTOR_FFMPEG_NATIVE_VIKTOR_VIDEO_CLIP_H
