//
// Created by rainmeterLotus on 2021/7/20.
//

#include "ff_opensles.h"
#include "../timeline//util/ViktorLog.h"
int SLAudio_ES::open_audio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained){
    VIKTOR_LOGI("SLAudio_ES::open_audio");
    if (!m_slEngine){
        return -1;
    }

    if (!m_slOutputMixObject){
        return -1;
    }
    SLresult re = 0;
    m_spec = *desired;

    // config audio src
    //配置音频信息
    //缓冲队列
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = desired->channels;
    format_pcm.samplesPerSec = desired->freq * 1000;// milli Hz
    format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    switch (desired->channels) {
        case 2:
            format_pcm.channelMask  = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
            break;
        case 1:
            format_pcm.channelMask  = SL_SPEAKER_FRONT_CENTER;
            break;
    }

    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,OPENSLES_BUFFERS};

    SLDataSource audio_source = {&loc_bufq,&format_pcm};

    // config audio sink
    SLDataLocator_OutputMix loc_outmix = {
            SL_DATALOCATOR_OUTPUTMIX,
            m_slOutputMixObject
    };
    SLDataSink audio_sink = {&loc_outmix, NULL};

    SLObjectItf slPlayerObject = nullptr;
    const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAY};
    const SLboolean  req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    re = (*m_slEngine)->CreateAudioPlayer(m_slEngine,&slPlayerObject,&audio_source,&audio_sink, sizeof(ids) / sizeof(*ids),ids,req);
    if (re != SL_RESULT_SUCCESS){
        VIKTOR_LOGI("(*m_slEngine)->CreateAudioPlayer fail");
        close_audio();
        return -1;
    }
    m_slPlayerObject = slPlayerObject;

    re = (*slPlayerObject)->Realize(slPlayerObject,SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS){
        VIKTOR_LOGI("(*slPlayerObject)->Realize fail");
        close_audio();
        return -1;
    }

    re = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_PLAY, &m_slPlayItf);
    if (re != SL_RESULT_SUCCESS){
        VIKTOR_LOGI("(*slPlayerObject)->GetInterface m_slPlayItf fail");
        close_audio();
        return -1;
    }

    re = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_VOLUME, &m_slVolumeItf);
    if (re != SL_RESULT_SUCCESS){
        VIKTOR_LOGI("(*slPlayerObject)->GetInterface m_slVolumeItf fail");
        close_audio();
        return -1;
    }

    re = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &m_slBufferQueueItf);
    if (re != SL_RESULT_SUCCESS){
        VIKTOR_LOGI("(*slPlayerObject)->GetInterface m_slBufferQueueItf fail");
        close_audio();
        return -1;
    }

    //设置回调函数，播放队空调用
    re = (*m_slBufferQueueItf)->RegisterCallback(m_slBufferQueueItf,opensles_callback,this);
    if (re != SL_RESULT_SUCCESS){
        VIKTOR_LOGI("(*m_slBufferQueueItf)->RegisterCallback fail");
        close_audio();
        return -1;
    }
    bytes_per_frame  = format_pcm.numChannels * format_pcm.bitsPerSample / 8;
    milli_per_buffer  = OPENSLES_BUFLEN;
    frames_per_buffer = milli_per_buffer * format_pcm.samplesPerSec / 1000000; // samplesPerSec is in milli
    bytes_per_buffer  = bytes_per_frame * frames_per_buffer;
    buffer_capacity   = OPENSLES_BUFFERS * bytes_per_buffer;
    VIKTOR_LOGI("OpenSL-ES: bytes_per_frame  = %d bytes\n",  (int)bytes_per_frame);
    VIKTOR_LOGI("OpenSL-ES: milli_per_buffer = %d ms\n",     (int)milli_per_buffer);
    VIKTOR_LOGI("OpenSL-ES: frame_per_buffer = %d frames\n", (int)frames_per_buffer);
    VIKTOR_LOGI("OpenSL-ES: bytes_per_buffer = %d bytes\n",  (int)bytes_per_buffer);
    VIKTOR_LOGI("OpenSL-ES: buffer_capacity  = %d bytes\n",  (int)buffer_capacity);
    buffer          = static_cast<uint8_t *>(malloc(buffer_capacity));

    memset(buffer, 0, buffer_capacity);
    for(int i = 0; i < OPENSLES_BUFFERS; ++i) {
        re = (*m_slBufferQueueItf)->Enqueue(m_slBufferQueueItf,buffer + i* bytes_per_buffer,bytes_per_buffer);
        VIKTOR_LOGI("SLAudio_ES::open_audio Enqueue re:%d",re);
        if (re != SL_RESULT_SUCCESS){
            close_audio();
            return -1;
        }
    }

    pause_on = 1;
    abort_request = 0;
    audio_tid = new std::thread(audio_thread,this);

    if (obtained){
        *obtained = *desired;
        obtained->size = buffer_capacity;
        obtained->freq = format_pcm.samplesPerSec / 1000;
    }
    VIKTOR_LOGI("SLAudio_ES::open_audio ok");
    return buffer_capacity;
}

