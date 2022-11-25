//
// Created by rainmeterLotus on 2022/1/4.
//

#ifndef VIKTOR_FFMPEG_VIKTORVIDEODECODE_H
#define VIKTOR_FFMPEG_VIKTORVIDEODECODE_H


#include "IViktorDecode.h"

class ViktorVideoDecode: public IViktorDecode{
public:
    int decode_start(ViktorContext *context,CClip *clip);

private:
    static int video_thread(void *arg,void *context);


    int get_video_frame(ViktorContext *context, AVFrame *frame,CClip *clip);
    int queue_picture(ViktorContext *context, AVFrame *src_frame, double pts,
                      double duration, int64_t pos, int serial,int orientation);

    void alloc_picture(VKFrame *frame, int format);
};


#endif //VIKTOR_FFMPEG_VIKTORVIDEODECODE_H
