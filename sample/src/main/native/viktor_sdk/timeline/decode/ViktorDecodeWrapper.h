//
// Created by rainmeterLotus on 2022/7/25.
//

#ifndef VIKTOR_FFMPEG_VIKTORDECODEWRAPPER_H
#define VIKTOR_FFMPEG_VIKTORDECODEWRAPPER_H

#include "../context/viktor_context.h"
#include "../clang/CClip.h"
#include "ViktorVideoDecode.h"
#include "ViktorAudioDecode.h"

class ViktorDecodeWrapper {
public:
    int decode_init(ViktorContext *context,CClip *clip,bool isNow);
    void decode_destroy(ViktorContext *context);
    CClip *current_clip = nullptr;

    ViktorVideoDecode *m_video_decode = nullptr;
    ViktorAudioDecode *m_audio_decode = nullptr;
};


#endif //VIKTOR_FFMPEG_VIKTORDECODEWRAPPER_H
