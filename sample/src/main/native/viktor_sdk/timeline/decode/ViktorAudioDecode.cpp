//
// Created by rainmeterLotus on 2022/7/26.
//

#include "ViktorAudioDecode.h"
#include "../../ffplay/soundtouch_wrap.h"

int ViktorAudioDecode::decode_start(ViktorContext *context, CClip *clip) {
    int ret = 0;

    VIKTOR_LOGE("ViktorAudioDecode::decode_start audio_in_codec_ctx:%p",clip->audio_in_codec_ctx);
    if (clip->audio_in_codec_ctx) {
        if (context->auddec.decoder_tid){//说明解码线程已经启动
            context->auddec.avctx = clip->audio_in_codec_ctx;
            if (context->m_audioEs) {
                VIKTOR_LOGE("ViktorAudioDecode::decode_start close_audio");
                context->m_audioEs->close_audio();
                VIKTOR_LOGE("ViktorAudioDecode::decode_start createOpenSLES");
                context->m_audioEs->createOpenSLES();
            }
        }

        int sample_rate, nb_channels;
        int64_t channel_layout;
        /**
         * 从audio_in_codec_ctx(即AVCodecContext)中获取音频格式参数
         */
        sample_rate = clip->audio_in_codec_ctx->sample_rate;
        nb_channels = clip->audio_in_codec_ctx->channels;
        channel_layout = clip->audio_in_codec_ctx->channel_layout;
        VIKTOR_LOGE("ViktorAudioDecode::decode_start audio_open");
        /**
         * 调用audio_open打开sdl音频输出，实际打开的设备参数保存在audio_tgt，返回值表示输出设备的缓冲区大小
         *
         * 由于不同的音频输出设备支持的参数不同，音轨的参数不一定能被输出设备支持（此时就需要重采样了），audio_tgt就保存了输出设备参数
         */
        if ((ret = audio_open(context, channel_layout, nb_channels, sample_rate, &context->audio_tgt)) < 0) {
            VIKTOR_LOGE("ViktorAudioDecode::decode_start audio_open fail ret:%d",ret);
            return ret;
        }

        context->audio_src = context->audio_tgt;
        context->audio_buf_size = 0;
        context->audio_buf_index = 0;

        if (!context->auddec.decoder_tid){//说明解码线程还没有启动，需要初始化
            decoder_init(&context->auddec, clip->audio_in_codec_ctx,
                         &context->audio_packet_q, context->read_frame_cond);
        }

        if ((clip->in_fmt_ctx->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK))
            && clip->in_fmt_ctx->iformat->read_seek) {
            AVStream *audioStream = clip->in_fmt_ctx->streams[clip->m_audio_index];
            /**
             * decoder_decode_frame 解码时纠正pts时使用
             */
            context->auddec.start_pts = audioStream->start_time;
            context->auddec.start_pts_tb = audioStream->time_base;
        }

    } else {
        return -1;
    }

    VIKTOR_LOGE("ViktorAudioDecode::decode_start context->auddec.decoder_tid:%p",context->auddec.decoder_tid);

    current_clip = clip;
    if (context->auddec.decoder_tid){
        if (context->m_audioEs) {
            context->m_audioEs->pause_audio(0);
        }
        return ret;
    }

    if ((ret = decoder_start(&context->auddec, audio_thread, context, this)) < 0) {
        VIKTOR_LOGE("AVMEDIA_TYPE_VIDEO decoder_start audio fail");
        return ret;
    }
    if (context->m_audioEs) {
        context->m_audioEs->pause_audio(0);
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
    if (desired && context->m_audioEs) {
        return context->m_audioEs->open_audio(desired, obtained);
    }
    return -1;
}

