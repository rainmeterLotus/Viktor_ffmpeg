//
// Created by rainmeterLotus on 2022/1/4.
//

#ifndef VIKTOR_FFMPEG_VIKTORVIDEODISPLAY_H
#define VIKTOR_FFMPEG_VIKTORVIDEODISPLAY_H
#include "../../ffplay/ff_video_texture.h"
#include "../context/viktor_context.h"
class ViktorVideoDisplay {
public:
    ViktorVideoDisplay(){
        m_videoTexture = new VideoTexture();
    }
    ~ViktorVideoDisplay(){
        if (m_videoTexture){
            delete m_videoTexture;
            m_videoTexture = nullptr;
        }
    }
    void start(ViktorContext *context);
    void video_display(ViktorContext *context);

    void setSurface(JNIEnv *env,jobject surface);

    void release();
private:
    std::thread *m_display_thread;
    ANativeWindow *m_window = nullptr;
    VideoTexture *m_videoTexture = nullptr;

    void video_refresh(void *opaque, double *remaining_time);
    static void func_display_thread(void *arg,void *context);
};


#endif //VIKTOR_FFMPEG_VIKTORVIDEODISPLAY_H
