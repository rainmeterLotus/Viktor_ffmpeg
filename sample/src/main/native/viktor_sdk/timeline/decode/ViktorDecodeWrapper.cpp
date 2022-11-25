//
// Created by rainmeterLotus on 2022/7/25.
//

#include "ViktorDecodeWrapper.h"
int ViktorDecodeWrapper::decode_init(ViktorContext *context,CClip *clip,bool isNow){
    int ret = 0;
    VIKTOR_LOGE("ViktorDecodeWrapper::decode_init isNow:%d",isNow);
    if(clip){
        VIKTOR_LOGE("ViktorDecodeWrapper::decode_init clip in track index:%d",clip->m_index);
    }

    if(current_clip){
        VIKTOR_LOGE("ViktorDecodeWrapper::decode_init current_clip in track index:%d",current_clip->m_index);
    }
    if (clip != current_clip){
        ret = clip->openDecode(true);
    }

    if (ret < 0){
        return ret;
    }

    if (!m_video_decode) {
        m_video_decode = new ViktorVideoDecode();
    }

    if (!m_audio_decode) {
        m_audio_decode = new ViktorAudioDecode();
    }

    if (!context->wait_decode_cond){
        context->wait_decode_cond = sdl_create_cond();
    }

    /**
     * 是否立马开始解码
     * seek的需要立马开始解码
     */
    if (isNow){
        context->decode_state = 0;
        context->wait_decode_cond->notify_one();
    }

    std::mutex *wait_mutex = sdl_create_mutex();
    if (context->decode_state > 0){
        VIKTOR_LOGE("decode_start wait_decode_cond---");
        sdl_cond_wait(context->wait_decode_cond,wait_mutex);
    }
    delete wait_mutex;

    context->max_frame_duration = (clip->in_fmt_ctx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    context->decode_state = 1;
    ret = m_video_decode->decode_start(context, clip);
    if (m_audio_decode){
        m_audio_decode->decode_start(context, clip);
    }

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