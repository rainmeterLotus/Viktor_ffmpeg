//
// Created by rainmeterLotus on 2021/8/3.
//

#include "ff_egl.h"
#include "ff_egl_renderer_yuv420p.h"
#include "ff_renderer.h"

EGLBoolean VideoEGL::EGL_display(VideoOverlay *overlay,EGLNativeWindowType window){
    EGLBoolean ret = EGL_FALSE;
    if (!EGL_makeCurrent(window)){
        return EGL_FALSE;
    }
    VIKTOR_LOGW("EGL_display EGL_makeCurrent ok");
    ret = EGL_display_internal(overlay,window);
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglReleaseThread();
    return ret;
}

EGLBoolean VideoEGL::EGL_display_internal(VideoOverlay *overlay,EGLNativeWindowType window){
    if (!EGL_prepareRenderer(overlay)){
        VIKTOR_LOGE("[EGL] IJK_EGL_prepareRenderer failed\n");
        return EGL_FALSE;
    }

    if (!GLES2_Renderer_renderOverlay(m_renderer,overlay)){
        VIKTOR_LOGE("[EGL] IJK_GLES2_render failed\n");
        return EGL_FALSE;
    }

    if (m_rotate_filter && m_color_filter && overlay->frameFormat == AV_PIX_FMT_RGBA && has_filter){
        int width = overlay->w;
        int height = overlay->h;
        m_rotate_filter->onRender(m_renderer->m_out_texture[0],width,height,overlay->orientation,overlay->serial);
//        m_color_filter->onRender(m_rotate_filter->get_texture_id(),overlay->w,overlay->h,overlay->orientation,overlay->serial);
//        m_color_filter->onRender(m_renderer->m_out_texture[0],overlay->w,overlay->h,overlay->orientation,overlay->serial);
    }

    eglSwapBuffers(m_display,m_surface);
    return EGL_TRUE;
}


EGLBoolean VideoEGL::EGL_prepareRenderer(VideoOverlay *overlay){
    int width = overlay->w;
    int height = overlay->h;
    if (!GLES2_Renderer_isValid(m_renderer)){
        VIKTOR_LOGE("EGL_prepareRenderer");
        GLES2_Renderer_reset(m_renderer);
        GLES2_Renderer_freeP(&m_renderer);

        m_renderer = GLES2_Renderer_create(overlay);

        if (!m_renderer){
            VIKTOR_LOGE("[EGL] Could not create render.");
            return EGL_FALSE;
        }

        if (!GLES2_Renderer_use(m_renderer,overlay->w,overlay->h)){
            VIKTOR_LOGE("[EGL] Could not use render.");
            GLES2_Renderer_freeP(&m_renderer);
            return EGL_FALSE;
        }

    }

    VIKTOR_LOGE("EGL_prepareRenderer m_serial:%d,overlay->serial:%d",m_serial,overlay->serial);
    if (GLES2_Renderer_isValid(m_renderer) && m_serial != overlay->serial){
        m_renderer->format = overlay->format;
        m_renderer->orientation = overlay->orientation;
        m_serial = overlay->serial;


        m_renderer->reset_fbo(overlay->w,overlay->h);
//         GLES2_Renderer_matrix(m_renderer,overlay->w,overlay->h);
    }

    if (!EGL_setSurfaceSize(overlay->w, overlay->h)){
        VIKTOR_LOGE("[EGL] IJK_EGL_setSurfaceSize(%d, %d) failed\n", overlay->w, overlay->h);
        return EGL_FALSE;
    }

    if (!is_init_filter && overlay->frameFormat == AV_PIX_FMT_RGBA && has_filter){
        m_rotate_filter->onInit(width,height);
//        m_color_filter->onInit(width,height);
    }

//    glViewport(0,0,m_width,m_height);
    return EGL_TRUE;
}

int VideoEGL::EGL_getSurfaceWidth(){
    EGLint width = 0;
    if (!eglQuerySurface(m_display,m_surface,EGL_WIDTH,&width)){
        VIKTOR_LOGE("[EGL] eglQuerySurface(EGL_WIDTH) returned error %d", eglGetError());
        return 0;
    }
    return width;
}
int VideoEGL::EGL_getSurfaceHeight(){
    EGLint height = 0;
    if (!eglQuerySurface(m_display,m_surface,EGL_HEIGHT,&height)){
        VIKTOR_LOGE("[EGL] eglQuerySurface(EGL_HEIGHT) returned error %d", eglGetError());
        return 0;
    }
    return height;
}

