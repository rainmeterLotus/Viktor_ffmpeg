//
// Created by rainmeterLotus on 2021/8/3.
//

#ifndef VIKTOR_FFMPEG_FF_EGL_H
#define VIKTOR_FFMPEG_FF_EGL_H

#include <EGL/egl.h>
#include <android/native_window_jni.h>
#include "../timeline//util/ViktorLog.h"
#include "ff_egl_renderer.h"
#include "../../all/filter/viktor_filter_color.h"
#include "../../all/filter/viktor_rotate_filter.h"

class VideoEGL {
public:
    VideoEGL(){
        m_rotate_filter = new Viktor_Rotate_Filter();
        m_color_filter = new Viktor_Filter_Color();
    }

    ~VideoEGL(){
        delete m_rotate_filter;
        m_rotate_filter = nullptr;

        delete m_color_filter;
        m_color_filter = nullptr;
    }

    EGLBoolean EGL_display(VideoOverlay *overlay,EGLNativeWindowType window);
    EGLBoolean  EGL_makeCurrent(EGLNativeWindowType window);
    EGLBoolean EGL_isValid();
    EGLBoolean EGL_display_internal(VideoOverlay *overlay,EGLNativeWindowType window);
    EGLBoolean EGL_prepareRenderer(VideoOverlay *overlay);

    EGLBoolean EGL_setSurfaceSize(int width,int height);

    void EGL_terminate();


    void EGL_free();
private:
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLConfig m_context;

    EGLNativeWindowType m_window;

    EGLint m_width;
    EGLint m_height;

    //只要有构造函数，这里就要默认赋值null
    EGL_Renderer *m_renderer = nullptr;

    int EGL_getSurfaceWidth();
    int EGL_getSurfaceHeight();

    bool is_init_filter = false;
    Viktor_Base_Filter *m_rotate_filter = nullptr;
    Viktor_Base_Filter *m_color_filter = nullptr;
    /**
     * 用于区分是否同一视频源
     */
    int m_serial = -1;
};


#endif //VIKTOR_FFMPEG_FF_EGL_H
