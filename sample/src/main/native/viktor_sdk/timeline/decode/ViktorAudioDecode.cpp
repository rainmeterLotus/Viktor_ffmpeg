//
// Created by rainmeterLotus on 2022/7/26.
//

#include "ViktorAudioDecode.h"

int ViktorAudioDecode::decode_start(ViktorContext *context, CClip *clip, bool isNow) {
    int ret = 0;
    if (clip->audio_in_codec_ctx) {
        int sample_rate, nb_channels;
        int64_t channel_layout;
        sample_rate = clip->audio_in_codec_ctx->sample_rate;
        nb_channels = clip->audio_in_codec_ctx->channels;
        channel_layout = clip->audio_in_codec_ctx->channel_layout;

        if ((ret = audio_open(context, channel_layout, nb_channels, sample_rate, &context->audio_tgt)) < 0) {
            return ret;
        }

        decoder_init(&context->auddec, clip->audio_in_codec_ctx,
                     &context->audio_packet_q, context->read_frame_cond);

        if ((clip->in_fmt_ctx->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK))
            && clip->in_fmt_ctx->iformat->read_seek) {
            AVStream *audioStream = clip->in_fmt_ctx->streams[clip->m_audio_index];
            /**
             * decoder_decode_frame 解码时纠正pts时使用
             */
            context->auddec.start_pts = audioStream->start_time;
            context->auddec.start_pts_tb = audioStream->time_base;
        }

        current_clip = clip;

        if ((ret = decoder_start(&context->auddec,audio_thread,context,this)) < 0){
            VIKTOR_LOGE("AVMEDIA_TYPE_VIDEO decoder_start fail");
            return ret;
        }

        if (context->m_audioEs){
            context->m_audioEs->pause_audio(0);
        }

    } else {
        return -1;
    }
    return ret;
}


/**
 *
 * @param opaque
 * @param wanted_channel_layout 为音频 通道格式类型 如 单通道 双通道 .....
 * @param wanted_nb_channels channels  为 音频的 通道数 1 2 3 4 5.....
 * @param wanted_sample_rate
 * @param audio_hw_params
 * @return
 *
 * channels 和 channel_layout相互之间获取
 * av_get_channel_layout_nb_channels()
av_get_default_channel_layout()

 channel_layout_map[]
	{ "mono",        1,  AV_CH_LAYOUT_MONO },
    { "stereo",      2,  AV_CH_LAYOUT_STEREO },
    { "2.1",         3,  AV_CH_LAYOUT_2POINT1 },
    { "3.0",         3,  AV_CH_LAYOUT_SURROUND },

 */
int ViktorAudioDecode::audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels,
                                  int wanted_sample_rate, struct ViktorAudioParams *audio_hw_params) {

    auto *is = static_cast<ViktorContext *>(opaque);
    SDL_AudioSpec wanted_spec, spec;

    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    // 根据channel_layout获取nb_channels，当传入参数wanted_nb_channels不匹配时，此处会作修正
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;// 声道数
    wanted_spec.freq = wanted_sample_rate; // 采样率
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        VIKTOR_LOGE("Invalid sample rate or channel count!\n");
        return -1;
    }

    // 从采样率数组中找到第一个不大于传入参数wanted_sample_rate的值
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }

    wanted_spec.format = AUDIO_S16SYS;// S表带符号，16是采样深度，SYS表采用系统字节序
    wanted_spec.silence = 0;
    // SDL声音缓冲区尺寸，单位是单声道采样点尺寸x声道数
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = opaque;

    while (SDL_OpenAudio(is, &wanted_spec, &spec) < 0) {
        VIKTOR_LOGI("SDL_OpenAudio (%d channels, %d Hz)\n",
                    wanted_spec.channels, wanted_spec.freq);
        if (is->abort_request)
            return -1;
        // 如果打开音频设备失败，则尝试用不同的声道数或采样率再试打开音频设备
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                VIKTOR_LOGE("No more combinations to try, audio open failed\n");
                return -1;
            }
        }

        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }

    if (spec.format != AUDIO_S16SYS) {
        VIKTOR_LOGE("SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }

    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            VIKTOR_LOGE("SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels = spec.channels;
    //返回保存音频数据所需的字节数
    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        VIKTOR_LOGE("av_samples_get_buffer_size failed\n");
        return -1;
    }

    return spec.size;
}

int ViktorAudioDecode::SDL_OpenAudio(ViktorContext *context, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
    if (desired && m_audioEs) {
        return m_audioEs->open_audio(desired, obtained);
    }
    return -1;
}

int ViktorAudioDecode::audio_thread(void *arg,void *context){
    auto *viktor_context = static_cast<ViktorContext *>(arg);
    auto *viktor_audio_decode = static_cast<ViktorAudioDecode *>(context);

    if (!viktor_context || !viktor_audio_decode) {
        VIKTOR_LOGE("audio_thread is or player is NULL");
        return AVERROR(ENOMEM);
    }

    auto *video_clip = viktor_audio_decode->current_clip;
    AVFrame *frame = av_frame_alloc();
    VKFrame *af;
    int got_frame = 0;
    AVRational tb;
    int ret = 0;
    if (!frame) return AVERROR(ENOMEM);

    do {
        if ((got_frame = viktor_audio_decode->decoder_decode_frame(&viktor_context->auddec, frame, video_clip)) < 0) {
            goto the_end;
        }

        if (got_frame){
            tb = (AVRational) {1, frame->sample_rate};

            if (!(af = frame_queue_peek_writable(&viktor_context->sample_frame_q))) {
                goto the_end;
            }

            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->pos = frame->pkt_pos;
            af->serial = viktor_context->auddec.pkt_serial;
            af->duration = av_q2d((AVRational) {frame->nb_samples, frame->sample_rate});

            av_frame_move_ref(af->frame, frame);
            frame_queue_push(&viktor_context->sample_frame_q);
        }
    } while (ret >= 0);
    the_end:
    av_frame_free(&frame);
    return ret;
}

void ViktorAudioDecode::sdl_audio_callback(void *opaque, uint8_t *stream, int len) {
    auto *is = static_cast<ViktorContext *>(opaque);
    int audio_size, len1;
    int64_t audio_callback_time;
    while (len > 0) {

    }
}
