//
// Created by rainmeterLotus on 2021/6/22.
//


#include "ff_pkt_queue.h"
#include "../timeline//util/ViktorLog.h"
AVPacket flush_pkt;
int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    if (!(q->mutex = SDL_CreateMutex())) {
        VIKTOR_LOGI("packet_queue_init SDL_CreateMutex fail");
        return AVERROR(ENOMEM);
    }
    if (!(q->mutex_wait = SDL_CreateMutex())) {
        VIKTOR_LOGI("packet_queue_init SDL_CreateMutex fail");
        return AVERROR(ENOMEM);
    }

    if (!(q->mutex_get = SDL_CreateMutex())) {
        VIKTOR_LOGI("packet_queue_init SDL_CreateMutex fail");
        return AVERROR(ENOMEM);
    }

    if (!(q->cond = SDL_CreateCond())) {
        VIKTOR_LOGI("packet_queue_init SDL_CreateCond fail");
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

void packet_queue_abort(PacketQueue *q){
    VIKTOR_LOGI("packet_queue_abort");
    std::lock_guard<std::mutex> lock(*q->mutex);
    q->abort_request = 1;
    //packet_queue_get 方法会wait
    q->cond->notify_one();
}

void packet_queue_start(PacketQueue *q) {
    std::lock_guard<std::mutex> lock(*q->mutex);
    q->abort_request = 0;
    /**
     这里放入了一个flush_pkt
     是一个特殊的packet，主要用来作为非连续的两端数据的“分界”标记
     */
    packet_queue_put_private(q, &flush_pkt);
}

void packet_queue_destroy(PacketQueue *q) {
    VIKTOR_LOGI("packet_queue_destroy");
    packet_queue_flush(q);
    delete q->mutex;
    q->mutex = nullptr;

    delete q->mutex_wait;
    q->mutex_wait = nullptr;

    delete q->mutex_get;
    q->mutex_get = nullptr;

    delete q->cond;
    q->cond = nullptr;
}

/** 用于将队列中的所有节点清除 比如用于销毁队列、seek操作等*/
void packet_queue_flush(PacketQueue *q) {
    VIKTOR_LOGI("packet_queue_flush");
    MyAVPacketList *pkt, *pkt1;
    std::lock_guard<std::mutex> lock(*q->mutex);

    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }

    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    VIKTOR_LOGE("packet_queue_put");
    int ret;
    {
        std::lock_guard<std::mutex> lock(*q->mutex);
        ret = packet_queue_put_private(q, pkt);
    }

    if (pkt != &flush_pkt && ret < 0) {
        av_packet_unref(pkt);//放入失败，释放AVPacket
    }
    return ret;
}

/**
放入“空包”。放入空包意味着流的结束，一般在视频读取完成的时候放入空包
 */
int packet_queue_put_nullpacket(PacketQueue *q, int stream_index) {
    VIKTOR_LOGE("packet_queue_put_nullpacket");
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}


/**
PacketQueue
------------------------------------------------------------
|     first_pkt(1)                      last_pkt(3)        |
|       1                2                  3              |
|  ------------     ------------       ------------        |
|  | myPack   |     |  myPack   |      |  myPack  |        |
|  |  pkt     |     |   pkt     |      |  pkt     |        |
|  |  next<---|<----|   next<---|<-----|  next    |        |
|  ------------     ------------       ------------        |
------------------------------------------------------------
*/
int packet_queue_put_private(PacketQueue *q, AVPacket *pkt) {
    VIKTOR_LOGE("packet_queue_put_private");
    MyAVPacketList *pkt1;
    //如果已中止，则放入失败
    if (q->abort_request) return -1;

    pkt1 = static_cast<MyAVPacketList *>(av_malloc(sizeof(MyAVPacketList)));
    //内存不足，则放入失败
    if (!pkt1) return -1;

    pkt1->pkt = *pkt;//拷贝AVPacket(浅拷贝，AVPacket.data等内存并没有拷贝)
    pkt1->next = NULL;
    if (pkt == &flush_pkt) {//如果放入的是flush_pkt，需要增加 队列的序列号，以区分不连续的两段数据
        q->serial++;
    }
    pkt1->serial = q->serial;//用队列的序列号 标记节点

    /**
     * 队列操作：如果last_pkt为空，说明队列是空的，新增节点为队头；
     * 否则，队列有数据，则让原队尾的next为新增节点。
     * 最后将队尾指向新增节点
     * */
    if (!q->last_pkt) {
        q->first_pkt = pkt1;
    } else {
        q->last_pkt->next = pkt1;
    }
    q->last_pkt = pkt1;

    //队列属性操作：增加节点数、cache大小、cache总时长
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    q->duration += pkt1->pkt.duration;
    q->cond->notify_one();
    VIKTOR_LOGE("packet_queue_put_private cond->notify_one");
    return 0;
}

/**
return < 0 if aborted, 0 if no packet and > 0 if packet.
block: 调用者是否需要在没节点可取的情况下阻塞等待
AVPacket: 输出参数，即MyAVPacketList.pkt
serial: 输出参数，即MyAVPacketList.serial
 */
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    MyAVPacketList *pkt1;
    int ret;
    VIKTOR_LOGE("packet_queue_get");
    std::lock_guard<std::mutex> lock(*q->mutex_get);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;//从队头拿数据
        if (pkt1) {
            VIKTOR_LOGE("packet_queue_get if (pkt1)");
            q->first_pkt = pkt1->next;
            if (!q->first_pkt) {//说明队列中只有一条数据
                q->last_pkt = NULL;
            }
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            q->duration -= pkt1->pkt.duration;

            /**
             * 这里才是真正的取出数据并赋值给pkt返回
             */
            *pkt = pkt1->pkt;
            if (serial) {//如果需要输出serial，把serial输出
                *serial = pkt1->serial;
            }
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            VIKTOR_LOGE("packet_queue_get SDL_CondWait");
            SDL_CondWait(q->cond, q->mutex_wait);
        }
    }
    return ret;
}