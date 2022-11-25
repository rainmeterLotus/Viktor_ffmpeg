//
// Created by rainmeterLotus on 2021/12/24.
//

#include "viktor_packet_queue.h"


int packet_queue_init(VKPacketQueue *q){
    memset(q,0,sizeof(VKPacketQueue));
    if (!(q->mutex = sdl_create_mutex())){
        return AVERROR(ENOMEM);
    }
    if (!(q->mutex_wait = sdl_create_mutex())) {
        return AVERROR(ENOMEM);
    }

    if (!(q->mutex_get = sdl_create_mutex())) {
        return AVERROR(ENOMEM);
    }
    if (!(q->cond = sdl_create_cond())){
        return AVERROR(ENOMEM);
    }

    q->abort_request = 1;
    return 0;
}

void packet_queue_destroy(VKPacketQueue *q){
    if (!q) return;
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

void packet_queue_flush(VKPacketQueue *q){
    if (!q){
        VIKTOR_LOGD("packet_queue_flush q is null");
        return ;
    }
    VKPacketList *pkt,*pkt1;
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


int packet_queue_put(VKPacketQueue *q, AVPacket *pkt){
    VIKTOR_LOGD("packet_queue_put");
    int ret;
    {
        std::lock_guard<std::mutex> lock(*q->mutex);
        ret = packet_queue_put_private(q,pkt);
    }

    if (pkt != &flush_pkt && ret < 0){
        //放入失败，释放AVPacket
        av_packet_unref(pkt);
    }

    return ret;
}


int packet_queue_put_nullpacket(VKPacketQueue *q, int stream_index){
    if (!q){
        VIKTOR_LOGD("packet_queue_put_nullpacket q is null");
        return -1;
    }
    VIKTOR_LOGD("packet_queue_put_nullpacket");
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
int packet_queue_put_private(VKPacketQueue *q, AVPacket *pkt){
    if (!q){
        VIKTOR_LOGD("packet_queue_put_private q is null");
        return -1;
    }
    VIKTOR_LOGD("packet_queue_put_private");
    VKPacketList *pkt1;
    if (q->abort_request) return -1;
    pkt1 = static_cast<VKPacketList *>(av_malloc(sizeof(VKPacketList)));
    //内存不足，则放入失败
    if (!pkt1) return -1;
    pkt1->pkt = *pkt;//拷贝AVPacket(浅拷贝，AVPacket.data等内存并没有拷贝)
    pkt1->next = nullptr;
    if (pkt == &flush_pkt) {//如果放入的是flush_pkt，需要增加 队列的序列号，以区分不连续的两段数据
        q->serial++;
    }
    pkt1->serial = q->serial;//用队列的序列号 标记节点

    /**
    * 队列操作：如果last_pkt为空，说明队列是空的，新增节点为队头；
    * 否则，队列有数据，则让原队尾的next指向新增节点。
    * 最后将队尾指向新增节点
    * */
    if (q->last_pkt){
        q->last_pkt->next = pkt1;
    } else {
        q->first_pkt = pkt1;
    }
    q->last_pkt = pkt1;
    //队列属性操作：增加节点数、cache大小、cache总时长
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    q->duration += pkt1->pkt.duration;
    q->cond->notify_one();
    VIKTOR_LOGD("packet_queue_put_private cond->notify_one");
    return 0;
}

int packet_queue_get(VKPacketQueue *q, AVPacket *pkt, AVPacket *d_pkt,int block, int *serial,int decode_state){
    if (!q){
        VIKTOR_LOGD("packet_queue_get q is null");
        return -1;
    }
    VKPacketList *pkt1;
    int ret;
    VIKTOR_LOGD("packet_queue_get");
    std::lock_guard<std::mutex> lock(*q->mutex_get);
    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;//从队头拿数据
        if (pkt1) {
            VIKTOR_LOGD("packet_queue_get if (pkt1)");
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
            VIKTOR_LOGD("packet_queue_get SDL_CondWait :%p,d_pkt->size:%d",d_pkt,d_pkt->size);
            if (d_pkt->size > 0 && decode_state > 0){
                av_packet_move_ref(pkt, d_pkt);
                ret = 1;
                break;
            }
            VIKTOR_LOGD("packet_queue_get SDL_CondWait");
            sdl_cond_wait(q->cond, q->mutex_wait);
        }
    }
    return ret;
}

void packet_queue_start(VKPacketQueue *q){
    if (!q){
        VIKTOR_LOGD("packet_queue_start q is null");
        return;
    }
    std::lock_guard<std::mutex> lock(*q->mutex);
    q->abort_request = 0;
    /**
     这里放入了一个flush_pkt
     是一个特殊的packet，主要用来作为非连续的两端数据的“分界”标记
     */
    packet_queue_put_private(q, &flush_pkt);
}

void packet_queue_abort(VKPacketQueue *q){
    if (!q){
        VIKTOR_LOGD("packet_queue_abort q is null");
        return;
    }
    VIKTOR_LOGD("packet_queue_abort q:%p",q);
    std::lock_guard<std::mutex> lock(*q->mutex);
    q->abort_request = 1;
    //packet_queue_get 方法会wait
    q->cond->notify_one();
}