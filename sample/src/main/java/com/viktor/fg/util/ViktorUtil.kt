package com.viktor.fg.util

import android.content.ContentUris
import android.content.Context
import android.database.Cursor
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.DocumentsContract
import android.provider.MediaStore

object ViktorUtil {

    /**
     * @param ms 微秒
     */
    fun formatTime(ms: Long): String {
        val allSeconds = (ms / 1000000f).toInt()
        val seconds = allSeconds % 60
        val minutes = (allSeconds / 60) % 60
        val hours = allSeconds / 3600

        var result = ""
        if (hours > 0) {
            //hour
            result += if (hours <= 9) "0".plus(hours) else hours
            //minute
            result += if (minutes <= 9) ":".plus("0").plus(minutes) else ":".plus(minutes)
        } else {
            //minute
            result += if (minutes <= 9) "0".plus(minutes) else minutes
        }

        //second
        result += if (seconds <= 9) ":".plus("0").plus(seconds) else ":".plus(seconds)
        return result
    }


    fun obtainPathByUri(context: Context, uri: Uri?): String? {
        uri ?: return null
        if ("file".equals(uri.scheme, ignoreCase = true)) {
            return uri.path
        }else if ("content".equals(uri.scheme, ignoreCase = true)) {
            return getColumnInfo(context, uri, null, null)
        } else if (uri.toString().startsWith("/data") && uri.toString().contains(context.packageName)) {
            return uri.path
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT && DocumentsContract.isDocumentUri(context, uri)) {
            if ("com.android.providers.media.documents" == uri.authority) {
                val docId = DocumentsContract.getDocumentId(uri)
                val split = docId.split(":").toTypedArray()
                val type = split[0]
                var contentUri: Uri? = null
                when (type) {
                    "image" -> {
                        contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                    }
                    "video" -> {
                        contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                    }
                    "audio" -> {
                        contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                    }
                }
                val selection = "_id=?"
                val selectionArgs = arrayOf(
                    split[1]
                )
                return getColumnInfo(context, contentUri, selection, selectionArgs)
            } else if ("com.android.externalstorage.documents" == uri.authority) {
                val docId = DocumentsContract.getDocumentId(uri)
                val split = docId.split(":").toTypedArray()
                val type = split[0]
                if ("primary".equals(type, ignoreCase = true)) {
                    return Environment.getExternalStorageDirectory().toString() + "/" + split[1]
                }
            } else if ("com.android.providers.downloads.documents" == uri.authority) {
                val id = DocumentsContract.getDocumentId(uri)
                val contentUri = ContentUris.withAppendedId(
                    Uri.parse("content://downloads/public_downloads"), java.lang.Long.valueOf(id)
                )
                return getColumnInfo(context, contentUri, null, null)
            }
        }
        return null
    }

    private fun getColumnInfo(context: Context, uri: Uri?, selection: String?, selectionArgs: Array<String>?): String? {
        uri ?: return null
        var cursor: Cursor? = null
        val column = "_data"
        val projection = arrayOf(column)
        try {
            cursor = context.contentResolver.query(uri, projection, selection, selectionArgs, null)
            if (cursor != null && cursor.moveToFirst()) {
                val columnIndex = cursor.getColumnIndexOrThrow(column)
                return cursor.getString(columnIndex)
            }
        } catch (e: Exception) {
        } finally {
            cursor?.close()
        }
        return null
    }

    @JvmStatic
    fun isImage(mimeType: String?): Boolean {
        return mimeType?.startsWith("image") ?: false
    }

    @JvmStatic
    fun isVideo(mimeType: String?): Boolean {
        return mimeType?.startsWith("video") ?: false
    }

    @JvmStatic
    fun isGif(mimeType: String?): Boolean {
        return if (mimeType == null) false else mimeType == "image/gif"
    }

}