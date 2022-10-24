package com.viktor.sdk.timeline

import android.view.Surface
import com.viktor.sdk.ViktorAny
import com.viktor.sdk.impl.IViktorCallback

class ViktorActuator:ViktorAny() {

    private var callback: IViktorCallback? = null

    fun createTimeline(): ViktorTimeline? {
        return nativeTimeline()
    }

    fun releaseTimeline(timeline: ViktorTimeline?){
        timeline?.apply {
            nativeReleaseTimeline(getObjAddress())
        }
    }

    fun linkTimelineWithWindow(timeline: ViktorTimeline?) {
        timeline?.apply {
            nativeLinkTimelineWithWindow(getObjAddress())
        }
    }

    fun seek(timeline: ViktorTimeline?, microSecond:Long){
        timeline?.apply {
            nativeSeek(getObjAddress(),microSecond)
        }

    }

    fun play(timeline: ViktorTimeline?){
        timeline?.apply {
            nativePlay(getObjAddress())
        }

    }

    fun stop(timeline: ViktorTimeline?){
        timeline?.apply {
            nativeStop(getObjAddress())
        }

    }

    fun pause(timeline: ViktorTimeline?){
        timeline?.apply {
            nativePause(getObjAddress())
        }

    }

    fun setSurface(timeline: ViktorTimeline?,surface: Surface){
        timeline?.apply {
            nativeSetSurface(getObjAddress(),surface)
        }
    }

    fun setCallBack(callback:IViktorCallback){
        this.callback = callback
    }

    fun prepare(totalMs: Long,width:Int,height:Int) {
        callback?.onPrepare(totalMs,width,height)
    }

    fun progress(currentUs:Long){
        callback?.onCurrentUs(currentUs)
    }


    fun stateChanged(state:Int){
        callback?.onStateChanged(state)
    }

    private external fun nativeLinkTimelineWithWindow(timelineAddress:Long)
    private external fun nativeTimeline(): ViktorTimeline?
    private external fun nativeReleaseTimeline(timelineAddress:Long)
    private external fun nativeSeek(timelineAddress:Long,microSecond: Long)
    private external fun nativePlay(timelineAddress:Long)
    private external fun nativePause(timelineAddress:Long)
    private external fun nativeStop(timelineAddress:Long)
    private external fun nativeSetSurface(timelineAddress:Long, surface: Surface)
}