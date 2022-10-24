//
// Created by rainmeterLotus on 2021/1/20.
//

#ifndef VIKTOR_FFMPEG_NATIVE_VIKTOR_AUDIO_CLIP_H
#define VIKTOR_FFMPEG_NATIVE_VIKTOR_AUDIO_CLIP_H

#include <jni.h>



extern "C"
JNIEXPORT jlong JNICALL
Java_com_viktor_sdk_clip_ViktorAudioClip_nativeOriginalDuration(JNIEnv *env, jobject thiz, jlong object_address);



#endif //VIKTOR_FFMPEG_NATIVE_VIKTOR_AUDIO_CLIP_H
