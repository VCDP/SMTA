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
    app_util.cpp
    base_element.h
    base_element.cpp
    debug_info.cpp
    dispatcher.cpp
    fast_copy.cpp
    log_function.cpp
    measurement.h
    measurement.cpp
    media_pad.cpp
    mem_pool.cpp
    partially_linear_bitrate.cpp
    stream.cpp
    trace.cpp
    trace_file.cpp
    trace_filter.cpp
    trace_pool.h
    trace_pool.cpp
)

INCLUDE_DIRECTORIES(../../log4cpp/include
                ${CMAKE_SOURCE_DIR}/libxcode
)

if( Linux )
	ADD_LIBRARY(xcodebase SHARED ${SRC_list})
	add_dependencies(xcodebase pthread)
else()
	ADD_LIBRARY(xcodebase STATIC ${SRC_list})
endif()
add_dependencies(xcodebase xcodeos)
#SET_TARGET_PROPERTIES(xcodebase PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#SET_TARGET_PROPERTIES(xcodebase PROPERTIES VERSION 1.2 SOVERSION 1)

#INSTALL(TARGETS xcodebase xcodebase_static
#        LIBRARY DESTINATION lib
#        ARCHIVE DESTINATION lib)
#INSTALL(FILES myhello.h DESTINATION include/myhello)
