//
// Created by rainmeterLotus on 2021/12/24.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_FRAME_QUEUE_H
#define VIKTOR_FFMPEG_VIKTOR_FRAME_QUEUE_H
#include "../../../all/ff_define.h"
#include "../../../all/vk_all.h"
#include "viktor_packet_queue.h"

extern "C"{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

typedef struct VKFrame{
    AVFrame *frame;
    int serial;
    /** presentation timestamp for the frame 帧的表示时间戳，即相对总时长的偏移位，或解释为展示给用户看到的时间*/
    double pts;
    /** estimated duration of the frame 帧的估计持续时间
     * 类似AVFrame.pkt_duration(duration of the corresponding packet,对应数据包的持续时间)
     * */
    double duration;
    /** byte position of the frame in the input file 帧的字节位置*/
    int16_t pos;
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;

    VideoOverlay *bmp;
}VKFrame;

typedef struct VKFrameQueue{
    VKFrame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;

    std::mutex *mutex;
    std::condition_variable *cond;
    VKPacketQueue *pktq;
}VKFrameQueue;

int frame_queue_init(VKFrameQueue *f, VKPacketQueue *pktq, int max_size, int keep_last);
void frame_queue_destroy(VKFrameQueue *f);
void frame_queue_unref_item(VKFrame *vp);
int frame_queue_nb_remaining(VKFrameQueue *f);
VKFrame *frame_queue_peek_writable(VKFrameQueue *f);
VKFrame *frame_queue_peek_readable(VKFrameQueue *f);
void frame_queue_push(VKFrameQueue *f);
void frame_queue_next(VKFrameQueue *f);
VKFrame *frame_queue_peek(VKFrameQueue *f);
VKFrame *frame_queue_peek_next(VKFrameQueue *f);
VKFrame *frame_queue_peek_last(VKFrameQueue *f);
void frame_queue_signal(VKFrameQueue *f);

#endif //VIKTOR_FFMPEG_VIKTOR_FRAME_QUEUE_H
