package com.viktor.sdk.impl

interface IViktorCallback {
    fun onPrepare(totalUs:Long, width:Int, height:Int)
    fun onCurrentUs(currentUs:Long)
    fun onStateChanged(state:Int)
}