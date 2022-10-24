//
// Created by rainmeterLotus on 2022/1/4.
//

#include "ViktorVideoDecode.h"

int ViktorVideoDecode::decode_start(ViktorContext *context,CClip *clip,bool isNow){
    int ret = 0;

    if (clip->video_in_codec_ctx){
        VIKTOR_LOGE("decode_start before &context->viddec:%p,isNow:%d",&context->viddec,isNow);
        VIKTOR_LOGE("decode_start before &context->viddec.decoder_tid:%p",context->viddec.decoder_tid);

        if (context->viddec.decoder_tid){//说明解码线程已经启动

            /**
             * 是否立马开始解码
             * seek的需要立马开始解码
             */
            if (isNow){
                context->viddec.avctx = clip->video_in_codec_ctx;
                context->viddec.decode_state = 1;
//                avcodec_flush_buffers(clip->video_in_codec_ctx);
            } else {
                std::mutex *wait_mutex = sdl_create_mutex();
                for (;;){
                    if (context->abort_request){
                        break;
                    }
                    if (context->viddec.decode_state > 0){
                        VIKTOR_LOGE("decode_start wait_decode_cond---");
                        std::unique_lock<std::mutex> lock(*wait_mutex);
                        context->viddec.wait_decode_cond->wait_for(lock, std::chrono::milliseconds(10));
                        continue;
                    }

                    if (context->viddec.decode_state == 0){
                        context->viddec.avctx = clip->video_in_codec_ctx;
                        context->viddec.decode_state = 1;
//                        avcodec_flush_buffers(clip->video_in_codec_ctx);
                    }
                    break;
                }
                VIKTOR_LOGE("decode_start wait_decode_cond---go");
                delete wait_mutex;
            }

        } else {
            decoder_init(&context->viddec,clip->video_in_codec_ctx,
                         &context->video_packet_q,context->read_frame_cond);
        }

        VIKTOR_LOGE("decode_start after &context->viddec:%p",&context->viddec);
    } else {
        VIKTOR_LOGE("decode_start not found AVCodecContext:%p",clip->video_in_codec_ctx);
        return -1;
    }

    current_clip = clip;
    if (context->viddec.decoder_tid){
        return ret;
    }
    context->viddec.decode_state = 1;
    if ((ret = decoder_start(&context->viddec, video_thread, context, this)) < 0) {
        VIKTOR_LOGE("AVMEDIA_TYPE_VIDEO decoder_start fail");
        return ret;
    }
    return ret;
}


int ViktorVideoDecode::video_thread(void *arg,void *context){
    auto *viktor_context = static_cast<ViktorContext *>(arg);
    auto *vikt_video_decode = static_cast<ViktorVideoDecode *>(context);
    auto *video_clip = vikt_video_decode->current_clip;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;

    AVStream *videoStream = video_clip->in_fmt_ctx->streams[video_clip->m_video_index];
    AVRational tb = videoStream->time_base;
    AVRational frame_rate = av_guess_frame_rate(video_clip->in_fmt_ctx, videoStream, NULL);

    if (!frame) return AVERROR(ENOMEM);

    for (;;) {
        /**
         * 这里的video_clip和vikt_video_decode->current_clip可能不是同一个值
         * 当video_thread一运行video_clip就已经在当前方法中赋值了，但是上面的decode_start方法的如下逻辑：
         *  if (context->viddec.decoder_tid){
         *      return ret;
         *  }
         *  如果线程已经启动不再走video_thread（解码第二个片段时），也就导致video_clip不会再次赋值，所以在下面的if内赋值
         */
        if (video_clip != vikt_video_decode->current_clip){
            VIKTOR_LOGE("video_thread video_clip != current_clip");
            video_clip = vikt_video_decode->current_clip;
            videoStream = video_clip->in_fmt_ctx->streams[video_clip->m_video_index];
            tb = videoStream->time_base;
            frame_rate = av_guess_frame_rate(video_clip->in_fmt_ctx, videoStream, NULL);
        }


        ret = vikt_video_decode->get_video_frame(viktor_context, frame,video_clip);
        if (ret < 0) {
            VIKTOR_LOGE("video_thread for ret < 0");
            goto the_end;
        }
        if (!ret) {
            VIKTOR_LOGE("video_thread for continue");
            continue;
        }


        //中间省略滤镜
        VIKTOR_LOGI("video_thread queue_picture 00");
        duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational) {frame_rate.den, frame_rate.num}) : 0);

        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        VIKTOR_LOGI("video_thread duration：%lf,pts:%lf",duration,pts);
        ret = vikt_video_decode->queue_picture(viktor_context, frame, pts, duration, frame->pkt_pos, viktor_context->viddec.pkt_serial,video_clip->m_orientation);
        av_frame_unref(frame);
        VIKTOR_LOGI("video_thread queue_picture 11");
        if (ret < 0) {
            VIKTOR_LOGI("video_thread for if (ret < 0)");
            goto the_end;
        }
    }


    the_end:
    av_frame_free(&frame);
    return 0;
}

