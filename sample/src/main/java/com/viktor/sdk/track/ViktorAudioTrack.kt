package com.viktor.sdk.track

import com.viktor.sdk.clip.ViktorAudioClip

class ViktorAudioTrack: ViktorTrack() {
    fun appendClip(filePath:String,startTimeMicroSec:Long,endTimeMicroSec:Long): ViktorAudioClip? {
        return nativeInsertClip(objectAddress,filePath,startTimeMicroSec,endTimeMicroSec,getClipCount())
    }

    fun insertClipByIndex(filePath: String, startTimeMicroSec: Long, endTimeMicroSec: Long, index: Int): ViktorAudioClip? {
        return nativeInsertClip(objectAddress, filePath, startTimeMicroSec, endTimeMicroSec, index)
    }

    fun getClipByIndex(index: Int): ViktorAudioClip? {
        return nativeClipByIndex(objectAddress, index)
    }

    fun getClipCount():Int{
        return nativeGetClipCount(objectAddress)
    }


    private external fun nativeInsertClip(objectAddress: Long,filePath:String,startTimeMicroSec:Long,endTimeMicroSec:Long,index:Int): ViktorAudioClip?
    private external fun nativeClipByIndex(objectAddress: Long, index: Int): ViktorAudioClip?
    private external fun nativeGetClipCount(objectAddress: Long):Int
}