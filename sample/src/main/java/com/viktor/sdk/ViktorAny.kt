package com.viktor.sdk

open class ViktorAny {
    companion object{
        init {
            System.loadLibrary("viktor")
        }
    }
    @Volatile
    protected var objectAddress: Long = 0


    fun setObjAddress(objectAddress:Long){
        this.objectAddress = objectAddress
    }

    fun getObjAddress() = objectAddress

}