# 
#   Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
# 
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
# 
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
# 
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
# 
#   Contact Information:
#   Intel Corporation
# 
# 
#  version: SMTA.A.0.9.4-2

set(SRCS
    BitstreamSource.h
    BitstreamSink.h
    common_defs.h
    common_unix.h
    cmdProcessor.h
    cmdProcessor.cpp
    debug_stream.h
    ffmpeg_utils.h
    ffmpeg_utils.cpp
    FFmpegBitstream.h
    FFmpegBitstream.cpp
    FFmpegReader.cpp
    FFmpegReader.h
    FFmpegWriter.h
    FFmpegWriter.cpp
    InputParams.h
    Launcher.h
    Launcher.cpp
    log_flags.h
    LogPrintThread.h
    LogPrintThread.cpp
    sample_defs.h
    sample_utils.h
    sample_utils.cpp
    SinkWrap.h
    SinkWrap.cpp
    bitstream_mgr.hpp
    bitstream_mgr.cpp
    main.cpp
    )

include_directories(${PROJECT_SOURCE_DIR}/libxcode
    ${PROJECT_SOURCE_DIR}/dist/include)

link_directories(${PROJECT_SOURCE_DIR}/dist)
set( FFMPEG_LIBS avformat avcodec avutil)
set( MFX_LIBS libmfx dxva2 d3d9 d3d11 dxgi psapi)

add_executable(StreamingMediaTranscoder ${SRCS})

add_dependencies(StreamingMediaTranscoder xcodevideo xcodebase xcodeos)
target_link_libraries(StreamingMediaTranscoder xcodevideo xcodebase xcodeos ${FFMPEG_LIBS}
    libmfx d3d9 dxva2 d3d11 dxgi psapi legacy_stdio_definitions ws2_32)
if( Linux )
    target_link_libraries(StreamingMediaTranscoder pthread)
endif()

set( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/dist )
install( TARGETS StreamingMediaTranscoder
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/dist)

add_test(NAME H264ToMpeg2
    COMMAND StreamingMediaTranscoder -params -name test -stats -i::h264 e:\\6.h264 -o::mpeg out.ts -w 1920 -h 1080 -f 29.97 -b 8000)
