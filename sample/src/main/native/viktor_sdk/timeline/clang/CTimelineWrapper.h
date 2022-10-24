//
// Created by rainmeterLotus on 2021/12/16.
//

#ifndef VIKTOR_FFMPEG_CTIMELINEWRAPPER_H
#define VIKTOR_FFMPEG_CTIMELINEWRAPPER_H
#include <vector>
#include "CVideoTrack.h"
#include "CAudioTrack.h"
#include "../context/viktor_context.h"
#include "../demux/ViktorDemux.h"
#include "../display/ViktorVideoDisplay.h"

extern "C" {
#include <libavformat/avformat.h>
}

class CTimelineWrapper {
public:
    CTimelineWrapper(){
        m_context = static_cast<ViktorContext *>(av_mallocz(sizeof(ViktorContext)));

        if(m_context){
            m_context->vec_audio_track = new(std::nothrow) std::vector<CAudioTrack *>();
            m_context->vec_video_track = new(std::nothrow) std::vector<CVideoTrack *>();
        }

        m_demux = new (std::nothrow)ViktorDemux();

    }

    ~CTimelineWrapper(){

    }
    CVideoTrack *insertVideoTrack(int index){
        /**
         * 现代大部分编译器都会在new分配内存失败后直接抛出bad_alloc异常
         *  auto *videoTrack = new CVideoTrack();
         *
         *  nothrow可以设置new分配内存失败后不抛出异常，而是返回null
         */
        auto *videoTrack = new (std::nothrow)CVideoTrack();
        if (!videoTrack){
            return nullptr;
        }

        if (!m_context){
            return nullptr;
        }

        std::vector<CVideoTrack *> *video_track = m_context->vec_video_track;
        if (!video_track){
            return nullptr;
        }
        if (index > (int) video_track->size()) {
            index = video_track->size();
        }

        if (index < 0) {
            index = 0;
        }
        video_track->insert(video_track->begin() + index, videoTrack);
        return videoTrack;
    }

    CAudioTrack *insertAudioTrack(int index){
        auto *audioTrack{
            new (std::nothrow)CAudioTrack()
        };
        if (!audioTrack){
            return nullptr;
        }
        if (!m_context){
            return nullptr;
        }
        std::vector<CAudioTrack *> *audio_track = m_context->vec_audio_track;
        if (!audio_track){
            return nullptr;
        }
        if (index > (int) audio_track->size()) {
            index = audio_track->size();
        }

        if (index < 0) {
            index = 0;
        }
        audio_track->insert(audio_track->begin() + index, audioTrack);
        return audioTrack;
    }

    CVideoTrack *getVideoTrackByIndex(int index){
        if (!m_context){
            return nullptr;
        }
        std::vector<CVideoTrack *> *video_track = m_context->vec_video_track;
        if (!video_track){
            return nullptr;
        }
        if (video_track->empty() || index < 0 || index >= video_track->size()) {
            return NULL;
        }
        return video_track->at(index);
    }

    CAudioTrack *getAudioTrackByIndex(int index){
        if (!m_context){
            return nullptr;
        }
        std::vector<CAudioTrack *> *audio_track = m_context->vec_audio_track;
        if (!audio_track){
            return nullptr;
        }
        if (audio_track->empty() || index < 0 || index >= audio_track->size()) {
            return NULL;
        }
        return audio_track->at(index);
    }

    int getVideoTrackCount() {
        if (!m_context){
            return 0;
        }
        std::vector<CVideoTrack *> *video_track = m_context->vec_video_track;
        if (!video_track){
            return 0;
        }
        return video_track->size();
    }

    int getAudioTrackCount() {
        if (!m_context){
            return 0;
        }
        std::vector<CAudioTrack *> *audio_track = m_context->vec_audio_track;
        if (!audio_track){
            return 0;
        }
        return audio_track->size();
    }

    void startPlay();

    void pausePlay();

    void stopPlay();

    void seek(long microSecond);

    void setSurface(JNIEnv *env,jobject surface);

    void init(JNIEnv *env, jobject job);
    void release();

    long m_total_duration_us = 0;
    int m_width = -1;
    int m_height = -1;

protected:
    ViktorContext *m_context = nullptr;
    ViktorDemux *m_demux = nullptr;
    ViktorVideoDisplay *m_video_display = nullptr;

    std::thread *m_start_thread = nullptr;
private:
    void stream_open();
    void stream_close();
    static void start_prepare_thread(void *arg);

    void stateChanged(void *vikt_ctx,int state);
    void fun_prepare_cb(void *vikt_ctx,long durationUs,int width,int height);
    static void fun_progress_cb(void *vikt_ctx,long durationUs);
};


#endif //VIKTOR_FFMPEG_CTIMELINEWRAPPER_H
