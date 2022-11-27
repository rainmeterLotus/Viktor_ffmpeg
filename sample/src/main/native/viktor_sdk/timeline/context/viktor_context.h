//
// Created by rainmeterLotus on 2021/12/18.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_CONTEXT_H
#define VIKTOR_FFMPEG_VIKTOR_CONTEXT_H
#include <mutex>
#include <thread>
#include <jni.h>
#include "viktor_frame_queue.h"
#include "../clang/CVideoTrack.h"
#include "../clang/CAudioTrack.h"
#include "../all/vk_all.h"
#include "../../ffplay/ff_opensles.h"

extern "C" {
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

/**
 * 控制是否丢帧的开关变量
 */
static int frame_drop = -1;
static int soundtouch_enable = 1;
static float default_rate = 1.0f;

typedef struct ViktorAudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} ViktorAudioParams;

typedef struct ViktorDecoder {
    AVPacket pkt;
    VKPacketQueue *queue;
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
} ViktorDecoder;

typedef struct ViktorClock{
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double pts_2;
    double pts_drift_2;
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} ViktorClock;

typedef struct ViktorContext{
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int read_pause_return;

    int av_sync_type;

    int seek_flags;
    int seek_req;
    int64_t seek_pos;
    int64_t seek_rel;

    double frame_timer;
    int step;

    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity

    VKFrameQueue picture_frame_q;
    VKFrameQueue sample_frame_q;

    VKPacketQueue video_packet_q;
    VKPacketQueue audio_packet_q;

    ViktorClock audclk;
    ViktorClock vidclk;
    ViktorClock extclk;

    ViktorDecoder auddec;
    ViktorDecoder viddec;

    int continuous_frame_drops;

    enum ShowMode show_mode;

    /**
    * 规定的宽高，播放解码规定好的宽高，不是视频原始宽高
    */
    int fixWidth;
    int fixHeight;

    std::vector<CVideoTrack *> *vec_video_track = nullptr;
    std::vector<CAudioTrack *> *vec_audio_track = nullptr;

    /**
     *demux线程读取AVPacket，放入列表,满了之后需要等待,
     * 等待解码线程将AVPacket列表读取完了之后通知demux线程继续
     * 见：decoder_decode_frame
     * d->empty_queue_cond->notify_one()
     */
    std::condition_variable *read_frame_cond;

    double audio_clock;
    int audio_clock_serial;
    uint8_t *audio_buf;/* 从要输出的AVFrame中取出的音频数据（PCM），如果有必要，则对该数据重采样*/
    uint8_t *audio_buf1;
    short *audio_new_buf;  /* for soundtouch buf */
    unsigned int audio_buf_size; /* in bytes audio_buf的总大小*/
    unsigned int audio_buf1_size;
    unsigned int audio_new_buf_size;
    int audio_buf_index; /* in bytes  下一次可读的audio_buf的index位置*/
    int audio_write_buf_size; /* audio_buf已经输出的大小，即audio_buf_size - audio_buf_index */
    int audio_volume;
    int muted;
    float       pf_playback_rate = 1.0f;

    struct ViktorAudioParams audio_src;
    struct ViktorAudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    void *soundtouch = nullptr;

    SLAudio_ES *m_audioEs = nullptr;

    /**
     * 准备解码第二个片段的音频时，m_audioEs->close_audio有可能会卡死，
     * 用改字段做标记，close_audio是，在等待的地方做判断使用
     */
    int is_audio_close = 0;


    JavaVM *m_javaVM = nullptr;
    jobject m_javaObj = nullptr;
    SDL_ProgressCallback m_progressCallback;
    SDL_PrepareCallback m_prepareCallback;


    /**
     * 1，代表正在解码
     * 0，代表解码结束
     * 主要用于demux线程读取AVPacket到了第二个Clip，准备解码该AVPacket数据时，
     * 第一个Clip片段还在解码过程中,此时是需要在解码线程中等待，直到第一个Clip的数据结束
     * 详细见下面2个方法解读：
     * ViktorVideoDecode::decode_start---->
     * 该方法中第二个Clip需要解码时，需要wait：
     * context->viddec.wait_decode_cond->wait_for
     *
     * IViktorDecode::decoder_decode_frame---->
     * 该方法中正在解码第一个Clip,当结束之后会调用：
     * d->wait_decode_cond->notify_one()通知等待的地方结束等待
     */
    int decode_state;
    std::condition_variable *wait_decode_cond;

} ViktorContext;


void release_track(ViktorContext *context);

int stream_has_enough_packets(AVStream *st, int stream_id, VKPacketQueue *queue);


void init_clock(ViktorClock *c,int *queue_serial);
void set_clock(ViktorClock *c,double pts,int serial);
void set_clock_at(ViktorClock *c,double pts,int serial,double time);
double get_clock(ViktorClock *c);
double get_progress(ViktorClock *c);
void update_master_pts(ViktorClock *c,double pts);
void update_video_pts(ViktorContext *context, double pts, int serial);
void update_audio_pts(ViktorContext *context, double pts, double time, int serial);
void sync_clock_to_slave(ViktorClock *c, ViktorClock *slave);

void decoder_init(ViktorDecoder *d, AVCodecContext *avctx, VKPacketQueue *queue, std::condition_variable *empty_queue_cond);
int decoder_start(ViktorDecoder *d, int (*fn)(void *,void *), void* arg, void *context);
void decoder_abort(ViktorDecoder *d, VKFrameQueue *fq);
void decoder_destroy(ViktorDecoder *d);

double get_master_clock(ViktorContext *context);
int get_master_sync_type(ViktorContext *context);


double vp_duration(ViktorContext *context, VKFrame *vp, VKFrame *nextvp);
double compute_target_delay(double delay, ViktorContext *context);

void stream_toggle_pause(ViktorContext *context);
void step_to_next_frame(ViktorContext *context);
#endif //VIKTOR_FFMPEG_VIKTOR_CONTEXT_H
