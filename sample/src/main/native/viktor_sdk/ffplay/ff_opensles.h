//
// Created by rainmeterLotus on 2021/7/20.
//

#ifndef VIKTOR_FFMPEG_FF_OPENSLES_H
#define VIKTOR_FFMPEG_FF_OPENSLES_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "ff_sdl.h"

#define OPENSLES_BUFFERS 2 /* maximum number of buffers */
#define OPENSLES_BUFLEN  10 /* ms */

extern "C"{
#include <libavutil/mem.h>
}

class SLAudio_ES {
public:
    void createOpenSLES();
    int open_audio(const SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
    void close_audio();
    void flush_audio();
    void pause_audio(int pause_on);

    double get_latency_seconds();


    static void opensles_callback(SLAndroidSimpleBufferQueueItf caller, void *pContext);

private:
    SDL_AudioSpec m_spec;
    SLObjectItf m_slObject = nullptr;
    SLEngineItf m_slEngine = nullptr;
    SLObjectItf m_slOutputMixObject = nullptr;
    SLObjectItf m_slPlayerObject = nullptr;
    SLPlayItf m_slPlayItf = nullptr;
    SLVolumeItf m_slVolumeItf = nullptr;
    SLAndroidSimpleBufferQueueItf m_slBufferQueueItf = nullptr;

    int bytes_per_frame;
    int milli_per_buffer;
    int frames_per_buffer;
    int bytes_per_buffer;

    uint8_t       *buffer;
    size_t         buffer_capacity;

    volatile bool  abort_request;
    volatile bool  pause_on;
    volatile bool  need_flush;
    volatile bool  is_running;

    std::thread *audio_tid;
    std::mutex wakeup_mutex;
    std::mutex normal_mutex;
    std::condition_variable wakeup_cond;


    static void audio_thread(void *context);
};


#endif //VIKTOR_FFMPEG_FF_OPENSLES_H
