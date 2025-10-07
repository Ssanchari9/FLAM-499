package com.example.edgedetectionapp.view

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import com.example.edgedetectionapp.nativebridge.NativeLib

class GLRenderSurface @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : GLSurfaceView(context, attrs), GLSurfaceView.Renderer {

    init {
        setEGLContextClientVersion(2)
        setRenderer(this)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    override fun onSurfaceCreated(gl: javax.microedition.khronos.opengles.GL10?, config: javax.microedition.khronos.egl.EGLConfig?) {
        NativeLib.initGL()
    }

    override fun onSurfaceChanged(gl: javax.microedition.khronos.opengles.GL10?, width: Int, height: Int) {
        // Viewport setup can be done natively if needed
    }

    override fun onDrawFrame(gl: javax.microedition.khronos.opengles.GL10?) {
        NativeLib.renderFrame()
    }
}
