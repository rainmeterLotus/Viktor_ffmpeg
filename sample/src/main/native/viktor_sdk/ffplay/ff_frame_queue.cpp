//
// Created by rainmeterLotus on 2021/6/22.
//

#include "ff_frame_queue.h"
#include "../timeline//util/ViktorLog.h"

int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last) {
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = SDL_CreateMutex())) {
        VIKTOR_LOGI("frame_queue_init SDL_CreateMutex fail");
        return AVERROR(ENOMEM);
    }

    if (!(f->cond = SDL_CreateCond())) {
        VIKTOR_LOGI("frame_queue_init SDL_CreateCond fail");
        return AVERROR(ENOMEM);
    }

    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (int i = 0; i < f->max_size; i++) {
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    }

    return 0;
}

void frame_queue_destory(FrameQueue *f) {
    for (int i = 0; i < f->max_size; ++i) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
        free_picture(vp->bmp);
    }
    delete f->mutex;
    f->mutex = nullptr;

    delete f->cond;
    f->cond = nullptr;

//    delete f->pktq;
//    f->pktq = nullptr;
}

void frame_queue_signal(FrameQueue *f) {
    std::lock_guard<std::mutex> lock(*f->mutex);
    f->cond->notify_one();
}

/** 释放的内存都是关联的内存(即vp->frame)，而非结构体Frame自身内存 */
void frame_queue_unref_item(Frame *vp) {
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}


int frame_queue_nb_remaining(FrameQueue *f) {
    return f->size - f->rindex_shown;
}


Frame *frame_queue_peek_readable(FrameQueue *f) {
    std::unique_lock<std::mutex> lock(*f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->pktq->abort_request) {
        f->cond->wait(lock);
    }

    if (f->pktq->abort_request) return NULL;
    //因为rindex加1后可能超过max_size，所以这里取余
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

/**
 * 存入队列
 * 对windex加1，将写指针移动到下一个元素，凡是windex“之前”的节点，都是写过的
 * 在frame_queue_peek_writable获取到可写的Frame，写入数据后，f->windex节点的frame已经写上数据了
 * 这里frame_queue_push方法只是将windex加1代表着下一次可写的节点frame已经准备好，
 * 且将size++表示已经写了多少个frame
*/
void frame_queue_push(FrameQueue *f) {
    if (++f->windex == f->max_size) {
        f->windex = 0;
    }
    f->mutex->lock();
    f->size++;
    f->cond->notify_one();
    f->mutex->unlock();
}


/**获取一个可写节点
 * 读和写不存在重叠，即windex和rindex不会重叠
 * 通过size判断当前缓冲区内空间是否够写，或者够读，
 * 比如这里先通过一个循环的条件等待，判断f->size >= f->max_size，
 * 如果f->size >= f->max_size，那么说明队列中的节点已经写满，
 * 此时如果再写，肯定会覆写未读数据，那么就需要继续等待。
 * 当无需等待时，windex指向的内存一定是已经读过的（除非代码异常了）。
*/
Frame *frame_queue_peek_writable(FrameQueue *f) {
    std::unique_lock<std::mutex> lock(*f->mutex);
    while (f->size >= f->max_size && !f->pktq->abort_request) {
        VIKTOR_LOGI("frame_queue_peek_writable SDL_CondWait");
        f->cond->wait(lock);
    }

    if (f->pktq->abort_request) return NULL;
    return &f->queue[f->windex];
}

/**用于在读完一个节点后调用，用于标记一个节点已经被读过
 * 两个行为：标记一个节点为已读，以及rindex_shown的赋值。
 * */
void frame_queue_next(FrameQueue *f) {

    /**如果支持keep_last，且rindex_shown为0，则rindex_shown赋1，返回
     * rindex_shown的意思是rindex指向的节点是否被读过，如果被读过， 为1，反之，为0
     */
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    /**
     * rindex加1，如果超过max_size，则回环为0,加锁情况下大小减1.
    */
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size) {
        f->rindex = 0;
    }
    f->mutex->lock();
    f->size--;
    f->cond->notify_one();
    f->mutex->unlock();

}

//读当前节点，与frame_queue_peek_readable等效，但没有检查是否有可读节点
Frame *frame_queue_peek(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

//读下一个节点
Frame *frame_queue_peek_next(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

//读上一个节点,即用户看到的画面
Frame *frame_queue_peek_last(FrameQueue *f) {
    return &f->queue[f->rindex];
}

int frameDisplayWidth(const AVFrame *frame){
    int ret;
    if (frame->sample_aspect_ratio.den != 0 && frame->sample_aspect_ratio.num != 0) {
        ret = frame->sample_aspect_ratio.den > frame->sample_aspect_ratio.num ?
              frame->width * frame->sample_aspect_ratio.num / frame->sample_aspect_ratio.den
                                                                              : frame->width;
    } else {
        ret = frame->width;
    }
    return ret + ret % 2;
}
int frameDisplayHeight(const AVFrame *frame){
    int ret;
    if (frame->sample_aspect_ratio.den != 0 && frame->sample_aspect_ratio.num != 0) {
        ret = frame->sample_aspect_ratio.den < frame->sample_aspect_ratio.num ?
              frame->height * frame->sample_aspect_ratio.den /
              frame->sample_aspect_ratio.num : frame->height;
    } else {
        ret = frame->height;
    }
    return ret + ret % 2;
}