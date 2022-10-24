//
// Created by rainmeterLotus on 2021/1/24.
//
#include <jni.h>
#include "jni_help.h"

jobject toJavaAny(JNIEnv *env, const char *name, jlong cObjAddress) {
    jclass javaClazz = (*env)->FindClass(env, name);
    jmethodID address_m_id = (*env)->GetMethodID(env, javaClazz, "setObjAddress", "(J)V");
    jobject javaObj = (*env)->AllocObject(env, javaClazz);
    (*env)->CallVoidMethod(env, javaObj, address_m_id, cObjAddress);
    (*env)->DeleteLocalRef(env,javaClazz);
    return javaObj;
}
