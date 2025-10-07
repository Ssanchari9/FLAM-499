package com.example.edgedetectionapp.nativebridge

import android.graphics.Bitmap

/**
 * Native interface for edge detection and OpenGL rendering
 */
object NativeLib {
    init {
        System.loadLibrary("edge-detection")
    }

    // Native methods
    external fun initGL()
    external fun renderFrame()
    external fun processFrame(
        input: ByteArray,
        width: Int,
        height: Int,
        rotation: Int,
        isYUV: Boolean
    ): Int

    external fun setRenderMode(mode: Int)
    external fun getFps(): Float
}
