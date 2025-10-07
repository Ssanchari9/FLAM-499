#ifndef EDGE_DETECTION_GL_UTILS_H
#define EDGE_DETECTION_GL_UTILS_H

#include <jni.h>
#include <time.h>
#include <sys/time.h>

// Get current time in milliseconds
static long long getCurrentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Check for OpenGL errors
static void checkGlError(const char* operation) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        __android_log_print(ANDROID_LOG_ERROR, "GLUtils", "%s: glError 0x%x\n", operation, error);
    }
}

// Create a shader program from vertex and fragment shader source
static GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertexShader) {
        checkGlError("glCreateShader vertex");
        return 0;
    }
    
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    GLint compiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = new char[infoLen];
            if (buf) {
                glGetShaderInfoLog(vertexShader, infoLen, NULL, buf);
                __android_log_print(ANDROID_LOG_ERROR, "GLUtils", "Could not compile vertex shader: %s", buf);
                delete[] buf;
            }
        }
        glDeleteShader(vertexShader);
        return 0;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragmentShader) {
        checkGlError("glCreateShader fragment");
        return 0;
    }
    
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = new char[infoLen];
            if (buf) {
                glGetShaderInfoLog(fragmentShader, infoLen, NULL, buf);
                __android_log_print(ANDROID_LOG_ERROR, "GLUtils", "Could not compile fragment shader: %s", buf);
                delete[] buf;
            }
        }
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        return 0;
    }
    
    GLuint program = glCreateProgram();
    if (!program) {
        checkGlError("glCreateProgram");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }
    
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
                __android_log_print(ANDROID_LOG_ERROR, "GLUtils", "Could not link program: %s", buf);
                delete[] buf;
            }
        }
        glDeleteProgram(program);
        program = 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

#endif //EDGE_DETECTION_GL_UTILS_H
