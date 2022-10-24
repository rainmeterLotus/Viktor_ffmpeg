//
// Created by rainmeterLotus on 2021/8/11.
//

#ifndef VIKTOR_FFMPEG_FF_EGL_RENDERER_RGB_H
#define VIKTOR_FFMPEG_FF_EGL_RENDERER_RGB_H


#include "ff_egl_renderer.h"

class EGL_Renderer_RGB: public EGL_Renderer {
public:
    virtual GLboolean fun_use(int width, int height);
    virtual GLboolean fun_uploadTexture(VideoOverlay *overlay);

    virtual void func_destroy();

private:
    GLboolean fun_uploadTextureRGB(VideoOverlay *overlay);
    GLboolean sws_scale_frame(AVFrame *avFrame);

    int calculateInSampleSize(int srcWidth,int srcHeight,int reqWidth,int reqHeight);


    AVFrame *m_RGBAFrame = nullptr;
    SwsContext *m_SwsContext = nullptr;
    int flag = 0;
};


#endif //VIKTOR_FFMPEG_FF_EGL_RENDERER_RGB_H
