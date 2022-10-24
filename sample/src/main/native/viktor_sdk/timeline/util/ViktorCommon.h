//
// Created by rainmeterLotus on 2021/12/4.
//

#ifndef VIKTOR_FFMPEG_VIKTORCOMMON_H
#define VIKTOR_FFMPEG_VIKTORCOMMON_H
#include "ViktorLog.h"
#include <sys/stat.h>

extern "C"{
#include <libavformat/avformat.h>
#include <libexif/exif-data.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-tag.h>
};

int is_image_file(AVStream *videoSt);

int get_video_orientation(AVStream *videoSt);

int get_image_orientation(const char *filename, int *mirrorImage);

bool fileExist(const char *filename);

long long get_sys_current_millisecond();
#endif //VIKTOR_FFMPEG_VIKTORCOMMON_H
