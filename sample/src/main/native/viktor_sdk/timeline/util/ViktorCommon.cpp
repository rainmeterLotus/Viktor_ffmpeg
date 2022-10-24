//
// Created by rainmeterLotus on 2021/12/4.
//

#include "ViktorCommon.h"


int is_image_file(AVStream *videoSt) {
    if (!videoSt) {
        return 0;
    }
    double fps = av_q2d(videoSt->avg_frame_rate);
    if ((videoSt->avg_frame_rate.num == 0 ||
         videoSt->avg_frame_rate.den == 0) &&
        (isnan(fps) || fps < 1.0)) {
        return 1;
    }

    return 0;
}

int get_video_orientation(AVStream *videoSt) {
    if (!videoSt) {
        return 0;
    }
    AVDictionaryEntry *rotate_dict = NULL;
    rotate_dict = av_dict_get(videoSt->metadata, "rotate", rotate_dict, 0);
    VIKTOR_LOGD("-get_orientation is video rotate_dict:%p", rotate_dict);
    if (!rotate_dict) {
        return 0;
    }

    int angle = atoi(rotate_dict->value);
    VIKTOR_LOGD("-get_orientation is video angle:%d", angle);
    return angle;
}

int get_image_orientation(const char *filename, int *mirrorImage) {
    int theta = 0;
    ExifData *exifData = exif_data_new_from_file(filename);
    if (exifData) {
        ExifByteOrder byteOrder = exif_data_get_byte_order(exifData);
        ExifEntry *exifEntry = exif_data_get_entry(exifData, EXIF_TAG_ORIENTATION);
        if (exifEntry)
            theta = exif_get_short(exifEntry->data, byteOrder);
        exif_data_free(exifData);
    }

    int rote = 0;
    if (theta == 1) {
        rote = 0;
    } else if (theta == 2) {
        rote = 0;
        *mirrorImage = 1;
    } else if (theta == 3) {
        rote = 180;
    } else if (theta == 4) {
        rote = 180;
        *mirrorImage = 1;
    } else if (theta == 5) {
        rote = 90;
        *mirrorImage = 1;
    } else if (theta == 6) {
        rote = 90;
    } else if (theta == 7) {
        rote = 270;
        *mirrorImage = 1;
    } else if (theta == 8) {
        rote = 270;
    }

    VIKTOR_LOGD("get_orientation is image theta:%d,rote:%d,mirrorImage:%d", theta, rote, *mirrorImage);
    return rote;
}


bool fileExist(const char *filename){
    struct stat info{};
    //not exist                     //directory
    if (stat(filename,&info) != 0 || info.st_mode & S_IFDIR){
        return false;
    } else {
        return true;
    }
}

long long get_sys_current_millisecond()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long curTime = ((long)(time.tv_sec))*1000+time.tv_usec/1000;
    return curTime;
}