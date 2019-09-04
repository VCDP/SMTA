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

if( Linux )
  set( os_arch "lin" )
elseif( Darwin )
  set( os_arch "darwin" )
endif()
if( __ARCH MATCHES ia32)
  set( os_arch "${os_arch}_ia32" )
else( )
  set( os_arch "${os_arch}_x64" )
endif( )

if( Linux OR Darwin )
  if(CMAKE_MFX_HOME)
    set( MFX_API_HOME ${CMAKE_MFX_HOME} )
  else()
    set( MFX_API_HOME $ENV{MFX_HOME} )
  endif()
  find_path( MFX_INCLUDE mfxdefs.h PATHS ${MFX_API_HOME} PATH_SUFFIXES include )
  find_library ( MFX_LIBRARY libmfx.a PATHS ${MFX_API_HOME}/lib PATH_SUFFIXES ${os_arch} )

  # required:
  include_directories( ${MFX_API_HOME}/include )
  link_directories( ${MFX_API_HOME}/lib/${os_arch} )

elseif( Windows )
  if(CMAKE_MFX_HOME)
    set( MFX_API_HOME ${CMAKE_MFX_HOME} )
  else()
    set( MFX_API_HOME $ENV{MFX_HOME} )
  endif()
  set( os_arch "x64" )

  find_path( MFX_INCLUDE mfxdefs.h PATHS ${MFX_API_HOME} PATH_SUFFIXES include )
  find_library ( MFX_LIBRARY libmfx.lib PATHS ${MFX_API_HOME}/lib PATH_SUFFIXES ${os_arch} )

  # required:
  include_directories( ${MFX_API_HOME}/include )
  link_directories( ${MFX_API_HOME}/lib/${os_arch} )

else( )
  set( MFX_INCLUDE NOTFOUND )
  set( MFX_LIBRARY NOTFOUND )

endif( )

if(NOT MFX_INCLUDE MATCHES NOTFOUND)
  set( MFX_FOUND TRUE )
  include_directories( ${MFX_INCLUDE} )
endif( )

if(NOT DEFINED MFX_FOUND)
  message( FATAL_ERROR "Intel(R) Media SDK was not found in ${MFX_API_HOME} (${MFX_INCLUDE}, ${MFX_LIBRARY} required)! Set/check MFX_HOME environment variable!")
else ( )
  message( STATUS "Intel(R) Media SDK ${MFX_INCLUDE}, ${MFX_LIBRARY} was found here ${MFX_API_HOME}")
endif( )

if(NOT MFX_LIBRARY MATCHES NOTFOUND)
  get_filename_component(MFX_LIBRARY_PATH ${MFX_LIBRARY} PATH )
  message( STATUS "Intel(R) Media SDK ${MFX_LIBRARY_PATH} will be used")
  link_directories( ${MFX_LIBRARY_PATH} )
endif( )

if( Linux )
  set(MFX_LDFLAGS "-Wl,--default-symver" )
endif( )

if ( Windows )
 if(CMAKE_MFX_HOME)
    set( MFX_API_HOME ${CMAKE_MFX_HOME} )
  else()
    set( MFX_API_HOME $ENV{MFX_HOME} )
  endif()
  set( os_arch "x64" )
  set( IGFX_LIB_HOME ${MFX_API_HOME}/igfx_s3dcontrol)

  find_path( IGFX_INCLUDE igfx_s3dcontrol.h PATHS ${IGFX_LIB_HOME} PATH_SUFFIXES include )
  find_library ( IGFX_LIBRARY igfx_s3dcontrol.lib PATHS ${IGFX_LIB_HOME}/lib PATH_SUFFIXES ${os_arch} )

  # required:
  include_directories( ${IGFX_LIB_HOME}/include )
  link_directories( ${IGFX_LIB_HOME}/lib/${os_arch} )
endif()
