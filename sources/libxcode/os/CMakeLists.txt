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

set(SRC_list
    platform.h
    atomic.h
    atomic.cpp
    XC_Thread.h
    XC_Thread.cpp
    XC_Event.h
    XC_Event.cpp
    mutex.h
    mutex.cpp
    XC_Time.h
    XC_Time.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/libxcode)

add_library(xcodeos STATIC ${SRC_list})
