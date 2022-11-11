package com.viktor.sdk.view

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.Gravity
import android.view.SurfaceHolder
import android.view.ViewGroup
import android.widget.FrameLayout
import com.viktor.fg.BuildConfig
import com.viktor.sdk.impl.IRenderCallback
import com.viktor.sdk.impl.IViktorCallback
import com.viktor.sdk.timeline.ViktorActuator
import com.viktor.sdk.timeline.ViktorTimeline



class ViktorVideoView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) , IViktorCallback {

    companion object {
        const val TAG = "ViktorVideoView"
        val DEBUG = BuildConfig.DEBUG

        const val STATE_UNKNOWN = 0
        const val STATE_START = 1
        const val STATE_PAUSE = 2
        const val STATE_END = 3
    }

    private var listener: OnViePlayCallback? = null
    private var videoDuration = 0L
    private var mVideoWidth = 0
    private var mVideoHeight = 0
    private val vieSurfaceView = ViktorSurfaceView(context)


    private var state = 0

    private var viktorActuator:ViktorActuator? = null
    private var viktorTimeline:ViktorTimeline? = null

    init {
        initVideoView()
    }

    private fun initVideoView() {

        vieSurfaceView.setRenderCallback(object : IRenderCallback {
            override fun onSurfaceCreated(holder: SurfaceHolder?) {
                if(DEBUG){
                    Log.d(TAG, "onSurfaceCreated")
                }
                holder?.let {
//                    vieCore?.setSurface(it.surface)

                    viktorTimeline?.let {timeline->
                        viktorActuator?.setSurface(timeline,it.surface)
                    }
                }
            }

            override fun onSurfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
            }

            override fun onSurfaceDestroyed(holder: SurfaceHolder?) {
            }

        })

        val lp = LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER)
        vieSurfaceView.layoutParams = lp
        addView(vieSurfaceView)

        isFocusable = true
        isFocusableInTouchMode = true
        requestFocus()

    }

    fun start() {
        viktorActuator?.play(viktorTimeline)
    }

    fun release() {
        viktorActuator?.stop(viktorTimeline)
    }

    fun resume(){
        if (state == STATE_PAUSE){
            viktorActuator?.play(viktorTimeline)
        }
    }

    fun seek(pos: Long) {
        viktorActuator?.seek(viktorTimeline,pos * 1000)
    }

    fun pause() {
        viktorActuator?.pause(viktorTimeline)
    }

    fun changeSpeed(speed:Float){
    }

    fun isPlaying():Boolean{
        return state == STATE_START
    }

    fun setViktorActuator(viktorActuator: ViktorActuator,viktorTimeline:ViktorTimeline){
        this.viktorActuator = viktorActuator
        this.viktorTimeline = viktorTimeline
        this.viktorActuator?.setCallBack(this)
    }


    override fun onPrepare(totalUs: Long, width: Int, height: Int) {
        if(DEBUG){
            Log.d(TAG, "==onPrepare totalUs:$totalUs,width:$width,height:$height")
        }
        this.post {
            videoDuration = totalUs/1000
            mVideoWidth = width
            mVideoHeight = height

            vieSurfaceView.setVideoSize(width,height)
            listener?.onPrepare(videoDuration, width, height)
        }
    }


    fun calculateInSampleSize(srcWidth:Int,srcHeight:Int,reqWidth:Int,reqHeight:Int):Int{

        var inSampleSize = 1

        if (srcHeight > reqHeight || srcWidth > reqWidth) {

            while ((srcWidth / inSampleSize) > reqHeight || (srcHeight / inSampleSize) > reqWidth) {
                inSampleSize *= 2;
            }
        }

        return inSampleSize
    }

    override fun onCurrentUs(currentUs: Long) {
        listener?.onProgress(currentUs/1000)
    }

    override fun onStateChanged(state: Int) {
        this.state = state
        listener?.onStateChanged(state)
    }


    fun setOnViePlayCallback(listener: OnViePlayCallback) {
        this.listener = listener
    }

    interface OnViePlayCallback {
        fun onPrepare(totalMs: Long, width: Int, height: Int)
        fun onProgress(currentMs: Long)
        fun onStateChanged(state: Int)
    }

}