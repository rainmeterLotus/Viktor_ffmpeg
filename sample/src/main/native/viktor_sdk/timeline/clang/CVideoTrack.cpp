//
// Created by rainmeterLotus on 2021/1/23.
//

#include <string>
#include <utility>
#include "CVideoTrack.h"


CVideoClip *CVideoTrack::insertClipByIndex(std::string filePath, long startTime, long endTime, int index) {
    if (!fileExist(filePath.c_str())){
        return nullptr;
    }
    auto *videoClip{
            new (std::nothrow)CVideoClip(std::move(filePath), startTime, endTime)
    };
    if (videoClip == nullptr){
        return nullptr;
    }

    /**
     * index 是 int
     * vec_video_clip.size() 是 unsigned int
     */
    if (index > (int)vec_video_clip.size()){
        index = vec_video_clip.size();
    }

    if (index < 0){
        index = 0;
    }

    vec_video_clip.insert(vec_video_clip.begin() + index, videoClip);

    int size = vec_video_clip.size();
    for (int i = 0; i < size; ++i) {
        CVideoClip *clip = vec_video_clip.at(i);
        clip->m_index = i;
        clip->isLast = false;
        if (i == 0){
            clip->m_in_point_us = 0;
            clip->m_out_point_us = clip->m_end_micro_sec - clip->m_start_micro_sec;
        } else {
            CVideoClip *prevClip = vec_video_clip.at(i-1);
            clip->m_in_point_us = prevClip->m_out_point_us;
            clip->m_out_point_us = clip->m_in_point_us + (clip->m_end_micro_sec - clip->m_start_micro_sec);
        }
    }
    vec_video_clip.front()->isFirst = true;
    vec_video_clip.back()->isLast = true;
    return videoClip;
}

CVideoClip *CVideoTrack::getClipByIndex(int index) {
    if (vec_video_clip.empty() || index < 0 || index >= vec_video_clip.size()) {
        return NULL;
    }

    return vec_video_clip[index];
}