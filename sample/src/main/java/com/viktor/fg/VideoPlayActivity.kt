package com.viktor.fg

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import com.viktor.fg.loader.SelectMedia
import com.viktor.fg.util.ViktorConstants
import com.viktor.fg.util.ViktorUtil
import com.viktor.sdk.timeline.ViktorActuator
import com.viktor.sdk.view.ViktorVideoView
import kotlinx.android.synthetic.main.activity_video_play.*
import kotlinx.android.synthetic.main.activity_video_play.seek_bar

class VideoPlayActivity : AppCompatActivity() {


    companion object {
        const val TAG = "video_play_a"
        val DEBUG = BuildConfig.DEBUG

        fun startActivity(context: Context, mediaPathList: ArrayList<SelectMedia>?) {
            val intent = Intent(context, VideoPlayActivity::class.java)
            intent.putParcelableArrayListExtra(ViktorConstants.INTENT_DATA, mediaPathList)
            context.startActivity(intent)
        }

        private const val BASE_TIME = 100
    }

    private var isSeeking = false

    /**
     * 玩具总动员：http://vfx.mtime.cn/Video/2019/03/19/mp4/190319212559089721.mp4
     * 惊奇队长：http://vfx.mtime.cn/Video/2019/02/04/mp4/190204084208765161.mp4
     * 叶问4：http://vfx.mtime.cn/Video/2019/03/18/mp4/190318231014076505.mp4

    http://vfx.mtime.cn/Video/2019/03/18/mp4/190318214226685784.mp4
    http://vfx.mtime.cn/Video/2019/03/19/mp4/190319104618910544.mp4
    http://vfx.mtime.cn/Video/2019/03/19/mp4/190319125415785691.mp4
    http://vfx.mtime.cn/Video/2019/03/17/mp4/190317150237409904.mp4
    http://vfx.mtime.cn/Video/2019/03/14/mp4/190314223540373995.mp4
    http://vfx.mtime.cn/Video/2019/03/14/mp4/190314102306987969.mp4
    http://vfx.mtime.cn/Video/2019/03/13/mp4/190313094901111138.mp4
    http://vfx.mtime.cn/Video/2019/03/12/mp4/190312143927981075.mp4
    http://vfx.mtime.cn/Video/2019/03/12/mp4/190312083533415853.mp4
    http://vfx.mtime.cn/Video/2019/03/09/mp4/190309153658147087.mp4
     */

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_video_play)

        val mediaPathList = intent.getParcelableArrayListExtra<SelectMedia>(ViktorConstants.INTENT_DATA)

        if (DEBUG) {
            Log.d(TAG, "mediaPathList:$mediaPathList")
        }


        viktor_play.setOnViePlayCallback(object : ViktorVideoView.OnViePlayCallback {
            override fun onPrepare(totalMs: Long, width: Int, height: Int) {
                seek_bar.max = totalMs.toInt()
                tv_duration.post {
                    tv_duration.text =  ViktorUtil.formatTime(totalMs * 1000)
                    viktor_play.start()
                }
            }

            override fun onProgress(currentMs: Long) {
                if (isSeeking){
                    return
                }
                seek_bar.progress = currentMs.toInt()
                tv_current_duration.post {

                    val timeV =  ViktorUtil.formatTime(currentMs * 1000)
                    if (DEBUG) {
                        Log.d(TAG, "onProgress timeV:$timeV,currentMs:$currentMs")
                    }
                    tv_current_duration.text = timeV
                }
            }

            override fun onStateChanged(state: Int) {
                if (DEBUG) {
                    Log.d(TAG, "onStateChanged state:$state")
                }
                iv_video_btn.isSelected = state == ViktorVideoView.STATE_START
            }

        })

        iv_video_btn.setOnClickListener {
            iv_video_btn.isSelected = viktor_play.isPlaying()
            if (viktor_play.isPlaying()){
                viktor_play.pause()
            }else {
                viktor_play.start()
            }

        }

        seek_bar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                tv_current_duration.text = ViktorUtil.formatTime(progress * 1000L)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                isSeeking = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                viktor_play.seek(seek_bar.progress.toLong())
                viktor_play.start()

                isSeeking = false
            }

        })


        testViktor(mediaPathList)

    }

    private fun testViktor(mediaPathList: java.util.ArrayList<SelectMedia>?) {

        if (mediaPathList.isNullOrEmpty()){
            return
        }

        val viktorActuator = ViktorActuator()
        val createTimeline = viktorActuator.createTimeline()
        val appendVideoTrack = createTimeline?.appendVideoTrack()

        mediaPathList.forEach { selectMedia->
            appendVideoTrack?.let {
                val path = selectMedia.path ?: return@forEach
                val durationMills = selectMedia.durationMills

//                it.appendClip(path, 0L, 2000 * 1000)
                it.appendClip(path, 0L, durationMills * 1000)
            }

        }

        createTimeline?.let {
            viktor_play.setViktorActuator(viktorActuator,it)
            viktorActuator.linkTimelineWithWindow(it)


        }


        viktor_play.postDelayed({
                             viktor_play?.start()
        },1000)

    }

    override fun onResume() {
        super.onResume()
        viktor_play.resume()
        iv_video_btn.isSelected = viktor_play.isPlaying()
    }

    override fun onPause() {
        super.onPause()
        viktor_play?.pause()
        iv_video_btn?.isSelected = viktor_play.isPlaying()

        if (isFinishing) {
            viktor_play.release()
        }

    }
}