package com.viktor.sdk.clip


class ViktorVideoClip: ViktorClip() {

    fun getOriginalWith():Int{
        return nativeOriginalWidth(objectAddress)
    }

    fun getOriginalHeight():Int{
        return nativeOriginalHeight(objectAddress)
    }

    fun getOriginalDuration():Long{
        return nativeOriginalDuration(objectAddress)
    }

    fun getFilePath():String?{
        return nativeFilePath(objectAddress)
    }

    fun getStartTime():Long{
        return nativeStartTime(objectAddress)
    }

    fun getEndTime():Long{
        return nativeEndTime(objectAddress)
    }

    private external fun nativeOriginalWidth(objectAddress:Long):Int
    private external fun nativeOriginalHeight(objectAddress:Long):Int
    private external fun nativeOriginalDuration(objectAddress:Long):Long
    private external fun nativeFilePath(objectAddress:Long):String?
    private external fun nativeStartTime(objectAddress:Long):Long
    private external fun nativeEndTime(objectAddress:Long):Long
}