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

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set( CMAKE_VERBOSE_MAKEFILE             TRUE )

# the following options should disable rpath in both build and install cases
set( CMAKE_INSTALL_RPATH "" )
set( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )
set( CMAKE_SKIP_BUILD_RPATH TRUE )
set( LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/dist)

collect_oses( )
collect_arch( )

if( Windows )
  #message( FATAL_ERROR "Windows is not currently supported!" )

else( )
  add_definitions(-DUNIX)

  if( Linux )
    add_definitions(-D__USE_LARGEFILE64 -D_FILE_OFFSET_BITS=64)

    add_definitions(-DLINUX)
    add_definitions(-DLINUX32)

    if(__ARCH MATCHES intel64)
      add_definitions(-DLINUX64)
    endif( )
  endif( )

  if( Darwin )
    add_definitions(-DOSX)
    add_definitions(-DOSX32)

    if(__ARCH MATCHES intel64)
      add_definitions(-DOSX64)
    endif( )
  endif( )

  if( NOT DEFINED ENV{MFX_VERSION} )
    set( version 0.0.000.0000 )
  else( )
    set( version $ENV{MFX_VERSION} )
  endif( )

  if( Linux OR Darwin )
    execute_process(
      COMMAND echo
      COMMAND cut -f 1 -d.
      COMMAND date "+.%-y.%-m.%-d"
      OUTPUT_VARIABLE cur_date
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    string( SUBSTRING ${version} 0 1 ver )

    add_definitions( -DMFX_FILE_VERSION=\"${ver}${cur_date}\")
    add_definitions( -DMFX_PRODUCT_VERSION=\"${version}\" )
  endif( )

  set(no_warnings "-Wno-unknown-pragmas -Wno-unused")
  
  set(CMAKE_C_FLAGS "-pipe -fPIC")
  set(CMAKE_CXX_FLAGS "-pipe -fPIC")
  append("-fPIE -pie" CMAKE_EXE_LINKER_FLAGS)

  set(CMAKE_C_FLAGS_DEBUG     "-O0 -Wall ${no_warnings} -g -D_DEBUG" CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS_RELEASE   "-O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all -Wall ${no_warnings} -DNDEBUG"    CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -Wall ${no_warnings} -g -D_DEBUG" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all -Wall ${no_warnings} -DNDEBUG"    CACHE STRING "" FORCE)

  if (DEFINED CMAKE_FIND_ROOT_PATH)
#    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " CMAKE_C_FLAGS)
#    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " CMAKE_CXX_FLAGS)
    append("--sysroot=${CMAKE_FIND_ROOT_PATH} " LINK_FLAGS)
  endif (DEFINED CMAKE_FIND_ROOT_PATH)

  # SW HEVC decoder & encoder require SSE4.2
  if (CMAKE_C_COMPILER MATCHES icc)
    append("-xSSE4.2 -static-intel" CMAKE_C_FLAGS)
  else()
    append("-msse4.2" CMAKE_C_FLAGS)
  endif()

  if (CMAKE_CXX_COMPILER MATCHES icpc)
    append("-xSSE4.2 -static-intel" CMAKE_CXX_FLAGS)
  else()
    append("-msse4.2" CMAKE_CXX_FLAGS)
  endif()

  if(__ARCH MATCHES ia32)
    append("-m32" CMAKE_C_FLAGS)
    append("-m32" CMAKE_CXX_FLAGS)
    append("-m32" LINK_FLAGS)
  else ( )
    append("-m64" CMAKE_C_FLAGS)
    append("-m64" CMAKE_CXX_FLAGS)
    append("-m64" LINK_FLAGS)
  endif( )

  if(__ARCH MATCHES ia32)
    link_directories(/usr/lib)
  else ( )
    link_directories(/usr/lib64)
  endif( )
endif( )

# Some font definitions: colors, bold text, etc.
if(NOT Windows)
  string(ASCII 27 Esc)
  set(EndColor   "${Esc}[m")
  set(BoldColor  "${Esc}[1m")
  set(Red        "${Esc}[31m")
  set(BoldRed    "${Esc}[1;31m")
  set(Green      "${Esc}[32m")
  set(BoldGreen  "${Esc}[1;32m")
endif()

# Usage: report_targets( "Description for the following targets:" [targets] )
# Note: targets list is optional
function(report_targets description )
  message("")
  message("${ARGV0}")
  foreach(target ${ARGV1})
    message("  ${target}")
  endforeach()
  message("")
endfunction()

# Permits to accumulate strings in some variable for the delayed output
function(report_add_target var target)
  set(${ARGV0} ${${ARGV0}} ${ARGV1} CACHE INTERNAL "" FORCE)
endfunction()
