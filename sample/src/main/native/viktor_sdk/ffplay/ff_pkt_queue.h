//
// Created by rainmeterLotus on 2021/6/22.
//

#ifndef VIKTOR_FFMPEG_FF_PKT_QUEUE_H
#define VIKTOR_FFMPEG_FF_PKT_QUEUE_H
#include "ff_sdl.h"

extern "C"{
#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}
extern AVPacket flush_pkt;

/**
 * 在 packet_queue_put_private 方法中，保证了
 * MyAVPacketList.serial == PacketQueue.serial
 */
typedef struct MyAVPacketList{
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;//队列中一共有多少个节点
    int size;//队列所有节点字节总数，用于计算cache大小
    int64_t duration;//队列所有节点的合计时长
    int abort_request;//是否要中止队列操作，用于安全快速退出播放
    int serial;//序列号，和MyAVPacketList的serial作用相同，但改变的时序稍微有点不同
    std::mutex *mutex;
    std::mutex *mutex_wait;
    std::mutex *mutex_get;
    std::condition_variable *cond;
} PacketQueue;


int packet_queue_init(PacketQueue *q);
void packet_queue_destroy(PacketQueue *q);
void packet_queue_flush(PacketQueue *q);
int packet_queue_put(PacketQueue *q, AVPacket *pkt);
int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
void packet_queue_start(PacketQueue *q);
void packet_queue_abort(PacketQueue *q);
#endif //VIKTOR_FFMPEG_FF_PKT_QUEUE_H