int ViktorAudioDecode::audio_thread(void *arg, void *context) {
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
        if (video_clip != viktor_audio_decode->current_clip){
            video_clip = viktor_audio_decode->current_clip;
        }
        if ((got_frame = viktor_audio_decode->decoder_decode_frame(viktor_context,&viktor_context->auddec, frame, video_clip)) < 0) {
            goto the_end;
        }

        if (got_frame) {
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
    int64_t audio_callback_time = av_gettime_relative();
    VIKTOR_LOGE("sdl_audio_callback start len:%d", len);

    /**
     * len长度是调用者需要的长度
     * 循环发送，直到发够所需数据长度
     */
    while (len > 0) {
        VIKTOR_LOGE("sdl_audio_callback while (len > 0) audio_buf_index:%d,audio_buf_size:%d", is->audio_buf_index, is->audio_buf_size);

        //如果audio_buf消耗完了，就调用audio_decode_frame重新填充audio_buf
        if (is->audio_buf_index >= is->audio_buf_size) {
            /**
             *
             * audio_decode_frame会给audio_buf赋值
             * audio_buf:从要输出的AVFrame中取出的音频数据
             */
            audio_size = audio_decode_frame(is);
            VIKTOR_LOGE("sdl_audio_callback audio_size:%d", audio_size);
            if (audio_size < 0) {
                is->audio_buf = nullptr;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            } else {
                if (is->show_mode != SHOW_MODE_VIDEO) {
                    //TODO update_sample_display
                }

                //audio_buf的总大小
                is->audio_buf_size = audio_size;
            }
            //下一次可读的audio_buf的index位置
            is->audio_buf_index = 0;
        }

        if (is->auddec.pkt_serial != is->audio_packet_q.serial) {
            is->audio_buf_index = is->audio_buf_size;
            memset(stream, 0, len);
            is->m_audioEs->flush_audio();
            break;
        }

        //根据缓冲区剩余大小量力而行
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }

        /**根据audio_volume决定如何输出audio_buf
         * 输出audio_buf到stream，如果audio_volume为最大音量，则只需memcpy复制给stream即可。
         * 否则，可以利用SDL_MixAudioFormat进行音量调整和混音
         */
        if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME) {
            memcpy(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1);
        } else {
            memset(stream, 0, len1);
            if (!is->muted && is->audio_buf) {
                SDL_MixAudio(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1, is->audio_volume);
            }
        }


        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
        VIKTOR_LOGE("sdl_audio_callback while len:%d", len);
        VIKTOR_LOGE("sdl_audio_callback len1:%d", len1);
        VIKTOR_LOGE("sdl_audio_callback is->audio_buf_index:%d", is->audio_buf_index);

    }

    VIKTOR_LOGE("sdl_audio_callback audio_buf_index:%d,audio_buf_size:%d,audio_write_buf_size:%d", is->audio_buf_index, is->audio_buf_size,is->audio_write_buf_size);
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    VIKTOR_LOGE("sdl_audio_callback is->audio_write_buf_size:%d,end------", is->audio_write_buf_size);
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(is->audio_clock)) {
        double pts = is->audio_clock - (double) (is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec -
                     is->m_audioEs->get_latency_seconds();
        update_audio_pts(is, pts, audio_callback_time / 1000000.0, is->audio_clock_serial);
    }


}

/**
 * 重采样
 * @param is
 * @return
 */
