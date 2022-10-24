//
// Created by rainmeterLotus on 2022/7/26.
//

#ifndef VIKTOR_FFMPEG_VIKTORAUDIODECODE_H
#define VIKTOR_FFMPEG_VIKTORAUDIODECODE_H


#include "IViktorDecode.h"


class ViktorAudioDecode: public IViktorDecode {
public:
    int decode_start(ViktorContext *context,CClip *clip,bool isNow);

private:
    int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct ViktorAudioParams *audio_hw_params);
    int SDL_OpenAudio(ViktorContext *context,const SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
    static int audio_thread(void *arg,void *context);
    static void sdl_audio_callback(void *opaque, uint8_t *stream, int len);

    SLAudio_ES *m_audioEs = nullptr;
};


#endif //VIKTOR_FFMPEG_VIKTORAUDIODECODE_H
