package com.viktor.sdk.track

import com.viktor.sdk.clip.ViktorVideoClip

class ViktorVideoTrack : ViktorTrack() {

    fun appendClip(filePath: String, startTimeMicroSec: Long, endTimeMicroSec: Long): ViktorVideoClip? {
        return nativeInsertClip(objectAddress, filePath, startTimeMicroSec, endTimeMicroSec, getClipCount())
    }

    fun insertClipByIndex(filePath: String, startTimeMicroSec: Long, endTimeMicroSec: Long, index: Int): ViktorVideoClip? {
        return nativeInsertClip(objectAddress, filePath, startTimeMicroSec, endTimeMicroSec, index)
    }

    fun getClipByIndex(index: Int): ViktorVideoClip? {
        return nativeClipByIndex(objectAddress, index)
    }

    fun getClipCount(): Int {
        return nativeGetClipCount(objectAddress)
    }


    private external fun nativeInsertClip(objectAddress: Long, filePath: String, startTimeMicroSec: Long, endTimeMicroSec: Long, index: Int): ViktorVideoClip?
    private external fun nativeClipByIndex(objectAddress: Long, index: Int): ViktorVideoClip?
    private external fun nativeGetClipCount(objectAddress: Long): Int
}