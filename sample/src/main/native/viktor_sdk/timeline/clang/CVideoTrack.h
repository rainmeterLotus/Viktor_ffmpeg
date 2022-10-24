//
// Created by rainmeterLotus on 2021/1/23.
//

#ifndef VIKTOR_FFMPEG_CVIDEOTRACK_H
#define VIKTOR_FFMPEG_CVIDEOTRACK_H

#include <vector>
#include "CVideoClip.h"
#include "../util/ViktorCommon.h"

class CVideoTrack {
public:
    CVideoTrack() {}

    ~CVideoTrack() {
        VIKTOR_LOGD("~CVideoTrack");
    }

    CVideoClip *insertClipByIndex(std::string filePath, long startTime, long endTime, int index);

    CVideoClip *getClipByIndex(int index);

    int getVideoClipCount() {
        return vec_video_clip.size();
    }

    std::vector<CVideoClip *> *getVideoClips(){
        return &vec_video_clip;
    }

    void release(){
        for (auto clip:vec_video_clip) {
            clip->release();
            delete clip;
        }

        vec_video_clip.clear();
    }

protected:
    std::vector<CVideoClip *> vec_video_clip;
private:
    long m_duration_us;

};


#endif //VIKTOR_FFMPEG_CVIDEOTRACK_H
