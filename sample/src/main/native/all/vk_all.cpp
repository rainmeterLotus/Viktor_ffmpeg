//
// Created by rainmeterLotus on 2022/1/4.
//

#include "vk_all.h"

void free_picture(VideoOverlay *bmp){
    if (!bmp){
        return;
    }

    auto *overlay = bmp;

    sws_freeContext(overlay->img_convert_ctx);
    if (overlay->managed_frame){
        av_frame_free(&overlay->managed_frame);
    }

    if (overlay->linked_frame){
        av_frame_unref(overlay->linked_frame);
        av_frame_free(&overlay->linked_frame);
    }
    if (overlay->frame_buffer) {
        av_buffer_unref(&overlay->frame_buffer);
    }

    if (overlay->mutex){
        delete overlay->mutex;
        overlay->mutex = nullptr;
    }

    memset(overlay,0,sizeof(VideoOverlay));
    free(overlay);
    bmp= nullptr;
}

VideoOverlay *create_overlay(int width,int height,int format,uint32_t vk_overlay_format){
    auto *overlay = (VideoOverlay *)calloc(1,sizeof(VideoOverlay));
    if (!overlay){
        return nullptr;
    }

    overlay->mutex = sdl_create_mutex();
    overlay->w = width;
    overlay->h = height;
    overlay->format = vk_overlay_format;
    overlay->frameFormat = format;

    AVFrame *managed_frame = av_frame_alloc();
    if (!managed_frame){
        free_picture(overlay);
        return nullptr;
    }
    AVFrame *linked_frame = av_frame_alloc();
    if (!linked_frame){
        av_frame_free(&managed_frame);
        free_picture(overlay);
        return nullptr;
    }
    managed_frame->format = format;
    managed_frame->width = width;
    managed_frame->height = height;
    av_image_fill_arrays(managed_frame->data, managed_frame->linesize, nullptr, static_cast<AVPixelFormat>(format),
                         width, height, 1);

    overlay->managed_frame = managed_frame;
    overlay->linked_frame = linked_frame;

    overlay_fill(overlay,managed_frame);
    return overlay;
}
void overlay_fill(VideoOverlay *overlay, AVFrame *frame){
    for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
        overlay->pixels[i] = frame->data[i];
        overlay->pitches[i] = frame->linesize[i];
    }
}



AVFrame *obtain_managed_frame_buffer(VideoOverlay *overlay)
{
    if (overlay->frame_buffer != NULL)
        return overlay->managed_frame;

    AVFrame *managed_frame = overlay->managed_frame;


    int width = managed_frame->width;
    int height = managed_frame->height;
    int sampleSize = calculate_InSample_Size(width,height,1080,1920);
    int destWidth = width/sampleSize;
    int destHeight = height/sampleSize;
    VIKTOR_LOGI("sws_scale_frame sampleSize:%d",sampleSize);
    VIKTOR_LOGI("sws_scale_frame width:%d,height:%d,managed_frame->linesize[0]:%d",width,height,managed_frame->linesize[0]);
    VIKTOR_LOGI("sws_scale_frame destWidth:%d,destHeight:%d",destWidth,destHeight);


    int frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, destWidth, destHeight, 1);
    AVBufferRef *frame_buffer_ref = av_buffer_alloc(frame_bytes);
    if (!frame_buffer_ref)
        return NULL;

    av_image_fill_arrays(managed_frame->data, managed_frame->linesize,
                         frame_buffer_ref->data, AV_PIX_FMT_RGBA, destWidth, destHeight, 1);

    managed_frame->width = destWidth;
    managed_frame->height = destHeight;
    overlay->w = destWidth;
    overlay->h = destHeight;
    overlay->frame_buffer  = frame_buffer_ref;
    return overlay->managed_frame;
}

int func_fill_frame(VideoOverlay *overlay, const AVFrame *frame) {
    if (!overlay) {
        return -1;
    }

    AVFrame swscale_dst_pic = {{0}};

    av_frame_unref(overlay->linked_frame);

    int use_linked_frame = 0;
    enum AVPixelFormat dst_format = AV_PIX_FMT_NONE;
    switch (frame->format) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_RGBA:
//            use_linked_frame = 1;
//            break;
        case AV_PIX_FMT_YUVJ420P:
        default:
            dst_format = AV_PIX_FMT_RGBA;
            break;
    }


    if (use_linked_frame) {
        VIKTOR_LOGE("use_linked_frame 11");
        av_frame_ref(overlay->linked_frame, frame);

        overlay_fill(overlay, overlay->linked_frame);

    } else {
        VIKTOR_LOGE("use_linked_frame 22");
        AVFrame *managed_frame = obtain_managed_frame_buffer(overlay);
        if (!managed_frame) {
            VIKTOR_LOGE("OOM in opaque_obtain_managed_frame_buffer");
            return -1;
        }
        VIKTOR_LOGE("use_linked_frame 44");
        overlay->img_convert_ctx = sws_getCachedContext(overlay->img_convert_ctx,
                                                        frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                                                        managed_frame->width, managed_frame->height,
                                                        dst_format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if (overlay->img_convert_ctx == NULL) {
            VIKTOR_LOGE( "sws_getCachedContext failed");
            return -1;
        }
        sws_scale(overlay->img_convert_ctx, (const uint8_t **) frame->data, frame->linesize,
                  0, frame->height, managed_frame->data, managed_frame->linesize);

        overlay->frameFormat = dst_format;
        overlay_fill(overlay, managed_frame);
    }


    if (use_linked_frame) {
        VIKTOR_LOGE("use_linked_frame 33");
        // do nothing
    } else if (image_convert(frame->width, frame->height,
                             dst_format, swscale_dst_pic.data, swscale_dst_pic.linesize,
                             static_cast<AVPixelFormat>(frame->format), (const uint8_t **) frame->data, frame->linesize)) {
    }
    return 0;
}


int image_convert(int width, int height,
                  enum AVPixelFormat dst_format, uint8_t **dst_data, int *dst_linesize,
                  enum AVPixelFormat src_format, const uint8_t **src_data, const int *src_linesize){
    switch (src_format) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P: // FIXME: 9 not equal to AV_PIX_FMT_YUV420P, but a workaround
            break;
        default:
            break;
    }
    return -1;
}

int calculate_InSample_Size(int srcWidth,int srcHeight,int reqWidth,int reqHeight){
    int inSampleSize = 1;

    if (srcHeight > reqHeight || srcWidth > reqWidth) {

        while ((srcWidth / inSampleSize) > reqHeight || (srcHeight / inSampleSize) > reqWidth) {
            inSampleSize *= 2;
        }
    }

    return inSampleSize;
}

int isLoop() {
    if (loop == 1) return 0;
    if (loop == 0) return 1;
    --loop;
    if (loop > 0) return 1;
    return 0;
}