int ViktorAudioDecode::audio_decode_frame(ViktorContext *is) {
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    int wanted_nb_samples;
    int translate_time = 1;
    VKFrame *af;

    if (is->paused || is->step)
        return -1;
    reload:
    do {
        VIKTOR_LOGE("do audio_decode_frame");
        if (!(af = frame_queue_peek_readable(&is->sample_frame_q)))
            return -1;
        frame_queue_next(&is->sample_frame_q);
    } while (af->serial != is->audio_packet_q.serial);
    VIKTOR_LOGE("audio_decode_frame frame->channels:%d,frame->nb_samples:%d,frame->format:%d", af->frame->channels, af->frame->nb_samples,
                af->frame->format);

    //计算这一帧的字节数
    data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
                                           af->frame->nb_samples,
                                           static_cast<AVSampleFormat>(af->frame->format), 1);

    VIKTOR_LOGE("audio_decode_frame data_size:%d", data_size);
    //计算dec_channel_layout，用于确认是否需要重新初始化重采样
    dec_channel_layout =
            (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
            af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
    wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);
    VIKTOR_LOGE("audio_decode_frame dec_channel_layout:%ld,wanted_nb_samples:%d", dec_channel_layout, wanted_nb_samples);

    /**
    * 判断是否需要重新初始化重采样
       is->audio_tgt是SDL可接受的音频帧数，是audio_open()中取得的参数
       在audio_open()函数中又有“is->audio_src = is->audio_tgt”
       此处表示：
       如果frame中的音频参数 == is->audio_src == is->audio_tgt，那音频重采样的过程就免了(因此is->swr_ctr是NULL)
       否则使用frame(源)和is->audio_tgt(目标)中的音频参数来设置is->swr_ctx，并使用frame中的音频参数来赋值is->audio_src
    *
    */
    if (af->frame->format != is->audio_src.fmt ||
        dec_channel_layout != is->audio_src.channel_layout ||
        af->frame->sample_rate != is->audio_src.freq ||
        (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx)) {

        VIKTOR_LOGE("audio_decode_frame frame->format:%d,audio_src.fmt:%d", af->frame->format, is->audio_src.fmt);
        VIKTOR_LOGE("audio_decode_frame dec_channel_layout:%ld,audio_src.channel_layout:%ld", dec_channel_layout, is->audio_src.channel_layout);
        VIKTOR_LOGE("audio_decode_frame frame->sample_rate:%d,audio_src.freq:%d", af->frame->sample_rate, is->audio_src.freq);
        VIKTOR_LOGE("audio_decode_frame frame->nb_samples:%d,wanted_nb_samples:%d", af->frame->nb_samples, wanted_nb_samples);

        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL, is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
                                         dec_channel_layout, static_cast<AVSampleFormat>(af->frame->format), af->frame->sample_rate,
                                         0, NULL);

        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
            VIKTOR_LOGE(
                    "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                    af->frame->sample_rate, av_get_sample_fmt_name(static_cast<AVSampleFormat>(af->frame->format)), af->frame->channels,
                    is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
            return -1;
        }

        /**
         * 使用frame中的参数更新is->audio_src，第一次更新后后面基本不用执行此if分支了，因为一个音频流中各frame通用参数一样
         */
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels = af->frame->channels;
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = static_cast<AVSampleFormat>(af->frame->format);

    }

    /**
    * 获取这一帧的数据。对于frame格式和输出设备不同的，需要重采样；如果格式相同，则直接拷贝指针输出即可。
    * 总之，需要在audio_buf中保存与输出设备格式相同的音频数据
   */
    if (is->swr_ctx) {
        VIKTOR_LOGE("audio_decode_frame need resampled");
        const auto **in = (const uint8_t **) af->frame->extended_data;
        uint8_t **out = &is->audio_buf1;
        int out_count = (int) ((int64_t) wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256);
        int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            VIKTOR_LOGE("av_samples_get_buffer_size() failed\n");
            return -1;
        }

        if (wanted_nb_samples != af->frame->nb_samples) {
            if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
                                     wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0) {
                VIKTOR_LOGE("swr_set_compensation() failed\n");
                return -1;
            }
        }

        if (!is->audio_buf) {
            VIKTOR_LOGE("audio_decode_frame need resampled 00");
        }
        if (!is->audio_buf1_size) {
            VIKTOR_LOGE("audio_decode_frame need resampled 11");
        }

        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) {
            return AVERROR(ENOMEM);
        }

        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
//        if (len2 < 0) {
//            VIKTOR_LOGE("swr_convert() failed\n");
//            return -1;
//        }

        if (len2 == out_count) {
            VIKTOR_LOGE("audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0) {
                swr_free(&is->swr_ctx);
            }
        }
        is->audio_buf = is->audio_buf1;
        // 重采样返回的一帧音频数据大小(以字节为单位)
        // sample个数 * 通道数 * 每个sample的字节数
        int bytes_per_sample = av_get_bytes_per_sample(is->audio_tgt.fmt);
        VIKTOR_LOGE("audio_decode_frame bytes_per_sample:%d,is->audio_tgt.fmt:%d", bytes_per_sample, is->audio_tgt.fmt);
        resampled_data_size = len2 * is->audio_tgt.channels * bytes_per_sample;

        //是否变速处理
        if (soundtouch_enable && is->pf_playback_rate != 1.0f && !is->abort_request) {
            av_fast_malloc(&is->audio_new_buf, &is->audio_new_buf_size, out_size * translate_time);
            for (int i = 0; i < (resampled_data_size / 2); i++) {
                is->audio_new_buf[i] = (short) (is->audio_buf1[i * 2] | (is->audio_buf1[i * 2 + 1] << 8));
            }
            int ret_len = soundtouch_translate(is->soundtouch, is->audio_new_buf, (float) is->pf_playback_rate, (float) (1.0f / is->pf_playback_rate),
                                               resampled_data_size / 2, bytes_per_sample, is->audio_tgt.channels, af->frame->sample_rate);

            if (ret_len > 0) {
                is->audio_buf = (uint8_t *) is->audio_new_buf;
                resampled_data_size = ret_len;
            } else {
                translate_time++;
                goto reload;
            }
        }
    } else {
        VIKTOR_LOGE("audio_decode_frame no need resampled");
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
    }


    /**
     * 更新audio_clock，audio_clock_serial。用于设置audclk
     * update the audio clock with the pts */
    if (!isnan(af->pts)) {
        is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    } else {
        is->audio_clock = NAN;
    }
    is->audio_clock_serial = af->serial;
    return resampled_data_size;
}

int ViktorAudioDecode::synchronize_audio(ViktorContext *is, int nb_samples) {
    int wanted_nb_samples = nb_samples;
    if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {

    }
    return wanted_nb_samples;
}