int ViktorVideoDecode::get_video_frame(ViktorContext *context, AVFrame *frame,CClip *clip){
    int got_picture;
    if ((got_picture = decoder_decode_frame(&context->viddec, frame, clip)) < 0) return -1;
    VIKTOR_LOGE("got_picture:%d", got_picture);
    //针对丢帧的处理
    if (got_picture) {
        double dpts = NAN;
        AVStream *videoStream = clip->in_fmt_ctx->streams[clip->m_video_index];
        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(videoStream->time_base) * frame->pts;
        }

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(clip->in_fmt_ctx, videoStream, frame);
        /**控制是否丢帧
        为1，则始终判断是否丢帧；
        为0，则始终不丢帧；
        为-1（默认值），则在主时钟不是video的时候，判断是否丢帧。*/
        if (frame_drop > 0 || (frame_drop && get_master_sync_type(context) != AV_SYNC_VIDEO_MASTER)) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double getClock = get_master_clock(context);
                double diff = dpts - getClock;
                VIKTOR_LOGE("get_video_frame diff:%lf,dpts:%lf,getClock:%lf", diff, dpts, getClock);
                VIKTOR_LOGE("get_video_frame viddec.pkt_serial:%d,vidclk.serial:%d", context->viddec.pkt_serial, context->vidclk.serial);


                /**
                 * 加入context->vidclk.serial == -1判断原因：
                 * 某些视频从2秒开始播放，会送入AvPacket.pts=0到解码器，
                 * 这时vidclk.serial=-1（还是初始化init_clock时设置的-1，而不是update_video_pts更新后的值）
                 * 会将第一帧数据播放出来，但是这一帧不是2秒的画面
                 */
                bool isSameSerial = (context->viddec.pkt_serial == context->vidclk.serial || context->vidclk.serial == -1);
                /**
                 * diff - is->frame_last_filter_delay < 0
                 * frame_last_filter_delay与滤镜有关，可以先忽略
                 * 即：dpts < get_master_clock(is)丢帧
                 */
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                        diff < 0 && isSameSerial &&
                        context->video_packet_q.nb_packets) {
                    VIKTOR_LOGE("got_picture AV_NOSYNC_THRESHOLD");
                    context->continuous_frame_drops++;
                    //优化过度丢帧给人卡死的现象
                    if (context->continuous_frame_drops > default_frame_drop) {
                        context->continuous_frame_drops = 0;
                    } else {

                    }

                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    VIKTOR_LOGE("got_picture after:%d", got_picture);
    return got_picture;
}
int ViktorVideoDecode::queue_picture(ViktorContext *context, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial,int orientation){
    VKFrame *vp;
    if (!(vp = frame_queue_peek_writable(&context->picture_frame_q))) return -1;

    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;

    if (!vp->bmp ||
        vp->width != src_frame->width ||
        vp->height != src_frame->height ||
        vp->format != src_frame->format){

        vp->width = src_frame->width;
        vp->height = src_frame->height;
        vp->format = src_frame->format;

        alloc_picture(vp,src_frame->format);
    }

    if (vp->bmp){
        vp->bmp->mutex->lock();
        if (func_fill_frame(vp->bmp,src_frame) < 0){
            VIKTOR_LOGI("Cannot initialize the conversion context\\n");
        }
        vp->bmp->mutex->unlock();
        vp->bmp->serial = serial;
        vp->bmp->fixWidth = context->fixWidth;
        vp->bmp->fixHeight = context->fixHeight;

        vp->pts = pts;
        vp->duration = duration;
        vp->pos = pos;
        vp->serial = serial;
        vp->sar = src_frame->sample_aspect_ratio;
        vp->bmp->sar_num = vp->sar.num;
        vp->bmp->sar_den = vp->sar.den;
        vp->bmp->orientation = orientation;

        av_frame_move_ref(vp->frame, src_frame);
        frame_queue_push(&context->picture_frame_q);
    }

    return 0;
}

void ViktorVideoDecode::alloc_picture(VKFrame *frame, int format){
    free_picture(frame->bmp);
    frame->bmp = create_overlay(frame->width,frame->height,format,overlay_format);

}