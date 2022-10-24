//
// Created by rainmeterLotus on 2021/1/23.
//

#include "CTimeline.h"

CVideoTrack *CTimeline::insertVideoTrack(int index) {
    if (m_timeline_wrapper) {
        return m_timeline_wrapper->insertVideoTrack(index);
    }
    return nullptr;
}

CAudioTrack *CTimeline::insertAudioTrack(int index) {
    if (m_timeline_wrapper) {
        return m_timeline_wrapper->insertAudioTrack(index);
    }
    return nullptr;
}

CVideoTrack *CTimeline::getVideoTrackByIndex(int index) {
    if (m_timeline_wrapper) {
        return m_timeline_wrapper->getVideoTrackByIndex(index);
    }
    return nullptr;
}

CAudioTrack *CTimeline::getAudioTrackByIndex(int index) {
    if (m_timeline_wrapper) {
        return m_timeline_wrapper->getAudioTrackByIndex(index);
    }
    return nullptr;
}