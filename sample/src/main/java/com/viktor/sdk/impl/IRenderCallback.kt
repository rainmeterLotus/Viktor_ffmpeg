package com.viktor.sdk.impl

import android.view.SurfaceHolder

interface IRenderCallback {
    fun onSurfaceCreated(holder: SurfaceHolder?)

    fun onSurfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int)

    fun onSurfaceDestroyed(holder: SurfaceHolder?)
}