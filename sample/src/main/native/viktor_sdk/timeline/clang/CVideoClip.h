//
// Created by rainmeterLotus on 2021/1/24.
//

#ifndef VIKTOR_FFMPEG_CVIDEOCLIP_H
#define VIKTOR_FFMPEG_CVIDEOCLIP_H

#include "CClip.h"

class CVideoClip : public CClip {
public:
    CVideoClip(std::string filePath,long startTime,long endTime):CClip(filePath,startTime,endTime){

    }

    ~CVideoClip() {}

};


#endif //VIKTOR_FFMPEG_CVIDEOCLIP_H
