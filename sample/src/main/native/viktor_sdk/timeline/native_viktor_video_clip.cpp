//
// Created by rainmeterLotus on 2021/1/20.
//

#include "native_viktor_video_clip.h"
#include "clang/CVideoClip.h"

extern "C" {
JNIEXPORT jint JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeOriginalWidth(JNIEnv *env, jobject thiz, jlong object_address) {
}

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeOriginalHeight(JNIEnv *env, jobject thiz, jlong object_address){

}


JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeOriginalDuration(JNIEnv *env, jobject thiz, jlong object_address){

}

JNIEXPORT jstring JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeFilePath(JNIEnv *env, jobject thiz, jlong object_address) {
    auto *cVideoClip = reinterpret_cast<CVideoClip*>(object_address);
    return env->NewStringUTF(cVideoClip->m_file_path.c_str());
}

JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeStartTime(JNIEnv *env, jobject thiz, jlong object_address) {
    auto *cVideoClip = reinterpret_cast<CVideoClip*>(object_address);
    return cVideoClip->m_start_micro_sec;
}

JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorVideoClip_nativeEndTime(JNIEnv *env, jobject thiz, jlong object_address) {
    auto *cVideoClip = reinterpret_cast<CVideoClip*>(object_address);
    return cVideoClip->m_end_micro_sec;
}


}