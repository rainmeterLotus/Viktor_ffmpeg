//
// Created by rainmeterLotus on 2022/1/4.
//

#ifndef VIKTOR_FFMPEG_VK_ALL_H
#define VIKTOR_FFMPEG_VK_ALL_H
#include <string.h>
#include "ff_define.h"
#include "../viktor_sdk/timeline/util/ViktorLog.h"
#include "../viktor_sdk/timeline/context/viktor_sdl.h"

extern "C"{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

static uint32_t overlay_format = SDL_FCC__GLES2;

enum{
    CODE_UNKNOWN = 0,
    CODE_START = 1,
    CODE_PAUSE = 2,
    CODE_END = 3,
};

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

enum ShowMode {
    SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
};


static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static enum ShowMode show_mode = SHOW_MODE_NONE;
static int64_t audio_callback_time;
static int startup_volume = 100;

/**
 * 某些只有视频流，播放时，因为pf_playback_rate>=2.0,
 * 导致在get_video_frame中一直丢帧，画面近似卡死
 */
static int default_frame_drop = 2;
/**
 * 是否需要重新排序时间戳
 */
static int decoder_reorder_pts = -1;

/**
 * 控制是否丢帧的开关变量
 */
static int framedrop = -1;

/**
 * 0无限次播放
 */
static int loop = 0;
static int autoexit = 0;

typedef struct VideoOverlay {
    int w;
    int h;
    uint32_t format;
    int frameFormat;
    int planes;
    uint16_t pitches[AV_NUM_DATA_POINTERS];
    uint8_t *pixels[AV_NUM_DATA_POINTERS];

    AVFrame *managed_frame;
    AVFrame *linked_frame;

    AVBufferRef *frame_buffer;

    int no_neon_warned;

    struct SwsContext *img_convert_ctx;
    int sws_flags;
    int sar_num;
    int sar_den;
    int orientation;
    /**
     * 用于区分是否同一视频源
     */
    int serial;
    /**
     * 规定的宽高，播放解码规定好的宽高，不是视频原始宽高
     */
    int fixWidth;
    int fixHeight;

    std::mutex *mutex;
} VideoOverlay;


typedef void (*SDL_PrepareCallback) (void *arg,void *vikt_ctx);
typedef void (*SDL_ProgressCallback) (void *vikt_ctx,long durationUs);


void free_picture(VideoOverlay *bmp);
VideoOverlay *create_overlay(int width,int height,int format,uint32_t vk_overlay_format);
void overlay_fill(VideoOverlay *overlay, AVFrame *frame);
int func_fill_frame(VideoOverlay *overlay, const AVFrame *frame);

int image_convert(int width, int height,
                  enum AVPixelFormat dst_format, uint8_t **dst_data, int *dst_linesize,
                  enum AVPixelFormat src_format, const uint8_t **src_data, const int *src_linesize);
int calculate_InSample_Size(int srcWidth,int srcHeight,int reqWidth,int reqHeight);

int isLoop();
#endif //VIKTOR_FFMPEG_VK_ALL_H
