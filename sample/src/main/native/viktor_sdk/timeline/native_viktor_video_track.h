//
// Created by rainmeterLotus on 2021/1/19.
//

#ifndef VIKTOR_FFMPEG_NATIVE_VIKTOR_VIDEO_TRACK_H
#define VIKTOR_FFMPEG_NATIVE_VIKTOR_VIDEO_TRACK_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_track_ViktorVideoTrack_nativeInsertClip(JNIEnv *env, jobject thiz, jlong object_address, jstring file_path,
                                                            jlong start_micro_sec, jlong end_micro_sec,jint index);


JNIEXPORT jint JNICALL
Java_com_viktor_sdk_track_ViktorVideoTrack_nativeGetClipCount(JNIEnv *env, jobject thiz, jlong object_address);

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_track_ViktorVideoTrack_nativeClipByIndex(JNIEnv *env, jobject thiz, jlong object_address, jint index);

#ifdef __cplusplus
}
#endif

#endif //VIKTOR_FFMPEG_NATIVE_VIKTOR_VIDEO_TRACK_H
