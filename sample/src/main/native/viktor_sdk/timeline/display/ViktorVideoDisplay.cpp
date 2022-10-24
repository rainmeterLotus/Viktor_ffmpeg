//
// Created by rainmeterLotus on 2022/1/4.
//

#include "ViktorVideoDisplay.h"

void ViktorVideoDisplay::setSurface(JNIEnv *env,jobject surface){
    if (m_window) {
        ANativeWindow_release(m_window);
    }

    m_window = ANativeWindow_fromSurface(env, surface);
}

void ViktorVideoDisplay::start(ViktorContext *context){
    m_display_thread = new (std::nothrow)std::thread(func_display_thread,context,this);
    if (!m_display_thread){
        VIKTOR_LOGE("ViktorVideoDisplay func_display_thread ERROR");
        return;
    }
}

void ViktorVideoDisplay::func_display_thread(void *arg,void *context){
    auto *viktor_context = static_cast<ViktorContext *>(arg);
    auto *vikt_display = static_cast<ViktorVideoDisplay *>(context);

    double remaining_time = 0.0;
    for (;;) {
        if (viktor_context->abort_request) {
            VIKTOR_LOGI("refresh_loop_wait_event break");
            break;
        }

        if (remaining_time > 0.0) {
            av_usleep((int64_t) (remaining_time * 1000000.0));
        }
        remaining_time = 0.01;

        if (viktor_context->show_mode != SHOW_MODE_NONE && (!viktor_context->paused || viktor_context->force_refresh)) {
            vikt_display->video_refresh(viktor_context, &remaining_time);
        }
    }

}


void ViktorVideoDisplay::video_refresh(void *opaque, double *remaining_time){
    auto *is = static_cast<ViktorContext *>(opaque);
    double time;

    if (true) {
        retry:
//        if (!is->audio_st && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK){
//            change_external_clock_speed(is);
//        }
        if (frame_queue_nb_remaining(&is->picture_frame_q) == 0) {
            VIKTOR_LOGI("video_refresh remaining is 0");
        } else {
            VIKTOR_LOGI("video_refresh remaining is not 0");
            double last_duration, duration, delay;
            //vp 这次将要显示的目标帧
            //lastvp 已经显示了的帧（也是当前屏幕上看到的帧）
            VKFrame *vp, *lastvp;

            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->picture_frame_q);
            vp = frame_queue_peek(&is->picture_frame_q);

            //需要先判断frame_queue_peek获取的vp是否是最新序列
            if (vp->serial != is->video_packet_q.serial) {
                //说明发生过seek等操作，流不连续，应该抛弃lastvp。
                //故调用frame_queue_next抛弃lastvp后，返回流程开头重试下一轮。
                VIKTOR_LOGI("video_refresh vp->serial != is->videoq.serial");
                frame_queue_next(&is->picture_frame_q);
                goto retry;
            }

            if (lastvp->serial != vp->serial) {
                VIKTOR_LOGI("video_refresh lastvp->serial != vp->serial");
                is->frame_timer = av_gettime_relative() / 1000000.0;
            }

            if (is->paused) {
                VIKTOR_LOGI("video_refresh is->paused display");
                goto display;
            }


            /** compute nominal last_duration 计算latstvp显示的时长*/
            last_duration = vp_duration(is, lastvp, vp);
            /** 考虑到同步，比如视频同步到音频，则还需要考虑当前与主时钟的差距,故要参考audio clock计算上一帧真正的持续时长 */
            delay = compute_target_delay(last_duration, is);

            time = av_gettime_relative() / 1000000.0;//获取当前系统时间(单位秒)

            //VIKTOR_LOGI("video_refresh last_duration:%lf,delay:%lf,time:%lf,frame_timer+delay:%lf",last_duration,delay,time,is->frame_timer + delay)
            //视频播放过快，重复显示
            if (time < is->frame_timer + delay) {//如果上一帧显示时长未满，重复显示上一帧
                //VIKTOR_LOGI("video_refresh time < is->frame_timer + delay")
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                VIKTOR_LOGI("video_refresh 0000");
                goto display;
            }
            VIKTOR_LOGI("video_refresh 444");
            is->frame_timer += delay;//更新frame_timer，现在表示vp的显示时刻
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX) {
                VIKTOR_LOGI("video_refresh is->frame_timer > AV_SYNC_THRESHOLD_MAX");
                is->frame_timer = time;//如果和系统时间差距太大，就纠正为系统时间
            }
            is->picture_frame_q.mutex->lock();
            if (!isnan(vp->pts)) {
                update_video_pts(is, vp->pts, vp->serial);//更新vidclk
            }
            is->picture_frame_q.mutex->unlock();
            //视频播放过慢，追赶音频，丢帧
            if (frame_queue_nb_remaining(&is->picture_frame_q) > 1) {//只有有nextvp才会丢帧
                VKFrame *nextvp = frame_queue_peek_next(&is->picture_frame_q);
                duration = vp_duration(is, vp, nextvp);
                //系统时刻大于vp结束显示时刻(time > is->frame_timer + duration)
                if (!is->step && (framedrop > 0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration) {
                    VIKTOR_LOGI("video_refresh 只有有nextvp才会丢帧");
                    frame_queue_next(&is->picture_frame_q);
                    goto retry;
                }
            }

            VIKTOR_LOGI("video_refresh GOGO");
            frame_queue_next(&is->picture_frame_q);
            is->force_refresh = 1;

            if (is->step && !is->paused) {
                stream_toggle_pause(is);
            }
        }

        display:
        bool force = is->force_refresh;
        bool showMode = is->show_mode == SHOW_MODE_VIDEO;
        bool rindexS = is->picture_frame_q.rindex_shown;
        VIKTOR_LOGI("video_refresh force:%d,showMode:%d,rindexS:%d", force, showMode, rindexS);
        if (is->force_refresh && is->show_mode == SHOW_MODE_VIDEO && is->picture_frame_q.rindex_shown) {
            VIKTOR_LOGI("video_refresh video_display");
            video_display(is);
        }

    } else {
        VIKTOR_LOGI("video_refresh is->video_st null");
    }

    is->force_refresh = 0;
}

void ViktorVideoDisplay::video_display(ViktorContext *context){
    VKFrame *vp;
    vp = frame_queue_peek_last(&context->picture_frame_q);
    VIKTOR_LOGI("video_display GOGO vp->uploaded:%d", vp->uploaded);
    VIKTOR_LOGI("video_display GOGO vp->pts:%lf,pts_us:%ld", vp->pts,(long)vp->pts * 1000000);
    if (!vp->uploaded) {//如果是重复显示上一帧，那么uploaded就是1

        if (m_videoTexture && vp->bmp) {
            VIKTOR_LOGI("video_display format:%d,formatName:%s",vp->bmp->frameFormat,av_get_pix_fmt_name(static_cast<AVPixelFormat>
                                                                                                       (vp->bmp->frameFormat)));
            m_videoTexture->display_overlay(vp->bmp, m_window,overlay_format);
        }
        vp->uploaded = 1;


        long progress = static_cast<long>(get_master_clock(context) * 1000000);
        long progress1 = static_cast<long>(get_progress(&context->extclk) * 1000000);
        VIKTOR_LOGI("video_display GOGO progress:%ld,progress1:%ld", progress,progress1);
        context->m_progressCallback(context,progress1);
    }
}


void ViktorVideoDisplay::release(){
    if (m_videoTexture) {
        m_videoTexture->close();
    }

    if (m_window) {
        ANativeWindow_release(m_window);
    }
}