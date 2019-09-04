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

SET(SRC_list
    base_allocator.cpp
    decoder_element.cpp
    element_common.cpp
    element_factory.cpp
    encoder_element.cpp
    hevc_encoder_element.cpp
    general_allocator.cpp
    msdk_codec.cpp
    msdk_xcoder.cpp
    picture_decode_plugin.cpp
    picture_decoder_element.cpp
    string_decode_plugin.cpp
    string_decoder_element.cpp
    vpp_element.cpp
    yuv_reader_element.cpp
    yuv_reader_plugin.cpp
    yuv_writer_element.cpp
    yuv_writer_plugin.cpp
)

if ( Linux )
    set( GFX_SRCS
        vaapi_allocator.cpp
        vaapi_device.cpp
        vaapi_utils.cpp
        vaapi_utils_drm.cpp
        vaapi_utils_x11.cpp
    )
elseif ( Windows )
    set( GFX_SRCS
        d3d_allocator.cpp
        d3d_device.cpp
        d3d11_allocator.cpp
        d3d11_device.cpp
        sysmem_allocator.cpp
        hw_device.h
    )
endif()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libxcode ../../log4cpp/include)

IF(LIBDISPATCH_ENABLE)
    MESSAGE(STATUS " video maro LIBDISPATCH_ENABLE is defined ")
    add_definitions(-DMFX_DISPATCHER_EXPOSED_PREFIX)
    LINK_DIRECTORIES(/opt/intel/mediasdk/lib/lin_x64)
ELSE()
    MESSAGE(STATUS " video maro LIBDISPATCH_ENABLE is not defined ")
    LINK_DIRECTORIES(/opt/intel/mediasdk/lib64)
ENDIF()

if( Linux )
    ADD_LIBRARY(xcodevideo SHARED ${SRC_list} ${GFX_SRCS})
else()
    ADD_LIBRARY(xcodevideo STATIC ${SRC_list} ${GFX_SRCS})
endif()
#SET_TARGET_PROPERTIES(xcodevideo PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#SET_TARGET_PROPERTIES(xcodevideo PROPERTIES VERSION 1.2 SOVERSION 1)

if ( Linux )
    IF(LIBDISPATCH_ENABLE)
        MESSAGE(STATUS "maro LIBDISPATCH_ENABLE is defined ")
        ADD_DEFINITIONS(-DMFX_DISPATCHER_EXPOSED_PREFIX)
        add_dependencies(xcodevideo xcodebase va va-drm va-x11 dispatch_shared mfx)
        TARGET_LINK_LIBRARIES(xcodevideo xcodebase va va-drm va-x11 dispatch_shared mfx)
    ELSE()
        MESSAGE(STATUS "maro LIBDISPATCH_ENABLE is not defined ")
        TARGET_LINK_LIBRARIES(xcodevideo xcodebase mfxhw64 va va-drm va-x11 )
        add_dependencies(xcodevideo xcodebase mfxhw64 va va-drm va-x11 )
    ENDIF()
    #ADD_DEFINITIONS(-DLIBVA_SUPPORT -DLIBVA_X11_SUPPORT)
    ADD_DEFINITIONS(-DENABLE_RAW_DECODE -DSUPPORT_INTERLACE -DSUPPORT_SMTA)
elseif( Windows )
    MESSAGE(STATUS "maro LIBDISPATCH_ENABLE is defined ")
    #add_dependencies(xcodevideo xcodebase )
    #TARGET_LINK_LIBRARIES(xcodevideo xcodebase
            #libmfx d3d9 dxva2 d3d11 dxgi psapi ws2_32)
            # dwmapi igfx_s3dcontrol comsuppw)
endif()

#INSTALL(TARGETS xcodevideo xcodevideo_static
#        LIBRARY DESTINATION lib
#        ARCHIVE DESTINATION lib)
#INSTALL(FILES xcodevideo.h DESTINATION include/xcodevideo)
