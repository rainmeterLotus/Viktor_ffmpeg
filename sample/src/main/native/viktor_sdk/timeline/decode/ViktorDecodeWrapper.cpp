//
// Created by rainmeterLotus on 2022/7/25.
//

#include "ViktorDecodeWrapper.h"
int ViktorDecodeWrapper::decode_init(ViktorContext *context,CClip *clip,bool isNow){
    int ret = 0;

    if (clip != current_clip){
        ret = clip->openDecode(true);
    }

    if (ret < 0){
        return ret;
    }

    if (!m_video_decode) {
        m_video_decode = new ViktorVideoDecode();
    }

    ret = m_video_decode->decode_start(context, clip,isNow);

    current_clip = clip;
    return ret;
}

void ViktorDecodeWrapper::decode_destroy(ViktorContext *context){
    if (m_audio_decode){
        decoder_abort(&context->auddec,&context->sample_frame_q);
        decoder_destroy(&context->auddec);

        delete m_audio_decode;
        m_audio_decode = nullptr;
    }

    if (m_video_decode){
        decoder_abort(&context->viddec,&context->picture_frame_q);
        decoder_destroy(&context->viddec);

        delete m_video_decode;
        m_video_decode = nullptr;
    }
}