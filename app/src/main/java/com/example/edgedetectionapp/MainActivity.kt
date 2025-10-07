package com.example.edgedetectionapp

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.Camera
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.example.edgedetectionapp.databinding.ActivityMainBinding
import com.example.edgedetectionapp.nativebridge.NativeLib
import com.example.edgedetectionapp.view.GLRenderSurface
import com.google.android.material.snackbar.Snackbar
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    private var camera: Camera? = null
    private lateinit var cameraExecutor: ExecutorService
    private var isProcessing = false
    private var lastFpsUpdate = 0L
    private var frameCount = 0

    // Camera and display configuration
    private var lensFacing = CameraSelector.LENS_FACING_BACK
    // Using GLSurfaceView for rendering; no CameraX Preview use case
    private var imageAnalysis: ImageAnalysis? = null
    private var cameraProvider: ProcessCameraProvider? = null

    // Rendering configuration
    private var surfaceRotation = 0
    private var isEdgeDetectionEnabled = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Request camera permissions
        if (allPermissionsGranted()) {
            startCamera()
        } else {
            ActivityCompat.requestPermissions(
                this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS
            )
        }

        // Set up the camera and OpenGL
        cameraExecutor = Executors.newSingleThreadExecutor()
        setupUI()
    }

    private fun setupUI() {
        binding.btnToggleMode.setOnClickListener {
            isEdgeDetectionEnabled = !isEdgeDetectionEnabled
            binding.btnToggleMode.setImageResource(
                if (isEdgeDetectionEnabled) R.drawable.ic_camera else R.drawable.ic_edge
            )
            NativeLib.setRenderMode(if (isEdgeDetectionEnabled) 1 else 0)
        }
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            try {
                cameraProvider = cameraProviderFuture.get()
                bindCameraUseCases()
            } catch (e: Exception) {
                Log.e(TAG, "Use case binding failed", e)
            }
        }, ContextCompat.getMainExecutor(this))
    }

    private fun bindCameraUseCases() {
        val cameraProvider = cameraProvider ?: return
        val cameraSelector = CameraSelector.Builder().requireLensFacing(lensFacing).build()

        // Image analysis
        imageAnalysis = ImageAnalysis.Builder()
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_YUV_420_888)
            .build()
            .also {
                it.setAnalyzer(cameraExecutor) { image ->
                    if (!isProcessing) {
                        isProcessing = true
                        processImage(image)
                        image.close()
                    }
                }
            }

        try {
            // Unbind use cases before rebinding
            cameraProvider.unbindAll()

            // Bind only ImageAnalysis use case; rendering is via GLSurfaceView
            camera = cameraProvider.bindToLifecycle(
                this, cameraSelector, imageAnalysis
            )
        } catch (exc: Exception) {
            Log.e(TAG, "Use case binding failed", exc)
        }
    }

    private fun processImage(image: ImageProxy) {
        try {
            val rotation = when (image.imageInfo.rotationDegrees) {
                0 -> 0
                90 -> 1
                180 -> 2
                270 -> 3
                else -> 0
            }

            val planes = image.planes
            val yBuffer = planes[0].buffer
            val uBuffer = planes[1].buffer
            val vBuffer = planes[2].buffer

            val ySize = yBuffer.remaining()
            val uSize = uBuffer.remaining()
            val vSize = vBuffer.remaining()

            val nv21 = ByteArray(ySize + uSize + vSize)
            yBuffer.get(nv21, 0, ySize)
            vBuffer.get(nv21, ySize, vSize)
            uBuffer.get(nv21, ySize + vSize, uSize)

            // Process frame in native code
            NativeLib.processFrame(nv21, image.width, image.height, rotation, true)

            // Update FPS counter
            updateFps()
        } catch (e: Exception) {
            Log.e(TAG, "Error processing image", e)
        } finally {
            isProcessing = false
        }
    }

    private fun updateFps() {
        frameCount++
        val now = System.currentTimeMillis()
        if (now - lastFpsUpdate >= 1000) {
            val fps = frameCount * 1000 / (now - lastFpsUpdate)
            runOnUiThread {
                binding.tvFps.text = String.format("FPS: %d", fps)
            }
            frameCount = 0
            lastFpsUpdate = now
        }
    }

    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(baseContext, it) == PackageManager.PERMISSION_GRANTED
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (allPermissionsGranted()) {
                startCamera()
            } else {
                Snackbar.make(
                    binding.root,
                    "Permissions not granted by the user.",
                    Snackbar.LENGTH_SHORT
                ).show()
                finish()
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
    }

    companion object {
        private const val TAG = "EdgeDetectionApp"
        private const val REQUEST_CODE_PERMISSIONS = 10
        private val REQUIRED_PERMISSIONS = arrayOf(Manifest.permission.CAMERA)
    }
}