EGLBoolean VideoEGL::EGL_setSurfaceSize(int width,int height){
    if (!EGL_isValid()){
        return EGL_FALSE;
    }

    m_width = EGL_getSurfaceWidth();
    m_height = EGL_getSurfaceHeight();

    if (width != m_width || height != m_height){
        int format = ANativeWindow_getFormat(m_window);
        VIKTOR_LOGE("ANativeWindow_setBuffersGeometry before(w=%d,h=%d) -> overlay(w=%d,h=%d);",
              m_width, m_height,
              width, height);
        int ret = ANativeWindow_setBuffersGeometry(m_window,width,height,format);
        if (ret){
            VIKTOR_LOGE("[EGL] ANativeWindow_setBuffersGeometry() returned error %d", ret);
            return EGL_FALSE;
        }
        m_width = EGL_getSurfaceWidth();
        m_height = EGL_getSurfaceHeight();
        VIKTOR_LOGE("ANativeWindow_setBuffersGeometry after (w=%d,h=%d) -> overlay(w=%d,h=%d);",
             m_width, m_height,
             width, height);
        return (m_width && m_height) ? EGL_TRUE:EGL_FALSE;
    }

    return EGL_TRUE;
}


void VideoEGL::EGL_terminate(){
    if (!EGL_isValid()){
        return;
    }

    if (m_display){
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_context){
            eglDestroyContext(m_display, m_context);
        }

        if (m_surface){
            eglDestroySurface(m_display, m_surface);
        }

        eglTerminate(m_display);
        eglReleaseThread();
    }

    m_context = EGL_NO_CONTEXT;
    m_surface = EGL_NO_SURFACE;
    m_display = EGL_NO_DISPLAY;
}


void VideoEGL::EGL_free(){
    VIKTOR_LOGE("EGL_free m_renderer:%p",m_renderer);
    if (m_renderer){
        VIKTOR_LOGE("EGL_free");
        GLES2_Renderer_reset(m_renderer);
        GLES2_Renderer_freeP(&m_renderer);
    }

    EGL_terminate();
}

EGLBoolean VideoEGL::EGL_isValid(){
    if (m_display && m_surface && m_context){
        return EGL_TRUE;
    }

    return EGL_FALSE;
}



EGLBoolean VideoEGL::EGL_makeCurrent(EGLNativeWindowType window){
    if (window && window == m_window &&
        m_display && m_surface && m_context){

        if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)){
            VIKTOR_LOGW("[EGL] elgMakeCurrent() failed (cached)");
            return EGL_FALSE;
        }

        return EGL_TRUE;
    }
    VIKTOR_LOGW("EGL_makeCurrent EGL_free");
    //window改变之后，需要释放m_renderer
    EGL_free();
    m_window = window;
    if (!window)
        return EGL_FALSE;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY){
        VIKTOR_LOGW("eglGetDisplay fail");
        return EGL_FALSE;
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        VIKTOR_LOGW("[EGL] eglInitialize failed\n");
        return EGL_FALSE;
    }

    VIKTOR_LOGW("[EGL] eglInitialize major:%d,minor:%d\n", (int)major, (int)minor);

    static const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
            EGL_BLUE_SIZE,          8,
            EGL_GREEN_SIZE,         8,
            EGL_RED_SIZE,           8,
            EGL_NONE
    };

    EGLConfig config;
    EGLint numConfig;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfig)) {
        VIKTOR_LOGW("[EGL] eglChooseConfig failed\n");
        eglTerminate(display);
        return EGL_FALSE;
    }

    /***********start***********/
    EGLint native_visual_id = 0;
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &native_visual_id)) {
        VIKTOR_LOGW("[EGL] eglGetConfigAttrib() returned error %d", eglGetError());
        eglTerminate(display);
        return EGL_FALSE;
    }
    int32_t width  = ANativeWindow_getWidth(window);
    int32_t height = ANativeWindow_getHeight(window);
    VIKTOR_LOGW("[EGL] ANativeWindow_setBuffersGeometry(f=%d),width:%d,height:%d;", native_visual_id,width,height);
    int ret = ANativeWindow_setBuffersGeometry(window, width, height, native_visual_id);
    if (ret) {
        VIKTOR_LOGW("[EGL] ANativeWindow_setBuffersGeometry(format) returned error %d", ret);
        eglTerminate(display);
        return EGL_FALSE;
    }
    /***********end***********/


    EGLSurface surface = eglCreateWindowSurface(display, config, window, NULL);
    if (surface == EGL_NO_SURFACE) {
        VIKTOR_LOGW("[EGL] eglCreateWindowSurface failed\n");
        eglTerminate(display);
        return EGL_FALSE;
    }

    static const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };

    EGLSurface context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        VIKTOR_LOGW("[EGL] eglCreateContext failed\n");
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return EGL_FALSE;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        VIKTOR_LOGW("[EGL] elgMakeCurrent() failed (new)\n");
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return EGL_FALSE;
    }
    GLES2_Renderer_setupGLES();
    m_context = context;
    m_surface = surface;
    m_display = display;
    return EGL_TRUE;
}