void SLAudio_ES::close_audio(){
    VIKTOR_LOGI("SLAudio_ES::close_audio");
    {
        std::lock_guard<std::mutex> lock(normal_mutex);
        abort_request = true;
        wakeup_cond.notify_one();
    }
    VIKTOR_LOGI("SLAudio_ES::close_audio wait");
    SDL_WaitThread(audio_tid);
    audio_tid = nullptr;
    VIKTOR_LOGI("SLAudio_ES::close_audio wait end");
    if (m_slPlayItf){
        (*m_slPlayItf)->SetPlayState(m_slPlayItf,SL_PLAYSTATE_STOPPED);
        m_slPlayItf = nullptr;
    }
    if (m_slBufferQueueItf){
        (*m_slBufferQueueItf)->Clear(m_slBufferQueueItf);
        m_slBufferQueueItf = nullptr;
    }

    if (m_slVolumeItf){
        m_slVolumeItf = nullptr;
    }

    if (m_slPlayerObject){
        (*m_slPlayerObject)->Destroy(m_slPlayerObject);
        m_slPlayerObject = nullptr;
    }

    if (m_slOutputMixObject){
        (*m_slOutputMixObject)->Destroy(m_slOutputMixObject);
        m_slOutputMixObject = nullptr;
    }

    if (m_slObject){
        (*m_slObject)->Destroy(m_slObject);
        m_slObject = nullptr;
    }

    m_slEngine = nullptr;


    av_freep(&buffer);
    VIKTOR_LOGI("SLAudio_ES::close_audio finish");
}

void SLAudio_ES::flush_audio(){
    VIKTOR_LOGI("flush_audio");
    std::lock_guard<std::mutex> lock(normal_mutex);
    need_flush = true;
    wakeup_cond.notify_one();
}

void SLAudio_ES::pause_audio(int pause){
    std::lock_guard<std::mutex> lock(normal_mutex);
    pause_on = pause;
    if (!pause){
        wakeup_cond.notify_one();
    }
}

double SLAudio_ES::get_latency_seconds(){
    SLAndroidSimpleBufferQueueState state = {0};
    SLresult slRet = (*m_slBufferQueueItf)->GetState(m_slBufferQueueItf,&state);
    if (slRet != SL_RESULT_SUCCESS){
        return ((double)milli_per_buffer) * OPENSLES_BUFFERS /1000;
    }

    return ((double)milli_per_buffer) * state.count / 1000;
}

void SLAudio_ES::createOpenSLES(){
    SLresult re;
    SLObjectItf slObject = nullptr;
    re = slCreateEngine(&slObject,0,nullptr,0,nullptr,nullptr);
    if (re != SL_RESULT_SUCCESS) return;
    re = (*slObject)->Realize(slObject,SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) return;
    m_slObject = slObject;

    SLEngineItf slEngine = nullptr;
    re = (*slObject)->GetInterface(slObject,SL_IID_ENGINE,&slEngine);
    if(re != SL_RESULT_SUCCESS) return;
    m_slEngine = slEngine;

    SLObjectItf slOutputMixObject = nullptr;
    const SLInterfaceID ids[] = {SL_IID_VOLUME};
    const SLboolean  req[] = {SL_BOOLEAN_FALSE};
    re = (*slEngine)->CreateOutputMix(slEngine, &slOutputMixObject, 1, ids, req);
    if (re != SL_RESULT_SUCCESS) return;
    re = (*slOutputMixObject)->Realize(slOutputMixObject,SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) return;
    m_slOutputMixObject = slOutputMixObject;
}

void SLAudio_ES::opensles_callback(SLAndroidSimpleBufferQueueItf caller, void *pContext){

    auto *opaque = static_cast<SLAudio_ES *>(pContext);
    std::lock_guard<std::mutex> lock(opaque->normal_mutex);
    opaque->wakeup_cond.notify_one();
}

