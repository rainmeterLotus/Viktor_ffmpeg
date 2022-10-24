package com.viktor.sdk.view

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import com.viktor.fg.BuildConfig
import com.viktor.sdk.impl.IRenderCallback

class ViktorSurfaceView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr) {
    
    companion object{
        val AR_ASPECT_FIT_PARENT = 0 // without clip

        val AR_ASPECT_FILL_PARENT = 1 // may clip

        val AR_ASPECT_WRAP_CONTENT = 2
        val AR_MATCH_PARENT = 3
        val AR_16_9_FIT_PARENT = 4
        val AR_4_3_FIT_PARENT = 5

        const val TAG = "VieSurfaceView"
        val DEBUG = BuildConfig.DEBUG
    }

    private var mVideoWidth = 0
    private var mVideoHeight = 0

    private val mVideoRotationDegree = 0

    private var renderCallback: IRenderCallback? = null

    private val ALL_RATIO = intArrayOf(
        AR_ASPECT_FIT_PARENT,
        AR_ASPECT_FILL_PARENT,
        AR_ASPECT_WRAP_CONTENT,
        AR_16_9_FIT_PARENT,
        AR_4_3_FIT_PARENT
    )

    private val wantRatio = ALL_RATIO[0]

    init {

        holder.addCallback(object : SurfaceHolder.Callback{
            override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
                if(DEBUG){
                    Log.d(TAG, "surfaceChanged width:$width,height:$height")
                    Log.d(TAG, "surfaceChanged getWidth():${getWidth()},getHeight():${getHeight()}")
                }
                renderCallback?.onSurfaceChanged(holder,format,width,height)
            }

            override fun surfaceDestroyed(holder: SurfaceHolder?) {
                if(DEBUG){
                    Log.d(TAG, "surfaceDestroyed")
                }
                renderCallback?.onSurfaceDestroyed(holder)
            }

            override fun surfaceCreated(holder: SurfaceHolder?) {
                if(DEBUG){
                    Log.d(TAG, "surfaceCreated")
                }

                renderCallback?.onSurfaceCreated(holder)
            }

        })

    }

    fun setRenderCallback(renderCallback: IRenderCallback){
        this.renderCallback = renderCallback
    }

    fun setVideoSize(videoWidth:Int,videoHeight:Int){
        if (videoWidth > 0 && videoHeight > 0){
            if(DEBUG){
                Log.d(TAG, "setVideoSize videoWidth:$videoWidth,videoHeight:$videoHeight")
            }
            mVideoWidth = videoWidth
            mVideoHeight = videoHeight
            holder.setFixedSize(videoWidth,videoHeight)
            requestLayout()
        }
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)

        var width = View.getDefaultSize(mVideoWidth, widthMeasureSpec)
        var height = View.getDefaultSize(mVideoHeight, heightMeasureSpec)
        if(DEBUG){
            Log.d(TAG, "onMeasure width:$width,height:$height")
        }
        if (mVideoWidth > 0 && mVideoHeight > 0){
            val widthSpecMode = MeasureSpec.getMode(widthMeasureSpec)
            val widthSpecSize = MeasureSpec.getSize(widthMeasureSpec)
            val heightSpecMode = MeasureSpec.getMode(heightMeasureSpec)
            val heightSpecSize = MeasureSpec.getSize(heightMeasureSpec)


            if (widthSpecMode == MeasureSpec.AT_MOST && heightSpecMode == MeasureSpec.AT_MOST) {
                val specAspectRatio = widthSpecSize.toFloat() / heightSpecSize.toFloat()

                var displayAspectRatio: Float
                when (wantRatio) {
                    AR_16_9_FIT_PARENT -> {
                        displayAspectRatio = 16.0f / 9.0f
                        if (mVideoRotationDegree == 90 || mVideoRotationDegree == 270) displayAspectRatio = 1.0f / displayAspectRatio
                    }
                    AR_4_3_FIT_PARENT -> {
                        displayAspectRatio = 4.0f / 3.0f
                        if (mVideoRotationDegree == 90 || mVideoRotationDegree == 270) displayAspectRatio = 1.0f / displayAspectRatio
                    }
                    AR_ASPECT_FIT_PARENT, AR_ASPECT_FILL_PARENT, AR_ASPECT_WRAP_CONTENT -> {
                        displayAspectRatio = mVideoWidth.toFloat() / mVideoHeight.toFloat()
                    }
                    else -> {
                        displayAspectRatio = mVideoWidth.toFloat() / mVideoHeight.toFloat()
                    }
                }

                val shouldBeWider = displayAspectRatio > specAspectRatio

                when (wantRatio) {
                    AR_ASPECT_FIT_PARENT, AR_16_9_FIT_PARENT, AR_4_3_FIT_PARENT -> if (shouldBeWider) {
                        // too wide, fix width
                        width = widthSpecSize
                        height = (width / displayAspectRatio).toInt()
                    } else {
                        // too high, fix height
                        height = heightSpecSize
                        width = (height * displayAspectRatio).toInt()
                    }
                    AR_ASPECT_FILL_PARENT -> if (shouldBeWider) {
                        // not high enough, fix height
                        height = heightSpecSize
                        width = (height * displayAspectRatio).toInt()
                    } else {
                        // not wide enough, fix width
                        width = widthSpecSize
                        height = (width / displayAspectRatio).toInt()
                    }
                    AR_ASPECT_WRAP_CONTENT -> if (shouldBeWider) {
                        // too wide, fix width
                        width = Math.min(mVideoWidth, widthSpecSize)
                        height = (width / displayAspectRatio).toInt()
                    } else {
                        // too high, fix height
                        height = Math.min(mVideoHeight, heightSpecSize)
                        width = (height * displayAspectRatio).toInt()
                    }
                    else -> if (shouldBeWider) {
                        width = Math.min(mVideoWidth, widthSpecSize)
                        height = (width / displayAspectRatio).toInt()
                    } else {
                        height = Math.min(mVideoHeight, heightSpecSize)
                        width = (height * displayAspectRatio).toInt()
                    }
                }
            } else if (widthSpecMode == MeasureSpec.EXACTLY && heightSpecMode == MeasureSpec.EXACTLY) {
                width = widthSpecSize
                height = heightSpecSize

                // for compatibility, we adjust size based on aspect ratio
                if (mVideoWidth * height < width * mVideoHeight) {
                    //Log.i("@@@", "image too wide, correcting");
                    width = height * mVideoWidth / mVideoHeight
                } else if (mVideoWidth * height > width * mVideoHeight) {
                    //Log.i("@@@", "image too tall, correcting");
                    height = width * mVideoHeight / mVideoWidth
                }
            } else if (widthSpecMode == MeasureSpec.EXACTLY) {
                // only the width is fixed, adjust the height to match aspect ratio if possible
                width = widthSpecSize
                height = width * mVideoHeight / mVideoWidth
                if (heightSpecMode == MeasureSpec.AT_MOST && height > heightSpecSize) {
                    // couldn't match aspect ratio within the constraints
                    height = heightSpecSize
                }
            } else if (heightSpecMode == MeasureSpec.EXACTLY) {
                // only the height is fixed, adjust the width to match aspect ratio if possible
                height = heightSpecSize
                width = height * mVideoWidth / mVideoHeight
                if (widthSpecMode == MeasureSpec.AT_MOST && width > widthSpecSize) {
                    // couldn't match aspect ratio within the constraints
                    width = widthSpecSize
                }
            } else {
                // neither the width nor the height are fixed, try to use actual video size
                width = mVideoWidth
                height = mVideoHeight
                if (heightSpecMode == MeasureSpec.AT_MOST && height > heightSpecSize) {
                    // too tall, decrease both width and height
                    height = heightSpecSize
                    width = height * mVideoWidth / mVideoHeight
                }
                if (widthSpecMode == MeasureSpec.AT_MOST && width > widthSpecSize) {
                    // too wide, decrease both width and height
                    width = widthSpecSize
                    height = width * mVideoHeight / mVideoWidth
                }
            }
        }

        setMeasuredDimension(width,height)

    }
}