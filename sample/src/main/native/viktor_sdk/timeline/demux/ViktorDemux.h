//
// Created by rainmeterLotus on 2021/12/18.
//

#ifndef VIKTOR_FFMPEG_VIKTORDEMUX_H
#define VIKTOR_FFMPEG_VIKTORDEMUX_H


#include "../context/viktor_context.h"
#include "../decode/ViktorVideoDecode.h"
#include "../decode/ViktorDecodeWrapper.h"

class ViktorDemux {

public:
    void read_frame_thread(ViktorContext *context);

    void stream_seek(ViktorContext *context, int64_t pos, int64_t rel, int seek_by_bytes);

    void stream_component_close(ViktorContext *context);
private:
    AVFormatContext *get_right_context(ViktorContext *context,int64_t pts);

    CVideoClip *find_current_clip(ViktorContext *context,int64_t pts,int track_index);

    void handle_clip_seek(ViktorContext *context,CClip *clip);
    int handle_seek(ViktorContext *context,AVFormatContext *in_fmt_ctx,CClip *curClip,CClip *preClip,AVPacket *pkt);
    void handle_pause(ViktorContext *context,CClip *clip);
    bool handle_enough(ViktorContext *context,CClip *clip);

    bool is_loop(ViktorContext *context, CClip *clip);

    int init_decode(ViktorContext *context,CClip *clip);

    void seek_to(ViktorContext *context, int64_t pos, int64_t rel, int seek_by_bytes);


    ViktorDecodeWrapper *m_decode_wrapper = nullptr;


    /**
     * 用于区分是否需要查找下一个clip
     */
    long m_start_us = 0;
    bool is_seek = false;

};


#endif //VIKTOR_FFMPEG_VIKTORDEMUX_H