void SLAudio_ES::audio_thread(void *context){
    VIKTOR_LOGI("SLAudio_ES::audio_thread start");
    auto *opaque = static_cast<SLAudio_ES *>(context);
    SLPlayItf slPlayItf  = opaque->m_slPlayItf;
    SLAndroidSimpleBufferQueueItf  slBufferQueueItf = opaque->m_slBufferQueueItf;
    SDL_AudioCallback audio_cblk = opaque->m_spec.callback;
    void *userdata         = opaque->m_spec.userdata;

    uint8_t *next_buffer = nullptr;
    int next_buffer_index = 0;
    size_t bytes_per_buffer = opaque->bytes_per_buffer;

    if (!opaque->abort_request && !opaque->pause_on){
        (*slPlayItf)->SetPlayState(slPlayItf,SL_PLAYSTATE_PLAYING);
    }

    std::mutex wait_mutex;
    while (!opaque->abort_request){
        VIKTOR_LOGI("SLAudio_ES::audio_thread while start");
        SLAndroidSimpleBufferQueueState slState = {0};
        SLresult slRet = (*slBufferQueueItf)->GetState(slBufferQueueItf,&slState);
        if (slRet != SL_RESULT_SUCCESS){
            opaque->wakeup_mutex.unlock();
        }
        opaque->wakeup_mutex.lock();
        if (!opaque->abort_request && (opaque->pause_on || slState.count >= OPENSLES_BUFFERS)){
            while (!opaque->abort_request && (opaque->pause_on || slState.count >= OPENSLES_BUFFERS)) {
                VIKTOR_LOGI("SLAudio_ES::audio_thread opaque->pause_on:%d,count:%d",opaque->pause_on,slState.count >= OPENSLES_BUFFERS);
                if (!opaque->pause_on) {
                    (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);
                }
                std::unique_lock<std::mutex> lock(wait_mutex);
                VIKTOR_LOGI("SLAudio_ES::audio_thread wait_for start");
                opaque->wakeup_cond.wait_for(lock, std::chrono::milliseconds(1000));
                VIKTOR_LOGI("SLAudio_ES::audio_thread wait_for end");
                SLresult slRet = (*slBufferQueueItf)->GetState(slBufferQueueItf,&slState);
                if (slRet != SL_RESULT_SUCCESS){
                    opaque->wakeup_mutex.unlock();
                }

                if (opaque->pause_on){
                    (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PAUSED);
                }

                VIKTOR_LOGI("SLAudio_ES::audio_thread while while end");
            }

            if (!opaque->abort_request && !opaque->pause_on) {
                (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);
            }
        }

        VIKTOR_LOGI("SLAudio_ES::audio_thread while need_flush:%d",opaque->need_flush);
        if (opaque->need_flush) {
            opaque->need_flush = false;
            (*slBufferQueueItf)->Clear(slBufferQueueItf);
        }

        //todo volume

        opaque->wakeup_mutex.unlock();
        next_buffer = opaque->buffer + next_buffer_index * bytes_per_buffer;
        next_buffer_index = (next_buffer_index + 1) % OPENSLES_BUFFERS;

        /**
         * 在ViktorAudioDecode::decode_start方法中会调用context->m_audioEs->close_audio()--->context->m_audioEs->close_audio()
         * 会触发audio_thread方法执行，
         * 下面audio_cblk调用ViktorAudioDecode::sdl_audio_callback--->audio_decode_frame--->frame_queue_peek_readable
         * frame_queue_peek_readable有可能卡死,这里再判断一次是否已经中断了
         */
        if (opaque->abort_request){
            VIKTOR_LOGI("SLAudio_ES::audio_cblk before is abort_request");
            break;
        }
        VIKTOR_LOGI("SLAudio_ES::audio_cblk start");
        audio_cblk(userdata,next_buffer,bytes_per_buffer);
        VIKTOR_LOGI("SLAudio_ES::audio_cblk end");
        if (opaque->need_flush){
            (*slBufferQueueItf)->Clear(slBufferQueueItf);
            opaque->need_flush = false;
        }

        if (opaque->need_flush) {
            VIKTOR_LOGI("SLAudio_ES::flush");
            opaque->need_flush = false;
            (*slBufferQueueItf)->Clear(slBufferQueueItf);
        } else {
            //播放音频数据
            slRet = (*slBufferQueueItf)->Enqueue(slBufferQueueItf, next_buffer, bytes_per_buffer);
            if (slRet == SL_RESULT_SUCCESS) {
                // do nothing
            } else if (slRet == SL_RESULT_BUFFER_INSUFFICIENT) {
                // don't retry, just pass through
                VIKTOR_LOGI("SL_RESULT_BUFFER_INSUFFICIENT");
            } else {
                VIKTOR_LOGI("slBufferQueueItf->Enqueue() = %d\n", (int)slRet);
                break;
            }
        }

        VIKTOR_LOGI("SLAudio_ES::audio_thread while end");
    }

}