//
// Created by rainmeterLotus on 2021/1/19.
//

#include "native_viktor_video_track.h"
#include "clang/CVideoTrack.h"


extern "C"{
#include "util/jni_help.h"

JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_track_ViktorVideoTrack_nativeInsertClip(JNIEnv *env, jobject thiz, jlong object_address, jstring file_path,
                                                            jlong start_micro_sec, jlong end_micro_sec,jint index) {
    auto *cVideoTrack = reinterpret_cast<CVideoTrack*>(object_address);

    std::string filePath = env->GetStringUTFChars(file_path,NULL);
    CVideoClip *cVideoClip = cVideoTrack->insertClipByIndex(filePath,start_micro_sec,end_micro_sec,index);
    if (!cVideoClip){
        return nullptr;
    }
    return toJavaAny(env,"com/viktor/sdk/clip/ViktorVideoClip",reinterpret_cast<jlong>(cVideoClip));
}


JNIEXPORT jobject JNICALL
Java_com_viktor_sdk_track_ViktorVideoTrack_nativeClipByIndex(JNIEnv *env, jobject thiz, jlong object_address, jint index) {
    auto *cVideoTrack = reinterpret_cast<CVideoTrack*>(object_address);
    CVideoClip *cVideoClip = cVideoTrack->getClipByIndex(index);
    if (!cVideoClip){
        return nullptr;
    }

    return toJavaAny(env,"com/viktor/sdk/clip/ViktorVideoClip",reinterpret_cast<jlong>(cVideoClip));
}

JNIEXPORT jint JNICALL
Java_com_viktor_sdk_track_ViktorVideoTrack_nativeGetClipCount(JNIEnv *env, jobject thiz, jlong object_address) {
    auto *cVideoTrack = reinterpret_cast<CVideoTrack*>(object_address);

    return cVideoTrack->getVideoClipCount();
}

}
