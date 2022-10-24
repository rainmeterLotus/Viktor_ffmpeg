//
// Created by rainmeterLotus on 2021/1/24.
//

#ifndef VIKTOR_FFMPEG_CAUDIOCLIP_H
#define VIKTOR_FFMPEG_CAUDIOCLIP_H

#include "CClip.h"

class CAudioClip : public CClip {
public:
    CAudioClip(std::string filePath,long startTime,long endTime):CClip(filePath,startTime,endTime){

    }

    ~CAudioClip() {}
};


#endif //VIKTOR_FFMPEG_CAUDIOCLIP_H
