//
// Created by rainmeterLotus on 2021/1/23.
//

#ifndef VIKTOR_FFMPEG_CAUDIOTRACK_H
#define VIKTOR_FFMPEG_CAUDIOTRACK_H

#include <vector>
#include "CAudioClip.h"
#include "../util/ViktorCommon.h"

class CAudioTrack {

public:
    CAudioTrack() {
    }

    ~CAudioTrack() {
        VIKTOR_LOGD("~CAudioTrack");
    }


    CAudioClip *insertClipByIndex(std::string filePath, long startTime, long endTime, int index);

    CAudioClip *getClipByIndex(int index);

    int getAudioClipCount() {
        return vec_audio_clip.size();
    }

    std::vector<CAudioClip *> *getAudioClips(){
        return &vec_audio_clip;
    }

    void release(){
        for (auto clip:vec_audio_clip) {
            clip->release();
            delete clip;
        }

        vec_audio_clip.clear();
    }

protected:
    std::vector<CAudioClip *> vec_audio_clip;
};


#endif //VIKTOR_FFMPEG_CAUDIOTRACK_H
