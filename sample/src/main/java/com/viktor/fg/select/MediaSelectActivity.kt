package com.viktor.fg.select

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.database.Cursor
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import com.viktor.fg.BuildConfig
import com.viktor.fg.R
import com.viktor.fg.adapter.AlbumsAdapter
import com.viktor.fg.loader.*
import com.viktor.fg.ui.AlbumsCategorySpinner
import com.viktor.fg.util.ViktorConstants
import kotlinx.android.synthetic.main.activity_media_select.*

class MediaSelectActivity :
    AppCompatActivity(){

    companion object {
        const val TAG = "media_select_a"
        var DEBUG = BuildConfig.DEBUG

        const val REQUEST_CODE_STORAGE = 1000
        const val REQUEST_CODE_ALBUM = 1001

        fun startActivityResult(context: Activity){
            val intent = Intent(context, MediaSelectActivity::class.java)
            context.startActivityForResult(intent,REQUEST_CODE_ALBUM)
        }

    }


    /**
     * 相册分类加载器
     */
    private val mAlbumCategory = AlbumCategoryManager()

    /**
     * 相册分类选择器
     */
    private var mAlbumsCategorySpinner: AlbumsCategorySpinner? = null
    //相册分类adapter
    private val mAlbumCategoryAdapter: AlbumsAdapter by lazy { AlbumsAdapter(this, null, false) }

    //选中的素材
    private val selectMediaList = ArrayList<SelectMedia>()

    //相册分类callBack
    private val mAlbumCategoryCallback: AlbumCategoryManager.AlbumCategoryCallbacks =
        object : AlbumCategoryManager.AlbumCategoryCallbacks {
            override fun onAlbumCagetoryLoad(cursor: Cursor?) {
                mAlbumCategoryAdapter.swapCursor(cursor)

                tv_album_category?.post {
                    cursor?.moveToPosition(mAlbumCategory.curPos)
                    mAlbumsCategorySpinner?.setSelection(this@MediaSelectActivity, mAlbumCategory.curPos)
                    val album = AlbumCategory.valueOf(cursor, mCurrentMediaType) ?: return@post

                    onAlbumCategorySelected(album)
                }
            }

            override fun onAlbumCategoryReset() {
                mAlbumCategoryAdapter.swapCursor(null)
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_media_select)
        initBundleData()
    }

    override fun onPause() {
        super.onPause()
        if (isFinishing) {
            mAlbumCategory.release()
        }
    }

    private fun initBundleData() {
        checkPermission()

        tv_tab_video.setOnClickListener {
            updateMediaType(ViktorConstants.TYPE_VIDEO)
        }

        tv_tab_img.setOnClickListener {
            updateMediaType(ViktorConstants.TYPE_PHOTO)
        }

        mAlbumsCategorySpinner = AlbumsCategorySpinner(this).apply {
            setOnItemSelectedListener(object :AdapterView.OnItemSelectedListener{
                override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                    mAlbumCategory.setStateCurrentSelection(position)
                    mAlbumCategoryAdapter.cursor.moveToPosition(position)
                    val album = AlbumCategory.valueOf(mAlbumCategoryAdapter.cursor, mCurrentMediaType) ?: return
                    onAlbumCategorySelected(album)
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {
                }

            })
            setAdapterAndAnchorView(mAlbumCategoryAdapter,cons_top)
            onSelectedView(tv_album_category)
        }

        iv_back.setOnClickListener { finish() }


        tv_done?.setOnClickListener {
            if (!it.isSelected){
                return@setOnClickListener
            }

            val intent = Intent()
            intent.putParcelableArrayListExtra(ViktorConstants.INTENT_DATA, selectMediaList)
            setResult(RESULT_OK, intent)
            finish()
        }
    }

    /**
     * 选中某个文件夹
     */
    private fun onAlbumCategorySelected(album: AlbumCategory) {
        fl_container?.visibility = View.VISIBLE


        val fragment: Fragment =
            MediaItemFragment.newInstance(album, mCurrentMediaType, selectMediaList).apply {
                onActivityMediaClick = {path,isAdd ->
                    dealWithMediaSelect(path,isAdd)
                }
            }
        supportFragmentManager.apply {
            beginTransaction()
                .replace(
                    R.id.fl_container,
                    fragment,
                    MediaItemFragment::class.java.simpleName
                )
                .commitAllowingStateLoss()
        }
    }

    private fun dealWithMediaSelect(selectMedia:SelectMedia,isAdd:Boolean){
        if (isAdd){
            selectMediaList.add(selectMedia)
        } else {
            selectMediaList.remove(selectMedia)
        }
        tv_done?.isSelected = !selectMediaList.isNullOrEmpty()
    }

    private var mCurrentMediaType: Int = ViktorConstants.TYPE_VIDEO

    private fun updateMediaType(type: Int) {
        val album = try {
            mAlbumCategoryAdapter.cursor.moveToPosition(mAlbumsCategorySpinner?.curPosition ?: 0)

            AlbumCategory.valueOf(mAlbumCategoryAdapter.cursor, type)
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
        if (mCurrentMediaType == type || album == null) return
        

        mCurrentMediaType = type

        mAlbumsCategorySpinner?.setCurrentType(type)
        mAlbumCategoryAdapter.setCurrentType(type)

        if (BuildConfig.DEBUG) {
            Log.d(TAG, "updateMediaType getDisplayName -> ${album.getDisplayName(this)}")
        }

        when (type) {
            ViktorConstants.TYPE_VIDEO -> {
                view_in_video.visibility = View.VISIBLE
                tv_tab_video.alpha = 1.0f
                view_in_img.visibility = View.GONE
                tv_tab_img.alpha = 0.4f

                onAlbumCategorySelected(album)
            }
            ViktorConstants.TYPE_PHOTO -> {
                view_in_video.visibility = View.GONE
                tv_tab_video.alpha = 0.4f
                view_in_img.visibility = View.VISIBLE
                tv_tab_img.alpha = 1.0f

                onAlbumCategorySelected(album)
            }
        }
    }

    private fun checkPermission() {
        if (ContextCompat.checkSelfPermission(
                this@MediaSelectActivity, Manifest.permission.READ_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED
        ) {
            mAlbumCategory.init(this@MediaSelectActivity, mAlbumCategoryCallback)
            mAlbumCategory.loadAlbums()
        } else {
            ActivityCompat.requestPermissions(this@MediaSelectActivity, arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), REQUEST_CODE_STORAGE)
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            REQUEST_CODE_STORAGE -> {
                if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    mAlbumCategory.init(this@MediaSelectActivity, mAlbumCategoryCallback)
                    mAlbumCategory.loadAlbums()
                }
            }
        }
    }
}