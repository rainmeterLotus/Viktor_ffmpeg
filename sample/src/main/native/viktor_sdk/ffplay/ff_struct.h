//
// Created by rainmeterLotus on 2021/6/16.
//

#ifndef VIKTOR_FFMPEG_FF_STRUCT_H
#define VIKTOR_FFMPEG_FF_STRUCT_H

#include <string.h>
#include "../timeline//util/ViktorLog.h"
#include "ff_frame_queue.h"
#include "ff_opensles.h"
#include "../all/vk_all.h"

extern "C"{
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libavutil/display.h>
#include <libavutil/eval.h>
#include <libexif/exif-data.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-tag.h>
void print_error(const char *filename, int err);
void print_no(int err);
}

/**
 * 总结各个结构体中的serial
 * PacketQueue.serial == MyAVPacketList.serial
 * (在 packet_queue_put_private 方法中，保证二者相等，并且二者的值的改变永远在该方法中)
 *
 * MyAVPacketList.serial
 * (作用：
 * 用于在packet_queue_get方法中给Decoder.pkt_serial赋值，只有这一个作用)
 *
 * Decoder.pkt_serial
 * (在 packet_queue_get 方法中被赋值(*serial = pkt1->serial)，来自MyAVPacketList.serial
 * 作用：
 * 1.在decoder_decode_frame方法中比较是否一个连续的流"d->queue->serial == d->pkt_serial"
 * 2.在decoder_decode_frame方法中取一个packet的do while中packet_queue_get方法中赋值，然后比较是否同一个流之后跳出do while
 * 3.在get_video_frame方法中，丢帧处理时做if条件判断使用is->viddec.pkt_serial == is->vidclk.serial
 * 4.在audio_thread中给Frame.serial赋值 af->serial = is->auddec.pkt_serial
 * 5.在vidoe_thread的queue_picture方法中给Frame.serial赋值 vp->serial = serial
 * 6.在subtitle_thread中给Frame.serial赋值 sp->serial = is->subdec.pkt_serial
 * 7.在decoder_decode_frame方法中给Decoder.finished赋值
 * Decoder.finished赋值主要也在decoder_decode_frame方法中
 * Decoder.finished主要作用在read_thread方法中判断是否播放完成，例如is->auddec.finished == is->audioq.serial
 *
 * 关于Decoder.finished在read_thread方法判断是否播放完成，可以如下追踪finished的来源
 * decoder_decode_frame-->packet_queue_get
 * (PacketQueue.serial-->MyAVPacketList.serial-->Decoder.pkt_serial-->Decoder.finished)
 * -->read_thread-->is->auddec.finished == is->audioq.serial && is->viddec.finished == is->videoq.serial
 * )
 *
 * Frame.serial
 * (来自于：
 * 视频-->video_thread解码---->queue_picture入队----->is->viddec.pkt_serial----->vp->serial = serial
 * 音频-->audio_thread解码---->frame入队列---->af->serial = is->auddec.pkt_serial'
 * 文字-->subtitle_thread解码---->frame入队列---->sp->serial = is->subdec.pkt_serial
 * 即Frame的serial来自音频，视频，文字解码时赋值，并且都是来自各自的解码Decoder.pkt_serial
 * PacketQueue.serial-->MyAVPacketList.serial-->Decoder.pkt_serial-->Frame.serial
 * 作用：
 * 1.frame_queue_last_pos 条件判断是否同流
 * 2.video_refresh 中:
 *  a.vp->serial != is->videoq.serial条件判断，标记一个节点已经被读过
 *  b.lastvp->serial != vp->serial 条件判断，更新VideoState.frame_timer
 *  c.update_video_pts 用于给视频时钟和外部时钟更新serial(见Clock.serial分析)
 *  d.vp_duration 条件判断是否同流
 *  e.字幕展示逻辑中，条件判断是否同流
 * 3.sdl_audio_callback-->audio_decode_frame-->do while中做条件判断
 * 4.sdl_audio_callback-->audio_decode_frame---->is->audio_clock_serial = af->serial给audio_clock_serial赋值
 * )
 *
 * Clock.serial
 * (在set_clock_at中赋值：set_clock-->set_clock_at
 * set_clock方法主要被调用关键点：
 * 1.read_thread中seek成功后给extclk.serial=0（即seek成功后将同步外部时钟extclk.serial清0）
 * 2.set_clock_speed-->set_clock方法改变速度时，给外部时钟extclk.serial赋值，即自己给自己赋值
 * 3.stream_toggle_pause-->set_clock方法暂停播放时，给vidclk,extclk各自的serial赋值，即自己给自己赋值
 * 4.update_video_pts-->set_clock和sync_clock_to_slave(video_refresh视频)
 *  sdl_audio_callback->set_clock和sync_clock_to_slave(sdl_audio_callback音频)
 *  sync_clock_to_slave-->set_clock即音频，视频各自的时钟同步到extclk
 * 作用：
 * 1.get_clock中用于判断Clock中的serial是否同一个流（*c->queue_serial != c->serial）
 * 2.在get_video_frame方法中，丢帧处理时做if条件判断使用is->viddec.pkt_serial == is->vidclk.serial
 * )
 *
 * Clock.queue_serial
 * (在init_clock中赋值，来自于PacketQueue.serial的地址，即指针，所以在packet_queue_put_private中给
 * PacketQueue.serial赋值，即给Clock.queue_serial赋值
 * 例外：如果是extclk，该queue_serial值即是extclk.serial的地址
 * 作用：get_clock中用于判断Clock中的serial是否同一个流（*c->queue_serial != c->serial）
 * )
 *
 * VideoState.audio_clock_serial
 * 给音频重采样完成后同步audclk中的serial中使用的(即调用set_clock_at方法)。
 * sdl_audio_callback-->audio_decode_frame
 * 1.在audio_decode_frame中使用Frame结构体中的serial给audio_clock_serial赋值(is->audio_clock_serial = af->serial);
 * 2.回到sdl_audio_callback后去同步audclk时再用audio_clock_serial给audclk.serial赋值。
 */

