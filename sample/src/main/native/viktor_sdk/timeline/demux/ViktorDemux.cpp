//
// Created by rainmeterLotus on 2021/12/18.
//

#include "ViktorDemux.h"

void ViktorDemux::read_frame_thread(ViktorContext *context) {
    AVPacket pkt1, *pkt = &pkt1;
    int track_index = 0;
    //todo stream_seek 方法中也需要改变该值
    int64_t current_pts = 0;
    AVFormatContext *ic = nullptr;
    CClip *current_clip = nullptr;
    CClip *prev_clip = nullptr;

    std::mutex *wait_mutex = sdl_create_mutex();
    int ret = 0;
    for (;;) {
        if (context->abort_request) {
            VIKTOR_LOGD("read_frame_thread break");
            break;
        }
        CClip *find_clip = nullptr;
        if (m_start_us != -1){
            find_clip = find_current_clip(context, m_start_us, track_index);
            VIKTOR_LOGD("read_frame_thread find_clip :%p",find_clip);
            VIKTOR_LOGD("read_frame_thread current_clip :%p",current_clip);
            handle_clip_seek(context, find_clip);
            if (find_clip && init_decode(context, find_clip) < 0) {
                VIKTOR_LOGE("read_frame_thread foreach init_decode fail");
                break;
            }
            prev_clip = current_clip;
            current_clip = find_clip;
            current_pts = 0;
        }

        if (!prev_clip) {
            prev_clip = current_clip;
        }

        if (!current_clip || !(ic = current_clip->in_fmt_ctx)) {
            VIKTOR_LOGE("find_right_context fail break");
            break;
        }

        //针对网络流
        handle_pause(context, current_clip);
        //处理seek操作
        handle_seek(context, ic, current_clip,prev_clip,pkt);

        /**
        PacketQueue默认情况下会有大小限制，达到这个大小后，就需要等待10ms，以让消费者——解码线程能有时间消耗。
        两种可能:
        1.audioq，videoq，subtitleq三个PacketQueue的总字节数达到了MAX_QUEUE_SIZE（15M）
        2.音频、视频、字幕流都已有够用的包（stream_has_enough_packets）
        */
        if (handle_enough(context, current_clip)) {
            /** wait 10 ms */
            std::unique_lock<std::mutex> lock(*wait_mutex);
            context->read_frame_cond->wait_for(lock, std::chrono::milliseconds(10));
//            VIKTOR_LOGE("infinite_buffer <1 wait 10 ms");
            continue;
        }


        ret = av_read_frame(ic, pkt);
        VIKTOR_LOGE("av_read_frame ret:%d", ret);
        if (ret < 0) {
            VIKTOR_LOGE("av_read_frame %s", av_err2str(ret));
            if (ret == AVERROR_EOF || avio_feof(ic->pb)) {
                //todo 处理当前片段是否结束
            }

            //发生错误了，退出主循环
            if (ic->pb && ic->pb->error) {
                break;
            }

            //如果都不是，只是要等一等
            /** wait 10 ms */
            std::unique_lock<std::mutex> lock(*wait_mutex);
            context->read_frame_cond->wait_for(lock, std::chrono::milliseconds(10));
            continue;
        }



        if (pkt->stream_index == current_clip->m_audio_index) {
            if (pkt->pts >= 0){
               // current_pts = pkt->pts;
            }
            VIKTOR_LOGE("read_frame_thread  packet_queue_put audio pkt->pts:%ld",pkt->pts);
            VIKTOR_LOGE("read_frame_thread  packet_queue_put audio current_pts:%ld",current_pts);
            packet_queue_put(&context->audio_packet_q, pkt);
        } else if (pkt->stream_index == current_clip->m_video_index) {
            if (pkt->pts >= 0){
                current_pts = pkt->pts * av_q2d(current_clip->in_fmt_ctx->streams[pkt->stream_index]->time_base)* 1000000;
            }
            VIKTOR_LOGE("read_frame_thread  packet_queue_put video pkt->pts:%ld",pkt->pts);
            VIKTOR_LOGE("read_frame_thread  packet_queue_put video current_pts:%ld",current_pts);
            if (pkt->flags & AV_PKT_FLAG_KEY){
                VIKTOR_LOGE("read_frame_thread is key frame...");
            } else {
                VIKTOR_LOGE("read_frame_thread no key frame...");
            }

            packet_queue_put(&context->video_packet_q, pkt);
        } else {
            av_packet_unref(pkt);
        }

        if (current_pts > current_clip->m_end_micro_sec){

            if (current_clip->isLast){
                for (;;){
                    if (context->abort_request) {
                        VIKTOR_LOGD("read_frame_thread break is_loop");
                        break;
                    }
                    bool isLoop = is_loop(context, current_clip);
                    if (!isLoop){
                        av_usleep(100);
                        continue;
                    } else {
                        m_start_us = 0;
                        break;
                    }
                }

            } else {
                m_start_us = current_clip->m_out_point_us;
            }

            VIKTOR_LOGE("av_read_frame change m_start_us:%ld", m_start_us);
        }


    }//end for(;;)

    delete wait_mutex;
}

