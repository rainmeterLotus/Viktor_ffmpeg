//
// Created by rainmeterLotus on 2021/12/16.
//

#include "CTimelineWrapper.h"


void CTimelineWrapper::init(JNIEnv *env, jobject job) {
    long startT = get_sys_current_millisecond();
    avformat_network_init();

    long videoDurationUs = 0;
    if (!m_context){
        return;
    }
    std::vector<CVideoTrack *> *video_track = m_context->vec_video_track;
    if (video_track){
        for (auto track:*video_track) {
            std::vector<CVideoClip *> *videoClips = track->getVideoClips();
            for (auto clip: *videoClips){
                //注意这里已经openFile
                if (clip->openFile() > 0){
                    videoDurationUs += (clip->m_end_micro_sec - clip->m_start_micro_sec);

                    if (m_width <= -1 & m_height <= -1){
                        m_context->fixWidth = m_width = clip->m_width;
                        m_context->fixHeight = m_height = clip->m_height;
                    }
                }
            }
        }
    }



    long audioDurationUs = 0;
    std::vector<CAudioTrack *> *audio_track = m_context->vec_audio_track;
    if (audio_track){
        for (auto track:*audio_track) {
            std::vector<CAudioClip *> *audioClips = track->getAudioClips();
            for (auto clip: *audioClips){
                //注意这里已经openFile
                if (clip->openFile() > 0){
                    audioDurationUs += (clip->m_end_micro_sec - clip->m_start_micro_sec);
                }
            }
        }
    }


    if (videoDurationUs > 0){
        m_total_duration_us = videoDurationUs;
    } else {
        m_total_duration_us = audioDurationUs;
    }

    VIKTOR_LOGD("m_total_duration_us:%ld",m_total_duration_us);
    VIKTOR_LOGD("spent time:%lld",get_sys_current_millisecond() - startT);

    m_video_display = new ViktorVideoDisplay();

    m_context->m_audioEs = new SLAudio_ES();
    m_context->m_audioEs->createOpenSLES();

    m_context->m_javaObj = env->NewGlobalRef(job);
    env->GetJavaVM(&m_context->m_javaVM);
    m_context->m_progressCallback = fun_progress_cb;

    fun_prepare_cb(m_context,m_total_duration_us,m_width,m_height);

}

void CTimelineWrapper::release(){
    if (m_context){
        std::vector<CAudioTrack *> *audio_track = m_context->vec_audio_track;
        if (audio_track){
            for (auto track:*audio_track) {
                delete track;
            }

            audio_track->clear();
        }

        std::vector<CVideoTrack *> *video_track = m_context->vec_video_track;
        if (video_track){
            for (auto track:*video_track) {
                delete track;
            }

            video_track->clear();
        }

        av_free(m_context);
    }
}

void CTimelineWrapper::startPlay(){
    if (!m_context) return;

    if (m_start_thread){
        if (m_context->paused){
            if (m_context->m_audioEs) {
                m_context->m_audioEs->pause_audio(!m_context->paused);
            }

            stream_toggle_pause(m_context);

            stateChanged(m_context,CODE_START);
        }
    } else {
        stream_open();
        if (m_video_display){
            m_video_display->start(m_context);
        }

        stateChanged(m_context,CODE_START);
    }

}

void CTimelineWrapper::stream_open(){
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *) &flush_pkt;

    if (frame_queue_init(&m_context->picture_frame_q,&m_context->video_packet_q,VIDEO_PICTURE_QUEUE_SIZE,1) < 0){
        goto fail;
    }

    if (frame_queue_init(&m_context->sample_frame_q,&m_context->audio_packet_q,SAMPLE_QUEUE_SIZE,1) < 0){
        goto fail;
    }

    if (packet_queue_init(&m_context->video_packet_q) < 0 || packet_queue_init(&m_context->audio_packet_q) < 0){
        goto fail;
    }

    if (!(m_context->read_frame_cond = sdl_create_cond())){
        goto fail;
    }

    init_clock(&m_context->vidclk, &m_context->video_packet_q.serial);
    init_clock(&m_context->audclk, &m_context->audio_packet_q.serial);
    init_clock(&m_context->extclk, &m_context->extclk.serial);

    m_context->av_sync_type = av_sync_type;
    m_context->show_mode = SHOW_MODE_VIDEO;

    m_start_thread = new (std::nothrow)std::thread(start_prepare_thread,this);

    if (!m_start_thread){
        VIKTOR_LOGD("m_start_thread fail");
fail:
        stream_close();
    }
}

