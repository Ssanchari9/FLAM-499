# ProGuard rules for EdgeDetectionApp
# Keep JNI interfaces
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep OpenCV classes if using Java wrapper
-keep class org.opencv.** { *; }
-dontwarn org.opencv.**
