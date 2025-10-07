package com.example.edgedetectionapp

import android.app.Application
import android.util.Log
import org.opencv.android.OpenCVLoader

class MainApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        // Initialize OpenCV
        if (!OpenCVLoader.initDebug()) {
            Log.e("MainApplication", "OpenCV initialization failed")
        } else {
            Log.d("MainApplication", "OpenCV and native libraries loaded successfully")
        }
    }
}
