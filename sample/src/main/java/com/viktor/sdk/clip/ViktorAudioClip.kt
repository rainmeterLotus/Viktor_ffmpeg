package com.viktor.sdk.clip

class ViktorAudioClip: ViktorClip() {

    fun getOriginalDuration():Long{
        return nativeOriginalDuration(objectAddress)
    }

    private external fun nativeOriginalDuration(objectAddress:Long):Long
}