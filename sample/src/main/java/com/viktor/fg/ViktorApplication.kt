package com.viktor.fg

import android.annotation.SuppressLint
import android.app.Application
import android.content.Context

class ViktorApplication: Application() {

    companion object {
        @SuppressLint("StaticFieldLeak")
        @JvmStatic
        lateinit var context: Context
            private set
    }

    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        context = this
    }
}