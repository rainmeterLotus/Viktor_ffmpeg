//
// Created by rainmeterLotus on 2021/1/24.
//
#ifndef JNI_HELP_H
#define JNI_HELP_H
#include <jni.h>
extern jobject toJavaAny(JNIEnv *env, const char *name, jlong cObjAddress);
#endif