static int genpts = 0;
static int find_stream_info = 1;
static int audio_disable = 1;
static int video_disable = 0;
static int subtitle_disable = 1;
static int infinite_buffer = -1;
static int fast = 0;
static int lowres = 0;
static int soundtouch_enable = 1;
static float default_rate = 1.0f;

/**
 * 只播放音频时，傅里叶速度
 */
static double rdftspeed = 0.02;


static const char *audio_codec_name;
static const char *video_codec_name;

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    /**
     * 在 packet_queue_get 方法中赋值，来自
     * MyAVPacketList.serial
     */
    int pkt_serial;
    int finished;
    int packet_pending;
    std::condition_variable *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    std::thread *decoder_tid;
} Decoder;

typedef struct Clock{
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

typedef struct VideoState{
    std::thread *read_tid;
    std::thread *read_loop;
    std::thread *prepare_thread;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;

    int frame_drops_early;
    int frame_drops_late;
    double last_vis_time;
    int queue_attachments_req;  //是否请求attachments(对于mp3来说，就是封面图片)
    int seek_flags;
    int seek_req;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic;
    int realtime;

    double audio_clock;
    int audio_clock_serial;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    short *audio_new_buf;  /* for soundtouch buf */
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    unsigned int audio_new_buf_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    struct AudioParams audio_src;
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue sampq;


    AVStream *audio_st;
    PacketQueue audioq;

    Decoder auddec;
    Decoder viddec;

    int av_sync_type;

    int audio_stream;

    double frame_last_filter_delay;

    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    int eof;

    double frame_timer;
    char *filename;
    int last_video_stream, last_audio_stream;
    int step;
    /**
     *
     */
    std::condition_variable *continue_read_thread;

    enum ShowMode show_mode;

    SLAudio_ES *m_audioEs = nullptr;

    void *soundtouch = nullptr;
    float       pf_playback_rate = 1.0f;
    int         pf_playback_rate_changed;
    int continuous_frame_drops;

    int m_width = 0;
    int m_height = 0;
    //视频帧的方向
    int orientation = 0;
    //视频总时长 微秒
    long m_durationUs = 0;

} VideoState;


int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue);

void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, std::condition_variable *empty_queue_cond);
int decoder_start(Decoder *d, int (*fn)(void *,void *), void* arg, void *context);
void decoder_abort(Decoder *d, FrameQueue *fq);
void decoder_destroy(Decoder *d);

void init_clock(Clock *c,int *queue_serial);
void set_clock(Clock *c,double pts,int serial);
void set_clock_at(Clock *c,double pts,int serial,double time);
double get_clock(Clock *c);
void update_video_pts(VideoState *is, double pts, int64_t pos, int serial);
void update_audio_pts(VideoState *is, double pts, double time, int serial);
void sync_clock_to_slave(Clock *c, Clock *slave);
void set_clock_speed(Clock *c, double speed);
void change_external_clock_speed(VideoState *is);

int is_realtime(AVFormatContext *s);


double get_master_clock(VideoState *is);
int get_master_sync_type(VideoState *is);

double vp_duration(VideoState *is, Frame *vp, Frame *nextvp);
double compute_target_delay(double delay, VideoState *is);

void set_default_window_size(int width, int height, AVRational sar);

void stream_toggle_pause(VideoState *is);

void step_to_next_frame(VideoState *is);

int isImage(AVStream *videoSt);

int draw_video_orientation(AVStream *videoSt);

int draw_image_orientation(char *filename, int *mirrorImage);

#endif //VIKTOR_FFMPEG_FF_STRUCT_H
