//
// Created by rainmeterLotus on 2021/8/2.
//

#ifndef VIKTOR_FFMPEG_FF_VIDEO_TEXTURE_H
#define VIKTOR_FFMPEG_FF_VIDEO_TEXTURE_H

#include "ff_egl.h"

extern "C"{
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
}


class VideoTexture{
public:
    VideoTexture(){
        egl = new VideoEGL();
    }
    int display_overlay(VideoOverlay *overlay,ANativeWindow *native_window,uint32_t overlay_format);

    void close();

private:

    VideoEGL *egl = nullptr;
};


#endif //VIKTOR_FFMPEG_FF_VIDEO_TEXTURE_H
