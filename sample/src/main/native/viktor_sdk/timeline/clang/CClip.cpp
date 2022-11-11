//
// Created by rainmeterLotus on 2021/12/18.
//

#include "CClip.h"
#include "../util/ViktorCommon.h"

int CClip::openFile(){

    VIKTOR_LOGI("CClip openFile:%s",m_file_path.c_str());

    int ret;
    /**
     * avformat_open_input默认是阻塞操作，如果不加控制，等待时间可能会达到30s以上
     * 可以如下处理
     * ic->interrupt_callback.callback = decode_interrupt_cb;
     * ic->interrupt_callback.opaque = is;
     */
    ret = avformat_open_input(&in_fmt_ctx,m_file_path.c_str(), nullptr, nullptr);
    if (ret < 0){
        VIKTOR_LOGI("CClip avformat_open_input ERROR:%s",av_err2str(ret));
        return ret;
    }

    /**
     * 需要花费较长的时间进行流格式探测
     * 如何减少探测时间？
     * 可以通过设置AVFotmatContext的probesize和max_analyze_duration属性进行调节
     * 又或者自己实现avformat_find_stream_info逻辑
     */
    in_fmt_ctx->max_analyze_duration = AV_TIME_BASE;
    ret = avformat_find_stream_info(in_fmt_ctx, nullptr);
    if (ret < 0){
        avformat_close_input(&in_fmt_ctx);
        VIKTOR_LOGI("CClip avformat_find_stream_info ERROR:%s",av_err2str(ret));
        return ret;
    }

    if (video_enable){
        m_video_index = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO,
                                            -1, -1, NULL, 0);
    }

    if (audio_enable){
        m_audio_index = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_AUDIO,
                                            -1, -1, NULL, 0);
    }

    m_duration_us = static_cast<long>(in_fmt_ctx->duration * (1000000 * av_q2d(AV_TIME_BASE_Q)));
    VIKTOR_LOGI("CClip m_duration_us:%ld",m_duration_us);
    if (m_video_index >= 0){
        AVStream *st = in_fmt_ctx->streams[m_video_index];
        AVCodecParameters *codecpar = st->codecpar;
        int mirrorImage;
        if (is_image_file(st)){
            m_orientation = get_image_orientation(m_file_path.c_str(),&mirrorImage);
        } else {
            m_orientation = get_video_orientation(st);
        }

        if(m_orientation % 180 == 90){
            m_width = codecpar->height;
            m_height = codecpar->width;
        } else {
            m_width = codecpar->width;
            m_height = codecpar->height;
        }

        double base = av_q2d(st->time_base);
        long durationUs = static_cast<long>(st->duration * base * 1000000);
        VIKTOR_LOGI("CClip video durationUs:%ld",durationUs);
        VIKTOR_LOGI("CClip video orientation:%d,width:%d,height:%d\n",m_orientation,m_width,m_height);
    }

    if (m_audio_index >= 0){
        AVStream *audioStream = in_fmt_ctx->streams[m_audio_index];
        double base = av_q2d(audioStream->time_base);
        long durationUs = static_cast<long>(audioStream->duration * base * 1000000);
        VIKTOR_LOGI("CClip audio durationUs:%ld",durationUs);
        VIKTOR_LOGI("CClip audio sample_rate = %d", audioStream->codecpar->sample_rate);
        VIKTOR_LOGI("CClip audio channels = %d", audioStream->codecpar->channels);
    }
    return 1;
}

int CClip::openDecode(bool isVideoStreamMain){
    if (!in_fmt_ctx)  return AVERROR(ENOMEM);
    int retVideo = 0;
    int retAudio = 0;

    if (m_audio_index >= 0 && !audio_in_codec_ctx){
        retAudio = init_decode(m_audio_index,&audio_in_codec_ctx);
    }

    if (m_video_index >= 0 && !video_in_codec_ctx){
        retVideo = init_decode(m_video_index,&video_in_codec_ctx);
    }

    if (isVideoStreamMain){
        return retVideo;
    }
    return retAudio;
}

int CClip::init_decode(int stream_index,AVCodecContext **avctx_p){
    AVCodec *codec;
    int ret = 0;
    AVCodecContext *avctx = *avctx_p;
    avctx = avcodec_alloc_context3(nullptr);
    if (!avctx) return AVERROR(ENOMEM);

    ret = avcodec_parameters_to_context(avctx,in_fmt_ctx->streams[stream_index]->codecpar);
    if (ret < 0) goto fail;
    avctx->pkt_timebase = in_fmt_ctx->streams[stream_index]->time_base;

    codec = avcodec_find_decoder(avctx->codec_id);

    if (!codec) {
        ret = AVERROR(EINVAL);

        VIKTOR_LOGI("No decoder could be found for codec '%s'\n", avcodec_get_name(avctx->codec_id));
        goto fail;
    }
    avctx->codec_id = codec->id;
    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
        VIKTOR_LOGI("avcodec_open2 fail");
        goto fail;
    }
    goto out;

    fail:
    avcodec_free_context(&avctx);
    out:
    *avctx_p = avctx;
    return ret;
}

void CClip::release(){
    if (in_fmt_ctx){
        avformat_close_input(&in_fmt_ctx);
    }

    if (video_in_codec_ctx){
        avcodec_free_context(&video_in_codec_ctx);
    }
    if (audio_in_codec_ctx){
        avcodec_free_context(&audio_in_codec_ctx);
    }
}