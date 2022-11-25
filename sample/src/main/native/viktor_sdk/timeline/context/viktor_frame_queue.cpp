//
// Created by rainmeterLotus on 2021/12/24.
//



#include "viktor_frame_queue.h"
int frame_queue_init(VKFrameQueue *f, VKPacketQueue *pktq, int max_size, int keep_last){
    memset(f,0,sizeof(VKFrameQueue));
    if (!(f->mutex = sdl_create_mutex())) {
        VIKTOR_LOGI( "frame_queue_init SDL_CreateMutex fail");
        return AVERROR(ENOMEM);
    }

    if (!(f->cond = sdl_create_cond())) {
        VIKTOR_LOGI( "frame_queue_init SDL_CreateCond fail");
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size,FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (int i = 0; i < f->max_size; ++i) {
        if (!(f->queue[i].frame = av_frame_alloc())){
            return AVERROR(ENOMEM);
        }
    }

    return 0;
}

void frame_queue_destroy(VKFrameQueue *f){
    if (!f) return;
    for (int i = 0; i < f->max_size; ++i) {
        VKFrame *vp= &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }

    delete f->mutex;
    f->mutex = nullptr;

    delete f->cond;
    f->cond = nullptr;
}

void frame_queue_signal(VKFrameQueue *f){
    if (!f) return;
    std::lock_guard<std::mutex> lock(*f->mutex);
    f->cond->notify_one();
}

void frame_queue_unref_item(VKFrame *vp){
    av_frame_unref(vp->frame);
}

int frame_queue_nb_remaining(VKFrameQueue *f){
    return f->size - f->rindex_shown;
}
VKFrame *frame_queue_peek_writable(VKFrameQueue *f){
    if (!f) return nullptr;
    std::unique_lock<std::mutex> lock(*f->mutex);
    while (f->size >= f->max_size && !f->pktq->abort_request) {
        VIKTOR_LOGI( "frame_queue_peek_writable SDL_CondWait");
        f->cond->wait(lock);
    }

    if (f->pktq->abort_request) return NULL;
    return &f->queue[f->windex];

}

VKFrame *frame_queue_peek_readable(VKFrameQueue *f){
    if (!f) return nullptr;
    std::unique_lock<std::mutex> lock(*f->mutex);
    while (f->size - f->rindex_shown <= 0 && !f->pktq->abort_request) {
        VIKTOR_LOGI( "frame_queue_peek_readable f->cond->wait");
        f->cond->wait(lock);
    }

    if (f->pktq->abort_request) return NULL;
    //因为rindex加1后可能超过max_size，所以这里取余
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void frame_queue_push(VKFrameQueue *f){
    if (!f) return;
    if (++f->windex == f->max_size) {
        f->windex = 0;
    }
    f->mutex->lock();
    f->size++;
    f->cond->notify_one();
    f->mutex->unlock();
}

/**用于在读完一个节点后调用，用于标记一个节点已经被读过
 * 两个行为：标记一个节点为已读，以及rindex_shown的赋值。
 * */
void frame_queue_next(VKFrameQueue *f){
    if (!f) return;
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
VKFrame *frame_queue_peek(VKFrameQueue *f){
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

//读下一个节点
VKFrame *frame_queue_peek_next(VKFrameQueue *f){
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

//读上一个节点,即用户看到的画面
VKFrame *frame_queue_peek_last(VKFrameQueue *f){
    return &f->queue[f->rindex];
}