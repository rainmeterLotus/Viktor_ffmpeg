//
// Created by rainmeterLotus on 2021/6/22.
//

#ifndef VIKTOR_FFMPEG_FF_FRAME_QUEUE_H
#define VIKTOR_FFMPEG_FF_FRAME_QUEUE_H
#include "ff_pkt_queue.h"
#include "../../all/ff_define.h"
#include "../../all/vk_all.h"
/**
 * AVFrame:yuv420p
 * data:
 * [0]:"\U00000011\U00000011....\U00000010\U00000010\xc1\xc1\...\xca\xca\xca\U00000011\...\U00000010\U00000010"
 * [1]:"\U0000007f\U0000007f\"
 * [2]:"\U0000007f\U0000007f\"
 * [3]:NULL
 * [4]:NULL
 * [5]:NULL
 * [6]:NULL
 * [7]:NULL
 * linesize:
 * [0]:832
 * [1]:416
 * [2]:416
 * [3]:0
 * [4]:0
 * [5]:0
 * [6]:0
 * [7]:0
 * width:816
 * height:1080
 */
extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}


typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    /** presentation timestamp for the frame 帧的表示时间戳，即相对总时长的偏移位，或解释为展示给用户看到的时间*/
    double pts;
    /** estimated duration of the frame 帧的估计持续时间
     * 类似AVFrame.pkt_duration(duration of the corresponding packet,对应数据包的持续时间)
     * */
    double duration;
    int64_t pos;          /** byte position of the frame in the input file 帧的字节位置*/
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;

    VideoOverlay *bmp;
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;

    std::mutex *mutex;
    std::condition_variable *cond;
    PacketQueue *pktq;
} FrameQueue;


int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);
void frame_queue_destory(FrameQueue *f);
void frame_queue_unref_item(Frame *vp);
int frame_queue_nb_remaining(FrameQueue *f);
Frame *frame_queue_peek_writable(FrameQueue *f);
Frame *frame_queue_peek_readable(FrameQueue *f);
void frame_queue_push(FrameQueue *f);
void frame_queue_next(FrameQueue *f);
Frame *frame_queue_peek(FrameQueue *f);
Frame *frame_queue_peek_next(FrameQueue *f);
Frame *frame_queue_peek_last(FrameQueue *f);
void frame_queue_signal(FrameQueue *f);
void sws_scale_frame(AVFrame *avFrame);
int frameDisplayWidth(const AVFrame *frame);
int frameDisplayHeight(const AVFrame *frame);
#endif //VIKTOR_FFMPEG_FF_FRAME_QUEUE_H
