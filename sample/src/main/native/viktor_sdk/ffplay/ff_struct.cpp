//
// Created by rainmeterLotus on 2021/6/16.
//



#include "ff_struct.h"


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
int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    if (!st) {
        VIKTOR_LOGE("stream_has_enough_packets:%d", stream_id);
        return 1;
    }
    bool streamRet = stream_id < 0;
    bool abortRet = queue->abort_request;
    bool disRet = (st->disposition & AV_DISPOSITION_ATTACHED_PIC);
    bool nbRet = queue->nb_packets > MIN_FRAMES;
    bool durationRet = !queue->duration;
    bool durationPlus = av_q2d(st->time_base) * queue->duration > 1.0;
//    VIKTOR_LOGE("streamRet:%d,abortRet:%d,disRet:%d,nbRet:%d,durationRet:%d,durationPlus:%d",
//         streamRet,abortRet,disRet,nbRet,durationRet,durationPlus);

//    return stream_id < 0 ||
//           queue->abort_request ||
//           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
//           queue->nb_packets > MIN_FRAMES &&
//           (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);

    return streamRet || abortRet || disRet || nbRet && (durationRet || durationPlus);
}

void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue,
                  std::condition_variable *empty_queue_cond) {
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
}

int decoder_start(Decoder *d, int (*fn)(void *, void *), void *arg, void *context) {
    packet_queue_start(d->queue);
    d->decoder_tid = SDL_CreateReadThread(fn, arg, context);
    if (!d->decoder_tid) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

void decoder_abort(Decoder *d, FrameQueue *fq) {
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    SDL_WaitThread(d->decoder_tid);
    delete d->decoder_tid;
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
    /**
     * /system/lib64/libhwui.so (SkSurface_Base::~SkSurface_Base()+72)
     * 下面代码打开，会报错
     delete d->queue;
    d->queue = NULL;
     */

}

void decoder_destroy(Decoder *d) {
    av_packet_unref(&d->pkt);
    avcodec_free_context(&d->avctx);
}

void init_clock(Clock *c, int *queue_serial) {
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

//private
void set_clock_at(Clock *c, double pts, int serial, double time) {
    VIKTOR_LOGE("ffplay","set_clock_at pts:%lf,time:%lf,serial:%d", pts,time,serial);
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

void set_clock(Clock *c, double pts, int serial) {
    double time = av_gettime_relative() / 1000000.0;
    VIKTOR_LOGE("ffplay","set_clock pts:%lf,serial:%d", pts,serial);
    set_clock_at(c, pts, serial, time);
}

double get_clock(Clock *c) {
    if (*c->queue_serial != c->serial) {
        return NAN;
    }
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        double result = c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
        VIKTOR_LOGE("ffplay","get_clock c->pts_drift:%lf,time:%lf,c->pts:%lf,result:%lf", c->pts_drift,
                    time, c->pts,result);
        return result;
    }
}

void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
    /* update current video pts */
    VIKTOR_LOGE("ffplay","update_video_pts");
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(&is->vidclk, pts, serial, time);
    sync_clock_to_slave(&is->extclk, &is->vidclk);
}

void update_audio_pts(VideoState *is, double pts, double time,int serial){
    set_clock_at(&is->audclk, pts, serial, time);
    sync_clock_to_slave(&is->extclk, &is->audclk);
}

//private
void sync_clock_to_slave(Clock *c, Clock *slave) {
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    VIKTOR_LOGE("ffplay","sync_clock_to_slave clock:%lf,slave_clock:%lf", clock,slave_clock);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->serial);
}

int is_realtime(AVFormatContext *s) {
    if (!strcmp(s->iformat->name, "rtp")
        || !strcmp(s->iformat->name, "rtsp")
        || !strcmp(s->iformat->name, "sdp")) {
        return 1;
    }

    if (s->pb && (!strncmp(s->url, "rtp:", 4) || !strncmp(s->url, "udp:", 4))) {
        return 1;
    }
    return 0;
}



int get_master_sync_type(VideoState *is) {
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

double get_master_clock(VideoState *is) {
    double val;
    switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->vidclk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->audclk);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}

void set_clock_speed(Clock *c, double speed) {
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}

void change_external_clock_speed(VideoState *is) {
    if (is->pf_playback_rate != 1.0f) {
        double value = is->pf_playback_rate + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - is->pf_playback_rate) / fabs(1.0 - is->pf_playback_rate);
        set_clock_speed(&is->extclk, value);
    }
}

double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

/**
 * 经验值delay一般是0.04xx
 * 所以sync_threshold一般也是0.04xx
 * 大部分情况下delay值为
 * delay = FFMAX(0, delay + diff)
 * 或者
 * delay = 2 * delay
 * 更多情况是：delay = 2 * delay
 * @param delay
 * @param is
 * @return
 *              -sync_threshold    0        sync_threshold      AV_SYNC_FRAMEDUP_THRESHOLD
 *                      |          |            |                   |
 * --------------------------------------------------------------------------------------------> diff
 * FFMAX(0,delay+diff)  |       delay           |  2 * delay        |  delay + diff
 */
