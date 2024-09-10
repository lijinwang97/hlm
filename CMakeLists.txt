cmake_minimum_required(VERSION 3.10)

project(hlm)

set(CMAKE_CXX_STANDARD 17)

# 设置third_party的路径
set(THIRD_PARTY "${CMAKE_SOURCE_DIR}/trunk/third_party")

# 包含头文件
include_directories(${THIRD_PARTY}/ffmpeg/include)
include_directories(${THIRD_PARTY}/spdlog/include)
include_directories(${THIRD_PARTY}/tomlplusplus/include)
include_directories(${THIRD_PARTY}/crow/include)
include_directories(${THIRD_PARTY}/asio/asio/include)
include_directories(${CMAKE_SOURCE_DIR}/trunk/src)

# 链接库文件
link_directories(${THIRD_PARTY}/ffmpeg/lib)
link_directories(${THIRD_PARTY}/spdlog/lib)
link_directories(${THIRD_PARTY}/ffmpeg/x264)

# 添加可执行文件
add_executable(hlm 
    trunk/src/main/hlm_main_server.cc
    trunk/src/app/hlm_http_server.cc
    trunk/src/core/hlm_task.cc
    trunk/src/core/hlm_screenshot_strategy.cc
    # trunk/src/core/hlm_decoder.cc
    # trunk/src/core/hlm_encoder.cc
    trunk/src/utils/hlm_logger.cc
    trunk/src/utils/hlm_config.cc
    trunk/src/utils/hlm_design_patterns.cc
)

# 添加单元测试的可执行文件
# add_executable(hlm_utest_http 
#     trunk/src/utest/hlm_utest_http.cc
#     trunk/src/app/hlm_http_server.cc
#     trunk/src/utils/hlm_logger.cc
#     trunk/src/utils/hlm_config.cc
#     trunk/src/utils/hlm_design_patterns.cc
# )

# 链接FFmpeg库
target_link_libraries(hlm
    avformat
    avcodec
    avutil
    avfilter
    swscale
    swresample
    postproc
    ssl
    crypto
    pthread
    z
    rt
    x264
    x265
    bz2
    dl
    mp3lame
    opus
    vorbis
    vorbisenc
    vpx
    fdk-aac
    va
    va-drm
    va-x11
    lzma
    vdpau
    X11
    spdlog
)

# target_link_libraries(hlm_utest_http
#     avformat
#     avcodec
#     avutil
#     avfilter
#     swscale
#     swresample
#     postproc
#     ssl
#     crypto
#     pthread
#     z
#     x264
#     x265
#     bz2
#     dl
#     mp3lame
#     opus
#     vorbis
#     vorbisenc
#     vpx
#     fdk-aac
#     va
#     va-drm
#     va-x11
#     lzma
#     vdpau
#     X11
#     spdlog
# )