void CTimelineWrapper::stream_close(){
    if (!m_context){
        return;
    }
    VIKTOR_LOGD("stream_close");
    m_context->abort_request = 1;

    sdl_wait_thread(m_start_thread);
    delete m_start_thread;
    m_start_thread = nullptr;

    if (m_demux){
        m_demux->stream_component_close(m_context);
    }

    if (m_video_display){
        m_video_display->release();
    }

    if (m_context->m_audioEs){
        m_context->m_audioEs->close_audio();
    }

    packet_queue_destroy(&m_context->audio_packet_q);
    packet_queue_destroy(&m_context->video_packet_q);

    frame_queue_destroy(&m_context->sample_frame_q);
    frame_queue_destroy(&m_context->picture_frame_q);

    delete m_context->read_frame_cond;
    m_context->read_frame_cond = nullptr;

    release_track(m_context);

    JNIEnv *env;
    int status;
    bool isAttach = false;
    status = m_context->m_javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        if (m_context->m_javaVM->AttachCurrentThread(&env, NULL) == JNI_OK) {
            isAttach = true;
        }
    }

    env->DeleteGlobalRef(m_context->m_javaObj);
    if (isAttach) {
        m_context->m_javaVM->DetachCurrentThread();
    }

    m_context->m_javaObj = nullptr;
    m_context->m_javaVM = nullptr;

    av_free(m_context);
}

void CTimelineWrapper::start_prepare_thread(void *arg){
    auto *cTimelineWrapper = static_cast<CTimelineWrapper *>(arg);

    if (cTimelineWrapper->m_demux && cTimelineWrapper->m_context){
        cTimelineWrapper->m_demux->read_frame_thread(cTimelineWrapper->m_context);
    }
}

void CTimelineWrapper::pausePlay(){
    if (!m_context) return;
    if (!m_context->paused){
        if (m_context->m_audioEs) {
            m_context->m_audioEs->pause_audio(!m_context->paused);
        }
        stream_toggle_pause(m_context);

        stateChanged(m_context,CODE_PAUSE);
    }
}

void CTimelineWrapper::stopPlay(){
    VIKTOR_LOGD("do exit");
    stream_close();
    avformat_network_deinit();
}

void CTimelineWrapper::seek(long microSecond){
    if (!m_context) return;
    if (!m_demux) return;

    m_demux->stream_seek(m_context,microSecond,0,0);
}

void CTimelineWrapper::setSurface(JNIEnv *env,jobject surface){
    if (m_video_display){
        m_video_display->setSurface(env,surface);
    }
}

void CTimelineWrapper::stateChanged(void *vikt_ctx,int state){
    auto *context = static_cast<ViktorContext *>(vikt_ctx);
    if (!context) {
        return;
    }

    if (context->m_javaObj == nullptr){
        return;
    }

    if (context->m_javaVM == nullptr){
        return;
    }

    JNIEnv *env;
    int status;
    bool isAttach = false;
    status = context->m_javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        if (context->m_javaVM->AttachCurrentThread(&env, NULL) == JNI_OK) {
            isAttach = true;
        }
    }

    jclass clazz = env->GetObjectClass(context->m_javaObj);
    jmethodID jmethodId = env->GetMethodID(clazz, "stateChanged", "(I)V");
    env->CallVoidMethod(context->m_javaObj, jmethodId, state);
    if (isAttach) {
        context->m_javaVM->DetachCurrentThread();
    }
}

void CTimelineWrapper::fun_progress_cb(void *vikt_ctx,long durationUs){
    auto *context = static_cast<ViktorContext *>(vikt_ctx);
    if (!context) {
        return;
    }

    if (context->m_javaObj == nullptr){
        return;
    }

    if (context->m_javaVM == nullptr){
        return;
    }


    JNIEnv *env;
    int status;
    bool isAttach = false;
    status = context->m_javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        if (context->m_javaVM->AttachCurrentThread(&env, NULL) == JNI_OK) {
            isAttach = true;
        }
    }

    jclass clazz = env->GetObjectClass(context->m_javaObj);
    jmethodID jmethodId = env->GetMethodID(clazz, "progress", "(J)V");
    env->CallVoidMethod(context->m_javaObj, jmethodId, durationUs);
    if (isAttach) {
        context->m_javaVM->DetachCurrentThread();
    }
}


void CTimelineWrapper::fun_prepare_cb(void *vikt_ctx,long durationUs,int width,int height){
    auto *context = static_cast<ViktorContext *>(vikt_ctx);

    if (!context) {
        return;
    }

    if (context->m_javaObj == nullptr){
        return;
    }

    if (context->m_javaVM == nullptr){
        return;
    }

    JNIEnv *env;
    int status;
    bool isAttach = false;
    status = context->m_javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0) {
        if (context->m_javaVM->AttachCurrentThread(&env, NULL) == JNI_OK) {
            isAttach = true;
        }
    }

    jclass clazz = env->GetObjectClass(context->m_javaObj);
    jmethodID jmethodId = env->GetMethodID(clazz, "prepare", "(JII)V");
    env->CallVoidMethod(context->m_javaObj, jmethodId, durationUs,width, height);
    if (isAttach) {
        context->m_javaVM->DetachCurrentThread();
    }
}