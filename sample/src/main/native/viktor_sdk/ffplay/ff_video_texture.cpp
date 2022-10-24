//
// Created by rainmeterLotus on 2021/8/2.
//

#include "ff_video_texture.h"

int VideoTexture::display_overlay(VideoOverlay *overlay,ANativeWindow *native_window,uint32_t overlay_format){
    if (!native_window){
        return -1;
    }

    if (!overlay){
        return -1;
    }

    if (overlay->w <=0 || overlay->h <= 0){
        return -1;
    }

    return egl->EGL_display(overlay, native_window);
}

void VideoTexture::close(){
    if (egl){
        egl->EGL_free();
        delete egl;
        egl = nullptr;
    }
}