CVideoClip *ViktorDemux::find_current_clip(ViktorContext *context, int64_t pts, int track_index) {
    if (!context || !context->vec_video_track) {
        return nullptr;
    }

    if (context->vec_video_track->empty() ||
        track_index < 0 ||
        track_index >= context->vec_video_track->size()) {
        return nullptr;
    }

    CVideoTrack *find_video_track = context->vec_video_track->at(track_index);
    if (!find_video_track) {
        return nullptr;
    }

    std::vector<CVideoClip *> *video_clips = find_video_track->getVideoClips();
    CVideoClip *findClip = nullptr;
    for (auto clip:*video_clips) {

        bool isLimitOut = pts < clip->m_out_point_us;
        if (clip->isLast){
            isLimitOut = pts <= clip->m_out_point_us;
        }

        if (pts >= clip->m_in_point_us && isLimitOut) {
            findClip = clip;
            break;
        }
    }

    if (!findClip){
        return nullptr;
    }


    int64_t real_pos = findClip->m_start_micro_sec + (pts - findClip->m_in_point_us);
    findClip->m_seek_to_us = real_pos;
    return findClip;
}

AVFormatContext *ViktorDemux::get_right_context(ViktorContext *context, int64_t pts) {
    if (!context || !context->vec_video_track) {
        return nullptr;
    }

    int track_index = 0;
    int track_clip_index = 0;
    if (context->vec_video_track->empty() ||
        track_index < 0 ||
        track_index >= context->vec_video_track->size()) {
        return nullptr;
    }

    CVideoTrack *find_video_track = context->vec_video_track->at(track_index);
    if (!find_video_track) {
        return nullptr;
    }
    if (find_video_track->getVideoClips()->empty() ||
        track_clip_index < 0 ||
        track_clip_index >= find_video_track->getVideoClips()->size()) {
        return nullptr;
    }

    CVideoClip *find_video_clip = find_video_track->getVideoClips()->at(track_clip_index);
    if (!find_video_clip) {
        return nullptr;
    }


    if (find_video_clip->in_fmt_ctx) {
        int findStreamIndex = -1;
        if (find_video_clip->m_video_index >= 0) {
            findStreamIndex = find_video_clip->m_video_index;
        } else {
            findStreamIndex = find_video_clip->m_audio_index;
        }

        if (findStreamIndex < 0) {
            return nullptr;
        }

        AVStream *findStream = find_video_clip->in_fmt_ctx->streams[findStreamIndex];
        if (findStream) {
            long pts_to_us = pts * av_q2d(findStream->time_base) * 1000000;

            if (pts_to_us >= find_video_clip->m_end_micro_sec) {

            }
        }


        return find_video_clip->in_fmt_ctx;
    }

    return nullptr;
}

void ViktorDemux::handle_clip_seek(ViktorContext *context, CClip *clip) {
    if (!clip) return;
    seek_to(context, clip->m_seek_to_us, 0, 0);
    clip->m_seek_to_us = 0;
}


/**
 *
 * @param context
 * @param in_fmt_ctx
 * @param clip 传入的是preClip，
 *  clip1          clip2        clip3...
 * -----------|-----------|------------>
 *假设clip1包含audio和video，此时seek到clip2，clip2没有audio，需要清除之前放入VKPacketQueue的数据
 * @return
 */
int ViktorDemux::handle_seek(ViktorContext *context, AVFormatContext *in_fmt_ctx, CClip *curClip,CClip *preClip,AVPacket *pkt) {
    VIKTOR_LOGD("handle_seek seek_req:%d,seek_pos:%ld",context->seek_req,context->seek_pos);
    if (!context->seek_req) {
        return 0;
    }

    int ret;
    int64_t seek_target = context->seek_pos;
    VIKTOR_LOGD("handle_seek seek_target:%ld", seek_target);
    int64_t seek_min = context->seek_rel > 0 ? seek_target - context->seek_rel + 2 : INT64_MIN;
    int64_t seek_max = context->seek_rel < 0 ? seek_target - context->seek_rel - 2 : INT64_MAX;
    ret = avformat_seek_file(in_fmt_ctx, -1, seek_min, seek_target, seek_max, 0);
    if (ret < 0){
        ret = avformat_seek_file(in_fmt_ctx, -1, seek_min, seek_target, seek_max, 0);
    }


    if (ret >= 0) {
        /**
         1.清除PacketQueue的缓存，并放入一个flush_pkt。放入的flush_pkt可以让PacketQueue的serial增1，以区分seek前后的数据
         2.同步外部时钟
         */
        if (preClip->m_audio_index >= 0) {
            packet_queue_flush(&context->audio_packet_q);
            packet_queue_put(&context->audio_packet_q, &flush_pkt);
        }

        if (preClip->m_video_index >= 0) {
            packet_queue_flush(&context->video_packet_q);
            packet_queue_put(&context->video_packet_q, &flush_pkt);
        }

        if (context->seek_flags & AVSEEK_FLAG_BYTE) {
            set_clock(&context->extclk, NAN, 0);
        } else {
            set_clock(&context->extclk, seek_target / (double) AV_TIME_BASE, 0);
        }
    }


    int64_t seek_master_rel = curClip->m_in_point_us + (seek_target - curClip->m_start_micro_sec);
    VIKTOR_LOGD("handle_seek seek_master_rel:%ld", seek_master_rel);
    update_master_pts(&context->extclk,seek_master_rel/1000000.0);

    context->seek_req = 0;
    if (context->paused) {//如果当前是暂停状态，就跳到seek后的下一帧，以直观体现seek成功了
        step_to_next_frame(context);
    }

    m_start_us = -1;
    is_seek = false;
    return ret;
}

