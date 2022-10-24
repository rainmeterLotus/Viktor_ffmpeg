//
// Created by rainmeterLotus on 2021/1/20.
//

#include "native_viktor_audio_track.h"
#include "clang/CAudioTrack.h"


extern "C" {
#include "util/jni_help.h"

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_track_ViktorAudioTrack_nativeInsertClip(JNIEnv *env, jobject thiz, jlong object_address, jstring file_path,
                                                            jlong start_micro_sec, jlong end_micro_sec,jint index) {
    auto *cAudioTrack = reinterpret_cast<CAudioTrack*>(object_address);

    std::string filePath = env->GetStringUTFChars(file_path,NULL);
    CAudioClip *cAudioClip = cAudioTrack->insertClipByIndex(filePath, start_micro_sec, end_micro_sec, index);
    if (!cAudioClip){
        return nullptr;
    }
    return toJavaAny(env,"com/viktor/sdk/clip/ViktorAudioClip",reinterpret_cast<jlong>(cAudioClip));
}

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_track_ViktorAudioTrack_nativeClipByIndex(JNIEnv *env, jobject thiz, jlong object_address, jint index) {
    auto *cAudioTrack = reinterpret_cast<CAudioTrack*>(object_address);
    CAudioClip *cAudioClip = cAudioTrack->getClipByIndex(index);
    if (!cAudioClip){
        return nullptr;
    }

    return toJavaAny(env,"com/viktor/sdk/clip/ViktorAudioClip",reinterpret_cast<jlong>(cAudioClip));
}

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_track_ViktorAudioTrack_nativeGetClipCount(JNIEnv *env, jobject thiz, jlong object_address) {
    auto *cAudioTrack = reinterpret_cast<CAudioTrack *>(object_address);
    return cAudioTrack->getAudioClipCount();
}


}