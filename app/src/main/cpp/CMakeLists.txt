cmake_minimum_required(VERSION 3.22.1)

# OpenCV_DIR is provided via Gradle arguments. Example:
# -DOpenCV_DIR=${rootDir}/OpenCV-android-sdk/sdk/native/jni
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# Add OpenGL ES library
find_library( log-lib log )
find_library( android-lib android )
find_library( egl-lib EGL )
find_library( glesv2-lib GLESv2 )

# Set the path to the source files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -frtti -fexceptions -D__STDC_CONSTANT_MACROS")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libatomic.a")

# Add the native library
add_library(
        edge-detection
        SHARED
        edge_detector.cpp
)

# Include directories
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/main/cpp/include
)

# Link libraries
target_link_libraries(
        edge-detection
        ${log-lib}
        ${android-lib}
        ${egl-lib}
        ${glesv2-lib}
        ${OpenCV_LIBS}
)
