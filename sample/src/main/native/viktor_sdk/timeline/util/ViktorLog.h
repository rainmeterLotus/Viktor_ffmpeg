//
// Created by rainmeterLotus on 2021/12/4.
//

#ifndef VIKTOR_FFMPEG_VIKTORLOG_H
#define VIKTOR_FFMPEG_VIKTORLOG_H
#include <android/log.h>
#ifndef VIKTOR_LOG_TAG
#define VIKTOR_LOG_TAG "Viktor"
#endif

#define LOG

#ifdef LOG
#define VIKTOR_LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,VIKTOR_LOG_TAG,FORMAT,##__VA_ARGS__)
#define VIKTOR_LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,VIKTOR_LOG_TAG,FORMAT,##__VA_ARGS__)
#define VIKTOR_LOGW(FORMAT,...) __android_log_print(ANDROID_LOG_WARN,VIKTOR_LOG_TAG,FORMAT,##__VA_ARGS__)
#define VIKTOR_LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,VIKTOR_LOG_TAG,FORMAT,##__VA_ARGS__)

#else
#define LOGD(TAG, FORMAT,...);
#define LOGI(TAG, FORMAT,...);
#define LOGW(TAG, FORMAT,...);
#define LOGE(TAG, FORMAT,...);
#endif


#endif //VIKTOR_FFMPEG_VIKTORLOG_H