double compute_target_delay(double delay, VideoState *is) {
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&is->vidclk) - get_master_clock(is);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        VIKTOR_LOGE("compute_target_delay delay:%lf,diff:%lf,sync_threshold:%lf", delay, diff, sync_threshold);
        if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
            if (diff <= -sync_threshold) {
                VIKTOR_LOGE("compute_target_delay FFMAX");
                /**
                * 视频播放过慢，需要适当丢帧
                */
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
                VIKTOR_LOGE("compute_target_delay delay + diff");
                delay = delay + diff;
            } else if (diff >= sync_threshold) {
                VIKTOR_LOGE("compute_target_delay 2 * delay");
                delay = 2 * delay;
            }
        }
    }

    VIKTOR_LOGE("compute_target_delay finally delay:%lf", delay);
    return delay;
}

void set_default_window_size(int pic_width, int pic_height, AVRational sar) {
    int scr_width = INT_MAX;
    int scr_height = INT_MAX;
    if (scr_width == INT_MAX && scr_height == INT_MAX) {
        scr_height = pic_height;
    }

    //calculate_display_rect(&rect, 0, 0, max_width, max_height, width, height, sar);

    AVRational aspect_ratio = sar;
    int64_t width, height, x, y;

    if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
        aspect_ratio = av_make_q(1, 1);

    aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));
    height = scr_height;
    width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;

    if (width > scr_width) {
        width = scr_width;
        height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
    }

    int finalW = FFMAX((int) width, 1);
    int finalH = FFMAX((int) height, 1);

    VIKTOR_LOGE("ffplay", "finalW:%d,finalH:%d", finalW, finalH);
}

void stream_toggle_pause(VideoState *is) {
    VIKTOR_LOGE("ffplay", "stream_toggle_pause start paused:%d",is->paused);
    if (is->paused) {
        is->frame_timer = av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }

    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;

}

void step_to_next_frame(VideoState *is) {
    if (is->paused) {
        stream_toggle_pause(is);
    }
    is->step = 1;
}

int isImage(AVStream *videoSt) {
    if (!videoSt) {
        return 0;
    }
    double fps = av_q2d(videoSt->avg_frame_rate);
    if ((videoSt->avg_frame_rate.num == 0 ||
         videoSt->avg_frame_rate.den == 0) &&
        (isnan(fps) || fps < 1.0)) {
        VIKTOR_LOGE("ffplay", "isImage");
        return 1;
    }

    return 0;
}

int draw_video_orientation(AVStream *videoSt) {
    if (!videoSt) {
        return 0;
    }
    /* AVDictionaryEntry *rotate_tag = av_dict_get(videoSt->metadata, "rotate", NULL, 0);
     uint8_t* displaymatrix = av_stream_get_side_data(videoSt,
                                                      AV_PKT_DATA_DISPLAYMATRIX, NULL);
     double theta = 0;
     if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
         char *tail;
         theta = av_strtod(rotate_tag->value, &tail);
         if (*tail)
             theta = 0;
     }
     if (displaymatrix && !theta)
         theta = -av_display_rotation_get((int32_t*) displaymatrix);

     theta -= 360*floor(theta/360 + 0.9/360);

     if (fabs(theta - 90*round(theta/90)) > 2){
         av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
                                      "If you want to help, upload a sample "
                                      "of this file to ftp://upload.ffmpeg.org/incoming/ "
                                      "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
     }

     VIKTOR_LOGE("ffplay", "get_orientation theta:%lf", theta);
 */
    AVDictionaryEntry *rotate_dict = NULL;
    rotate_dict = av_dict_get(videoSt->metadata, "rotate", rotate_dict, 0);
    VIKTOR_LOGE("ffplay", "get_orientation is video rotate_dict:%p", rotate_dict);
    if (!rotate_dict) {
        return 0;
    }

    int angle = atoi(rotate_dict->value);
    VIKTOR_LOGE("ffplay", "get_orientation is video angle:%d", angle);
    return angle;
}

int draw_image_orientation(char *filename, int *mirrorImage) {
    int theta = 0;
    ExifData *exifData = exif_data_new_from_file(filename);
    if (exifData) {
        ExifByteOrder byteOrder = exif_data_get_byte_order(exifData);
        ExifEntry *exifEntry = exif_data_get_entry(exifData, EXIF_TAG_ORIENTATION);
        if (exifEntry)
            theta = exif_get_short(exifEntry->data, byteOrder);
        exif_data_free(exifData);
    }

    int rote = 0;
    if (theta == 1) {
        rote = 0;
    } else if (theta == 2) {
        rote = 0;
        *mirrorImage = 1;
    } else if (theta == 3) {
        rote = 180;
    } else if (theta == 4) {
        rote = 180;
        *mirrorImage = 1;
    } else if (theta == 5) {
        rote = 90;
        *mirrorImage = 1;
    } else if (theta == 6) {
        rote = 90;
    } else if (theta == 7) {
        rote = 270;
        *mirrorImage = 1;
    } else if (theta == 8) {
        rote = 270;
    }

    VIKTOR_LOGE("ffplay", "get_orientation is image theta:%d,rote:%d,mirrorImage:%d", theta, rote, *mirrorImage);
    return rote;
}