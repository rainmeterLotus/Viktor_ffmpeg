//
// Created by rainmeterLotus on 2022/7/26.
//

#ifndef VIKTOR_FFMPEG_VIKTORAUDIODECODE_H
#define VIKTOR_FFMPEG_VIKTORAUDIODECODE_H


#include "IViktorDecode.h"


class ViktorAudioDecode: public IViktorDecode {
public:
    int decode_start(ViktorContext *context,CClip *clip);

private:
    int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct ViktorAudioParams *audio_hw_params);
    int SDL_OpenAudio(ViktorContext *context,const SDL_AudioSpec *desired, SDL_AudioSpec *obtained);

    static VKFrame *sample_frame_queue_peek_readable(VKFrameQueue *f ,const int *is_audio_close);
    static int audio_thread(void *arg,void *context);
    static void sdl_audio_callback(void *opaque, uint8_t *stream, int len);

    static int audio_decode_frame(ViktorContext *is);
    static int synchronize_audio(ViktorContext *is, int nb_samples);

    SLAudio_ES *m_audioEs = nullptr;
};


#endif //VIKTOR_FFMPEG_VIKTORAUDIODECODE_H
