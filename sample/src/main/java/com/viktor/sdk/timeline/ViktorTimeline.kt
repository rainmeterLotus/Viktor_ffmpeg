package com.viktor.sdk.timeline

import com.viktor.sdk.ViktorAny
import com.viktor.sdk.track.ViktorAudioTrack
import com.viktor.sdk.track.ViktorVideoTrack

class ViktorTimeline : ViktorAny() {

    fun appendVideoTrack(): ViktorVideoTrack? {
        return nativeAppendVideoTrack(objectAddress)
    }

    fun insertVideoTrack(index: Int): ViktorVideoTrack? {
        return nativeInsertVideoTrack(objectAddress, index)
    }

    fun appendAudioTrack(): ViktorAudioTrack? {
        return nativeAppendAudioTrack(objectAddress)
    }

    fun insertAudioTrack(index: Int): ViktorAudioTrack? {
        return nativeInsertAudioTrack(objectAddress, index)
    }

    fun getVideoTrackByIndex(index: Int): ViktorVideoTrack? {
        return nativeVideoTrackByIndex(objectAddress, index)
    }

    fun getAudioTrackByIndex(index: Int): ViktorAudioTrack? {
        return nativeAudioTrackByIndex(objectAddress, index)
    }

    fun getVideoTrackCount(): Int {
        return nativeVideoTrackCount(objectAddress)
    }

    fun getAudioTrackCount(): Int {
        return nativeAudioTrackCount(objectAddress)
    }

    fun getDurationUs():Long{
        return nativeDurationUs(objectAddress)
    }

    private external fun nativeAppendVideoTrack(timelineAddress: Long): ViktorVideoTrack?
    private external fun nativeAppendAudioTrack(timelineAddress: Long): ViktorAudioTrack?
    private external fun nativeInsertVideoTrack(timelineAddress: Long, index: Int): ViktorVideoTrack?
    private external fun nativeInsertAudioTrack(timelineAddress: Long, index: Int): ViktorAudioTrack?
    private external fun nativeVideoTrackByIndex(timelineAddress: Long, index: Int): ViktorVideoTrack?
    private external fun nativeAudioTrackByIndex(timelineAddress: Long, index: Int): ViktorAudioTrack?

    private external fun nativeVideoTrackCount(timelineAddress: Long): Int
    private external fun nativeAudioTrackCount(timelineAddress: Long): Int
    private external fun nativeDurationUs(timelineAddress: Long): Long
}