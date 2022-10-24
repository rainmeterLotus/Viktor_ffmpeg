//
// Created by rainmeterLotus on 2021/1/23.
//

#ifndef VIKTOR_FFMPEG_CTIMELINE_H
#define VIKTOR_FFMPEG_CTIMELINE_H

#include <vector>
#include "CVideoTrack.h"
#include "CAudioTrack.h"
#include "CTimelineWrapper.h"

class CTimeline {

public:
    CTimeline() {
        m_timeline_wrapper = new (std::nothrow)CTimelineWrapper();
    }

    ~CTimeline() {
    }

    CVideoTrack *insertVideoTrack(int index);

    CAudioTrack *insertAudioTrack(int index);

    CVideoTrack *getVideoTrackByIndex(int index);

    CAudioTrack *getAudioTrackByIndex(int index);

    int getVideoTrackCount() {
        if (m_timeline_wrapper){
            return m_timeline_wrapper->getVideoTrackCount();
        }
        return 0;
    }

    int getAudioTrackCount() {
        if (m_timeline_wrapper){
            return m_timeline_wrapper->getAudioTrackCount();
        }
        return 0;
    }

    void init(JNIEnv *env, jobject job){
        if (m_timeline_wrapper) {
            m_timeline_wrapper->init(env,job);
        }
    }


    void startPlay() {
        if (m_timeline_wrapper) {
            m_timeline_wrapper->startPlay();
        }
    }

    void pausePlay() {
        if (m_timeline_wrapper) {
            m_timeline_wrapper->pausePlay();
        }
    }


    void stopPlay() {
        if (m_timeline_wrapper) {
            m_timeline_wrapper->stopPlay();
        }
    }

    void seek(long microSecond) {
        if (m_timeline_wrapper) {
            m_timeline_wrapper->seek(microSecond);
        }
    }

    void release(){
        if (m_timeline_wrapper) {
            m_timeline_wrapper->release();
            delete m_timeline_wrapper;
            m_timeline_wrapper = nullptr;
        }
    }

    long getDurationUs(){
        if (m_timeline_wrapper) {
            return m_timeline_wrapper->m_total_duration_us;
        }

        return 0;
    }

    void setSurface(JNIEnv *env,jobject surface){
        if (m_timeline_wrapper) {
            m_timeline_wrapper->setSurface(env,surface);
        }
    }

private:
    CTimelineWrapper *m_timeline_wrapper = nullptr;
};


#endif //VIKTOR_FFMPEG_CTIMELINE_H
