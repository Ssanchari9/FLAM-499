#include <jni.h>
#include <android/log.h>
#include <opencv2/opencv.hpp>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cstring>
#include "gl_utils.h"

#define LOG_TAG "EdgeDetector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global variables
static cv::Mat inputFrame;
static cv::Mat processedFrame;
static GLuint textureId = 0;
static int renderMode = 0; // 0 = camera, 1 = edge detection
static float fps = 0.0f;
static int frameCount = 0;
static long long lastFpsUpdate = 0;

// Shader sources
const char* vertexShaderSource = R"(
    attribute vec4 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = aPosition;
        vTexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSource = R"(
    precision mediump float;
    uniform sampler2D uTexture;
    varying vec2 vTexCoord;
    void main() {
        gl_FragColor = texture2D(uTexture, vTexCoord);
    }
)";

// Vertex data for a full-screen quad
static const GLfloat quadVertices[] = {
    // Positions   // Texture coords
    -1.0f, -1.0f,  0.0f, 1.0f,  // Bottom left
     1.0f, -1.0f,  1.0f, 1.0f,  // Bottom right
    -1.0f,  1.0f,  0.0f, 0.0f,  // Top left
     1.0f,  1.0f,  1.0f, 0.0f   // Top right
};

// Shader program and attribute/uniform locations
static GLuint program = 0;
static GLuint aPosition = 0;
static GLuint aTexCoord = 0;
static GLuint uTexture = 0;

// Function to compile shader
GLuint loadShader(GLenum type, const char* shaderSource) {
    GLuint shader = glCreateShader(type);
    if (shader) {
        glShaderSource(shader, 1, &shaderSource, NULL);
        glCompileShader(shader);
        
        // Check compilation status
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = new char[infoLen];
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d: %s", type, buf);
                    delete[] buf;
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

// Function to create shader program
bool createProgram() {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (!vertexShader) {
        return false;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!fragmentShader) {
        return false;
    }

    program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = new char[bufLength];
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program: %s", buf);
                    delete[] buf;
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }

    if (program == 0) {
        LOGE("Failed to create program");
        return false;
    }

    // Get attribute and uniform locations
    aPosition = glGetAttribLocation(program, "aPosition");
    aTexCoord = glGetAttribLocation(program, "aTexCoord");
    uTexture = glGetUniformLocation(program, "uTexture");

    return true;
}

// Function to process frame with OpenCV
void processFrame() {
    if (inputFrame.empty()) {
        return;
    }

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(inputFrame, gray, cv::COLOR_YUV2GRAY_NV21);

    // Apply edge detection
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    // Convert back to RGB for display
    cv::cvtColor(edges, processedFrame, cv::COLOR_GRAY2RGBA);
}

// JNI functions
extern "C" {

JNIEXPORT void JNICALL
Java_com_example_edgedetectionapp_nativebridge_NativeLib_initGL(JNIEnv *env, jobject /* this */) {
    LOGI("Initializing OpenGL");
    
    // Create shader program
    if (!createProgram()) {
        LOGE("Failed to create shader program");
        return;
    }

    // Generate texture
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Set up vertex data and attribute pointers
    glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), quadVertices);
    glEnableVertexAttribArray(aPosition);
    glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), quadVertices + 2);
    glEnableVertexAttribArray(aTexCoord);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    LOGI("OpenGL initialized");
}

JNIEXPORT void JNICALL
Java_com_example_edgedetectionapp_nativebridge_NativeLib_renderFrame(JNIEnv *env, jobject /* this */) {
    if (program == 0) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    
    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(uTexture, 0);
    
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

JNIEXPORT jint JNICALL
Java_com_example_edgedetectionapp_nativebridge_NativeLib_processFrame(
        JNIEnv *env, jobject /* this */,
        jbyteArray frameData, jint width, jint height, jint rotation, jboolean isYUV) {
    
    if (frameData == nullptr) {
        LOGE("Frame data is null");
        return -1;
    }

    try {
        // Get frame data
        jbyte *data = env->GetByteArrayElements(frameData, nullptr);
        if (data == nullptr) {
            LOGE("Failed to get frame data");
            return -1;
        }

        // Create or update input frame
        if (isYUV) {
            inputFrame = cv::Mat(height + height / 2, width, CV_8UC1, (unsigned char *) data);
        } else {
            inputFrame = cv::Mat(height, width, CV_8UC4, (unsigned char *) data);
        }

        // Process frame if in edge detection mode
        if (renderMode == 1) {
            processFrame();
            
            // Update texture with processed frame
            if (!processedFrame.empty()) {
                glBindTexture(GL_TEXTURE_2D, textureId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, processedFrame.cols, processedFrame.rows, 
                            0, GL_RGBA, GL_UNSIGNED_BYTE, processedFrame.data);
            }
        } else {
            // Update texture with original frame
            cv::Mat rgba;
            cv::cvtColor(inputFrame, rgba, cv::COLOR_YUV2RGBA_NV21);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 
                        0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);
        }

        // Update FPS counter
        frameCount++;
        long long now = getCurrentTimeMillis();
        if (now - lastFpsUpdate >= 1000) {
            fps = frameCount * 1000.0f / (now - lastFpsUpdate);
            frameCount = 0;
            lastFpsUpdate = now;
            LOGI("FPS: %.2f", fps);
        }

        env->ReleaseByteArrayElements(frameData, data, JNI_ABORT);
        return 0;
    } catch (const cv::Exception &e) {
        LOGE("OpenCV error: %s", e.what());
        return -1;
    } catch (...) {
        LOGE("Unknown error in processFrame");
        return -1;
    }
}

JNIEXPORT void JNICALL
Java_com_example_edgedetectionapp_nativebridge_NativeLib_setRenderMode(
        JNIEnv *env, jobject /* this */, jint mode) {
    renderMode = mode;
}

JNIEXPORT jfloat JNICALL
Java_com_example_edgedetectionapp_nativebridge_NativeLib_getFps(JNIEnv *env, jobject /* this */) {
    return fps;
}

} // extern "C"
