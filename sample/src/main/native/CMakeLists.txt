cmake_minimum_required(VERSION 3.4.1)

#引入ffmpeg头文件路径，例如在native-lib.cpp中
# #include <libavcodec/avcodec.h>，如果不引入会找不到
include_directories(${CMAKE_SOURCE_DIR}/ffmpeg_include)
include_directories(${CMAKE_SOURCE_DIR}/sound_touch/include)
include_directories(${CMAKE_SOURCE_DIR}/libexif)
#include_directories(${CMAKE_SOURCE_DIR}/lippng-1.6.37)

# ffmpeg库so所在的路径，根据支持的不同abi来设置so库路径
set(ffmpeg_so_dir ${CMAKE_SOURCE_DIR}/ffmpeg_lib/${ANDROID_ABI})

# 添加一个库，以导入的形式添加
add_library(avcodec SHARED IMPORTED)
#设置这个库属性，例如库的所在的路径
#然后记得在target_link_libraries中添加，做链接
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_so_dir}/libavcodec.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_so_dir}/libavutil.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_so_dir}/libavformat.so)


add_library(swresample SHARED IMPORTED)
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_so_dir}/libswresample.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_so_dir}/libswscale.so)


add_library(exif SHARED IMPORTED)
set_target_properties(exif
        PROPERTIES IMPORTED_LOCATION
        ${ffmpeg_so_dir}/libexif.so)


add_library(png STATIC
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/png.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngerror.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngget.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngmem.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngpread.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngread.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngrio.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngrtran.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngrutil.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngset.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngtrans.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngwio.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngwrite.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngwtran.c
        ${CMAKE_SOURCE_DIR}/libpng-1.6.37/pngwutil.c
        )
set_target_properties(png
        PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
        ffmpeg_so_dir)
#set_target_properties(png PROPERTIES VERSION 1.0.0 OUTPUT_NAME "libpng" SUFFIX ".so")

target_link_libraries(
        png
        z
)


add_library(viktor_ffmpeg SHARED

        ${CMAKE_SOURCE_DIR}/all/vk_all.cpp
        ${CMAKE_SOURCE_DIR}/all/viktor_matrix.cpp
        ${CMAKE_SOURCE_DIR}/all/filter/viktor_base_filter.cpp
        ${CMAKE_SOURCE_DIR}/all/filter/viktor_rotate_filter.cpp
        ${CMAKE_SOURCE_DIR}/all/filter/viktor_filter_color.cpp

        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/AAFilter.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/BPMDetect.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/cpu_detect_x86.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/FIFOSampleBuffer.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/FIRFilter.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/InterpolateCubic.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/InterpolateLinear.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/InterpolateShannon.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/mmx_optimized.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/PeakFinder.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/RateTransposer.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/SoundTouch.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/sse_optimized.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundTouch/TDStretch.cpp
        ${CMAKE_SOURCE_DIR}/sound_touch/SoundStretch/WavFile.cpp

        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_frame_queue.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_pkt_queue.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_sdl.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_struct.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_opensles.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/soundtouch_wrap.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_video_texture.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_egl.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_renderer.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_egl_renderer.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_egl_renderer_yuv420p.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/ffplay/ff_egl_renderer_rgb.cpp

        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/util/jni_help.c
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/util/ViktorCommon.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CTimeline.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CAudioTrack.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CVideoTrack.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CClip.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CVideoClip.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CAudioClip.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/clang/CTimelineWrapper.cpp

        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/native_viktor_actuator.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/native_timeline.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/native_viktor_audio_track.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/native_viktor_video_track.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/native_viktor_audio_clip.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/native_viktor_video_clip.cpp

        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/demux/ViktorDemux.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/context/viktor_context.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/context/viktor_frame_queue.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/context/viktor_packet_queue.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/context/viktor_sdl.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/decode/IViktorDecode.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/decode/ViktorDecodeWrapper.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/decode/ViktorAudioDecode.cpp
        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/decode/ViktorVideoDecode.cpp

        ${CMAKE_SOURCE_DIR}/viktor_sdk/timeline/display/ViktorVideoDisplay.cpp
        )
set_target_properties(viktor_ffmpeg PROPERTIES VERSION 1.0.0 OUTPUT_NAME "libviktor" PREFIX ""SUFFIX ".so")

find_library(log-lib
        log)
find_library(android-lib
        android)

target_link_libraries(
        viktor_ffmpeg

        # 连接 FFmpeg 相关的库
        avcodec
        avutil
        avformat
        swresample
        swscale
        exif
        png

        EGL
        GLESv2
        OpenSLES
        ${log-lib}
        ${android-lib}
)