void ViktorDemux::stream_seek(ViktorContext *context, int64_t pos, int64_t rel, int seek_by_bytes) {
    VIKTOR_LOGD("stream_seek pos:%ld",pos);
    m_start_us = pos;
    is_seek = true;
}


void ViktorDemux::seek_to(ViktorContext *context, int64_t pos, int64_t rel, int seek_by_bytes) {
    if (!context->seek_req) {
        context->seek_pos = pos;
        context->seek_rel = rel;
        context->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes) {
            context->seek_flags |= AVSEEK_FLAG_BYTE;
        }
        context->seek_req = 1;
        context->read_frame_cond->notify_one();
    }
}

/**
 * av_read_pause和av_read_play对于URLProtocol，会调用其url_read_pause，
 * 通过参数区分是要暂停还是恢复。对于AVInputFormat会调用其read_pause和read_play.
一般情况下URLProtocol和AVInputFormat都不需要专门处理暂停和恢复，
 但对于像rtsp/rtmp这种在通讯协议上支持(需要)暂停、恢复的就特别有用了
 * @param context
 * @param in_fmt_ctx
 * @param clip
 */
void ViktorDemux::handle_pause(ViktorContext *context, CClip *clip) {
    if (context->paused != context->last_paused && clip->in_fmt_ctx) {
        context->last_paused = context->paused;
        if (context->paused) {
            context->read_pause_return = av_read_pause(clip->in_fmt_ctx);
        } else {
            av_read_play(clip->in_fmt_ctx);
        }
    }
}

bool ViktorDemux::handle_enough(ViktorContext *context, CClip *clip) {
    if (!clip) return false;
    bool isMaxQueueSize = (context->audio_packet_q.size + context->video_packet_q.size) > MAX_QUEUE_SIZE;

    int audio_stream_index = clip->m_audio_index;
    int video_stream_index = clip->m_video_index;
//    VIKTOR_LOGD("handle_enough audio_stream_index:%d,video_stream_index:%d",audio_stream_index,video_stream_index);

    AVStream *audioStream = nullptr;
    if (audio_stream_index >= 0){
        audioStream = clip->in_fmt_ctx->streams[audio_stream_index];
    }
    AVStream *videoStream = nullptr;
    if (video_stream_index >= 0){
        videoStream = clip->in_fmt_ctx->streams[video_stream_index];
    }
    bool isEnough = stream_has_enough_packets(audioStream, audio_stream_index, &context->audio_packet_q) &&
                    stream_has_enough_packets(videoStream, video_stream_index, &context->video_packet_q);
//    VIKTOR_LOGD("handle_enough isMaxQueueSize:%d,isEnough:%d",isMaxQueueSize,isEnough);

    return isMaxQueueSize || isEnough;
}

bool ViktorDemux::is_loop(ViktorContext *context,CClip *clip){
    bool isAudioFinish = !clip->audio_in_codec_ctx || (context->auddec.finished == context->audio_packet_q.serial && frame_queue_nb_remaining(&context->sample_frame_q) == 0);
    bool isVideoFinish = !clip->video_in_codec_ctx || (context->viddec.finished == context->video_packet_q.serial && frame_queue_nb_remaining(&context->picture_frame_q) == 0);
    VIKTOR_LOGD("viddec.finished:%d,video_packet_q.serial:%d,picture_frame_q:%d",context->viddec.finished, context->video_packet_q.serial,frame_queue_nb_remaining(&context->picture_frame_q));
    VIKTOR_LOGD("isAudioFinish:%d,isVideoFinish:%d",isAudioFinish,isVideoFinish);
    if (!context->paused && isAudioFinish && isVideoFinish){
        if (isLoop()){
            VIKTOR_LOGD("is_loop");
            return true;
        } else if(autoexit){
            VIKTOR_LOGD("autoexit");
        }
    }

    return false;
}


int ViktorDemux::init_decode(ViktorContext *context, CClip *clip) {
    int ret = -1;
    if (!clip) return ret;

    if (!m_decode_wrapper) {
        m_decode_wrapper = new ViktorDecodeWrapper();
    }
    VIKTOR_LOGD("init_decode m_decode_wrapper :%p",m_decode_wrapper);
    ret = m_decode_wrapper->decode_init(context, clip,is_seek);

    return ret;
}

void ViktorDemux::stream_component_close(ViktorContext *context){
    if (m_decode_wrapper){
        m_decode_wrapper->decode_destroy(context);
        delete m_decode_wrapper;
        m_decode_wrapper = nullptr;
    }
}