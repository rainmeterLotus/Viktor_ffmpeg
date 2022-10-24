//
// Created by rainmeterLotus on 2021/1/23.
//

#include "CAudioTrack.h"
#include <string>

CAudioClip *CAudioTrack::insertClipByIndex(std::string filePath, long startTime, long endTime, int index) {
    if (!fileExist(filePath.c_str())){
        return nullptr;
    }
    auto *videoClip {
            new (std::nothrow)CAudioClip(std::move(filePath), startTime, endTime)
    };

    if (videoClip == nullptr){
        return nullptr;
    }

    /**
     * index 是 int
     * vec_audio_clip.size() 是 unsigned int
     */
    if (index > (int)vec_audio_clip.size()){
        index = vec_audio_clip.size();
    }

    if (index < 0){
        index = 0;
    }

    vec_audio_clip.insert(vec_audio_clip.begin() + index, videoClip);

    for (int i = 0; i <vec_audio_clip.size(); ++i) {
        CAudioClip *clip = vec_audio_clip.at(i);
        if (i == 0){
            clip->m_in_point_us = 0;
            clip->m_out_point_us = clip->m_end_micro_sec;
        } else {
            CAudioClip *prevClip = vec_audio_clip.at(i-1);
            clip->m_in_point_us = prevClip->m_out_point_us;
            clip->m_out_point_us = clip->m_end_micro_sec;
        }
    }
    return videoClip;
}

CAudioClip *CAudioTrack::getClipByIndex(int index) {
    if (vec_audio_clip.empty() || index < 0 || index >= vec_audio_clip.size()) {
        return NULL;
    }

    return vec_audio_clip[index];
}