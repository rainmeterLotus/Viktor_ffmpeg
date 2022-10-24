//
// Created by rainmeterLotus on 2021/12/24.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_PACKET_QUEUE_H
#define VIKTOR_FFMPEG_VIKTOR_PACKET_QUEUE_H
#include "viktor_sdl.h"
#include "../util/ViktorLog.h"

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/mem.h>
}

extern AVPacket flush_pkt;

typedef struct VKPacketList {
    AVPacket pkt;
    struct VKPacketList *next;
    int serial;
}VKPacketList;

typedef struct VKPacketQueue {
    VKPacketList *first_pkt,*last_pkt;
    int nb_packets;//队列中一共有多少个节点
    int size;//队列所有节点字节总数，用于计算cache大小
    int64_t duration;//队列所有节点的合计时长
    int abort_request;//是否要中止队列操作，用于安全快速退出播放
    int serial;//序列号，和VKPacketList的serial作用相同，但改变的时序稍微有点不同

    std::mutex *mutex;
    std::mutex *mutex_wait;
    std::mutex *mutex_get;
    std::condition_variable *cond;
}VKPacketQueue;

int packet_queue_init(VKPacketQueue *q);
void packet_queue_destroy(VKPacketQueue *q);
void packet_queue_flush(VKPacketQueue *q);
int packet_queue_put(VKPacketQueue *q, AVPacket *pkt);
int packet_queue_put_nullpacket(VKPacketQueue *q, int stream_index);
int packet_queue_put_private(VKPacketQueue *q, AVPacket *pkt);
int packet_queue_get(VKPacketQueue *q, AVPacket *pkt,AVPacket *d_pkt, int block, int *serial,int decode_state);
void packet_queue_start(VKPacketQueue *q);
void packet_queue_abort(VKPacketQueue *q);
#endif //VIKTOR_FFMPEG_VIKTOR_PACKET_QUEUE_H
