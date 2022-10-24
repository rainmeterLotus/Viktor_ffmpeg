//
// Created by rainmeterLotus on 2022/1/4.
//

#ifndef VIKTOR_FFMPEG_IVIKTORDECODE_H
#define VIKTOR_FFMPEG_IVIKTORDECODE_H


#include "../context/viktor_context.h"

class IViktorDecode {
public:
    int decoder_decode_frame(ViktorDecoder *d, AVFrame *frame, CClip *clip);
    void release();

    CClip *current_clip = nullptr;
private:

};


#endif //VIKTOR_FFMPEG_IVIKTORDECODE_H
