//
// Created by rainmeterLotus on 2021/12/18.
//

#include "viktor_context.h"

/**
满足PacketQueue总时长为0，或总时长超过1s的前提下(!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0)
1.流没有打开（stream_id < 0）
2.有退出请求（queue->abort_request）
3.配置了AV_DISPOSITION_ATTACHED_PIC
（AV_DISPOSITION_ATTACHED_PIC 是一个标志。如果一个流中含有这个标志的话，那么就是说这个流是mp3文件中的一个 Video Stream 。
        并且该流只有一个AVPacket，也就是 attached_pic，这个 AVPacket 中所存储的内容就是这个mp3文件的封面图片，
        如果这个流中包含这个标志的话，说明这个流是mp3 文件中的 Video Stream 。
        不是传统意义上的视频流。它只存放了封面信息，在播放或者导出时，不需要这个数据）

4.队列内包个数大于MIN_FRAMES（=25）
 */
int stream_has_enough_packets(AVStream *st, int stream_id, VKPacketQueue *queue) {
    if (!st){
        return 1;
    }
    return stream_id < 0 ||
           queue->abort_request ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES &&
           (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

void init_clock(ViktorClock *c,int *queue_serial){
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

void set_clock(ViktorClock *c,double pts,int serial){
    double time = av_gettime_relative() / 1000000.0;
    VIKTOR_LOGD("set_clock pts:%lf,serial:%d", pts,serial);
    set_clock_at(c, pts, serial, time);
}
//private
void set_clock_at(ViktorClock *c,double pts,int serial,double time){
    VIKTOR_LOGD("set_clock_at pts:%lf,time:%lf,serial:%d", pts,time,serial);
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

double get_clock(ViktorClock *c){
    if (*c->queue_serial != c->serial) {
        return NAN;
    }
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        double result = c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
        VIKTOR_LOGD("get_clock c->pts_drift:%lf,time:%lf,c->pts:%lf,result:%lf", c->pts_drift,
             time, c->pts,result);
        return result;
    }
}

double get_progress(ViktorClock *c){
    if (*c->queue_serial != c->serial) {
        return NAN;
    }
    if (c->paused) {
        return c->pts_2;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        double result = c->pts_drift_2 + time - (time - c->last_updated) * (1.0 - c->speed);
        VIKTOR_LOGD("get_progress c->pts_drift:%lf,time:%lf,c->pts:%lf,result:%lf", c->pts_drift_2,
                    time, c->pts,result);
        return result;
    }
}

void update_master_pts(ViktorClock *c,double pts){
    VIKTOR_LOGD("update_master_pts");
    c->pts_2 = pts;
    c->pts_drift_2 = c->pts_2 - av_gettime_relative() / 1000000.0;
}

void update_video_pts(ViktorContext *context, double pts, int serial) {
    VIKTOR_LOGD("update_video_pts");
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(&context->vidclk, pts, serial, time);
    sync_clock_to_slave(&context->extclk, &context->vidclk);
}

void update_audio_pts(ViktorContext *context, double pts, double time, int serial){
    set_clock_at(&context->audclk, pts, serial, time);
    sync_clock_to_slave(&context->extclk, &context->audclk);
}
//private
void sync_clock_to_slave(ViktorClock *c, ViktorClock *slave) {
    double slave_clock = get_clock(slave);
    double clock = get_clock(c);
    VIKTOR_LOGD("sync_clock_to_slave clock:%lf,slave_clock:%lf", clock,slave_clock);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->serial);
}

void decoder_init(ViktorDecoder *d, AVCodecContext *avctx, VKPacketQueue *queue, std::condition_variable *empty_queue_cond){
    memset(d, 0, sizeof(ViktorDecoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
    if (!d->wait_decode_cond){
        d->wait_decode_cond = sdl_create_cond();
    }
}

int decoder_start(ViktorDecoder *d, int (*fn)(void *,void *), void* arg, void *context){
    packet_queue_start(d->queue);
    d->decoder_tid = new(std::nothrow) std::thread(fn, arg, context);
    if (!d->decoder_tid) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

void decoder_abort(ViktorDecoder *d, VKFrameQueue *fq){
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    sdl_wait_thread(d->decoder_tid);
    delete d->decoder_tid;
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}
void decoder_destroy(ViktorDecoder *d){
    av_packet_unref(&d->pkt);
//    avcodec_free_context(&d->avctx);
}

double get_master_clock(ViktorContext *context){
    double val;
    switch (get_master_sync_type(context)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&context->vidclk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&context->audclk);
            break;
        default:
            val = get_clock(&context->extclk);
            break;
    }
    return val;
}

int get_master_sync_type(ViktorContext *context){
    return AV_SYNC_EXTERNAL_CLOCK;
}

double vp_duration(ViktorContext *context, VKFrame *vp, VKFrame *nextvp){
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > context->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

double compute_target_delay(double delay, ViktorContext *context){
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(context) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&context->vidclk) - get_master_clock(context);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        VIKTOR_LOGD("compute_target_delay delay:%lf,diff:%lf,sync_threshold:%lf", delay, diff, sync_threshold);
        if (!isnan(diff) && fabs(diff) < context->max_frame_duration) {
            if (diff <= -sync_threshold) {
                /**
                * 视频播放过慢，需要适当丢帧
                */
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
                delay = delay + diff;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
    }

    VIKTOR_LOGD("compute_target_delay finally delay:%lf", delay);
    return delay;
}


void stream_toggle_pause(ViktorContext *context){
    VIKTOR_LOGD("stream_toggle_pause");
    if (context->paused) {
        context->frame_timer = av_gettime_relative() / 1000000.0 - context->vidclk.last_updated;
        if (context->read_pause_return != AVERROR(ENOSYS)) {
            context->vidclk.paused = 0;
        }
        set_clock(&context->vidclk, get_clock(&context->vidclk), context->vidclk.serial);
    }

    set_clock(&context->extclk, get_clock(&context->extclk), context->extclk.serial);
    update_master_pts(&context->extclk,get_progress(&context->extclk));
    context->paused = context->audclk.paused = context->vidclk.paused = context->extclk.paused = !context->paused;

}
void step_to_next_frame(ViktorContext *context){
    if (context->paused) {
        stream_toggle_pause(context);
    }
    context->step = 1;
}

void release_track(ViktorContext *context){
    if (!context) return;
    for (auto track:*context->vec_video_track) {
        delete track;
        context->vec_video_track->clear();
    }

    for (auto track:*context->vec_audio_track) {
        delete track;
        context->vec_audio_track->clear();
    }
}