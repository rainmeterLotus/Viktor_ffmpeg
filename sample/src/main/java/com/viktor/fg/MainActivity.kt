package com.viktor.fg

import android.app.Activity
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import com.viktor.fg.select.MediaSelectActivity
import com.viktor.fg.util.ViktorConstants
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        btn_play?.setOnClickListener {
            MediaSelectActivity.startActivityResult(this)
        }
    }


    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == MediaSelectActivity.REQUEST_CODE_ALBUM && resultCode == Activity.RESULT_OK){
            val mediaList = data?.getStringArrayListExtra(ViktorConstants.INTENT_DATA)
            mediaList?.forEach { path->
                Log.d("rain", "path:$path")
            }

            VideoPlayActivity.startActivity(this,mediaList)
        }
    }

}