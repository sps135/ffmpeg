# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).

        src/main/cpp/ffmpeg/cmdutils.c
        src/main/cpp/ffmpeg/ffmpeg.c
        src/main/cpp/ffmpeg/ffmpeg_filter.c
        src/main/cpp/ffmpeg/ffmpeg_opt.c
        src/main/cpp/ffmpeg/ffmpeg_hw.c
        src/main/cpp/ffmpeg_cmd.c
        src/main/cpp/AVpacket_queue.c
        src/main/cpp/media_player.c)

add_library(vcodec
        SHARED
        IMPORTED )
set_target_properties(vcodec
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libavcodec.so )

add_library(vdevice
        SHARED
        IMPORTED )
set_target_properties(vdevice
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libavdevice.so )

add_library(vfilter
        SHARED
        IMPORTED )
set_target_properties(vfilter
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libavfilter.so )

add_library(vformat
        SHARED
        IMPORTED )
set_target_properties(vformat
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libavformat.so )

add_library(vutil
        SHARED
        IMPORTED )
set_target_properties(vutil
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libavutil.so )

add_library(swresample
        SHARED
        IMPORTED )
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libswresample.so )

add_library(swscale
        SHARED
        IMPORTED )
set_target_properties( swscale
        PROPERTIES IMPORTED_LOCATION
        ../../../../libs/armeabi-v7a/libswscale.so )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

include_directories(src/main/cpp)
include_directories(src/main/cpp/include)

find_library( # Sets the name of the path variable.
        log-lib
        log )

target_link_libraries( # Specifies the target library.
        native-lib
        vcodec
        vdevice
        vfilter
        vformat
        vutil
        swresample
        swscale
        -landroid #native_window
        -ljnigraphics #bitmap
        -lOpenSLES #openSLES
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )