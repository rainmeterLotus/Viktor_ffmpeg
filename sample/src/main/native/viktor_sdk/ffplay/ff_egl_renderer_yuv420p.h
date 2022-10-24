//
// Created by rainmeterLotus on 2021/8/3.
//

#ifndef VIKTOR_FFMPEG_FF_EGL_RENDERER_YUV420P_H
#define VIKTOR_FFMPEG_FF_EGL_RENDERER_YUV420P_H


#include "ff_egl_renderer.h"

class EGL_Renderer_yuv420p: public EGL_Renderer {
public:
    virtual GLboolean fun_use(int width, int height);
    virtual GLboolean fun_uploadTexture(VideoOverlay *overlay);
    virtual void func_destroy();
};


#endif //VIKTOR_FFMPEG_FF_EGL_RENDERER_YUV420P_H
