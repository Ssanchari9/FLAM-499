package com.example.edgedetectionapp.view

import android.content.Context
import android.graphics.SurfaceTexture
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.TextureView
import com.example.edgedetectionapp.native.NativeLib

class GLTextureView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : TextureView(context, attrs, defStyleAttr), TextureView.SurfaceTextureListener {

    private var renderer: GLRenderer? = null

    init {
        surfaceTextureListener = this
    }

    fun setRenderer(renderer: GLRenderer) {
        this.renderer = renderer
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        Log.d("GLTextureView", "Surface created: $width x $height")
        NativeLib.initGL()
        renderer?.onSurfaceCreated()
        renderer?.onSurfaceChanged(width, height)
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        Log.d("GLTextureView", "Surface size changed: $width x $height")
        renderer?.onSurfaceChanged(width, height)
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        Log.d("GLTextureView", "Surface destroyed")
        renderer?.onSurfaceDestroyed()
        return true
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {
        // Called whenever the surface's SurfaceTexture is updated
    }

    interface GLRenderer {
        fun onSurfaceCreated()
        fun onSurfaceChanged(width: Int, height: Int)
        fun onDrawFrame()
        fun onSurfaceDestroyed()
    }
}
