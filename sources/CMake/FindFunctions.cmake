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

set( CMAKE_LIB_DIR ${CMAKE_BINARY_DIR}/__lib )
set( CMAKE_BIN_DIR ${CMAKE_BINARY_DIR}/__bin )

function( collect_arch )
  if(__ARCH MATCHES ia32)
    set( ia32 true PARENT_SCOPE )
    set( CMAKE_OSX_ARCHITECTURES i386 PARENT_SCOPE )
  else ( )
    set( intel64 true PARENT_SCOPE )
    set( CMAKE_OSX_ARCHITECTURES x86_64 PARENT_SCOPE )
  endif( )
endfunction( )

# .....................................................
function( collect_oses )
  if( ${CMAKE_SYSTEM_NAME} MATCHES Windows )
    set( Windows    true PARENT_SCOPE )
    set( NotLinux   true PARENT_SCOPE )
    set( NotDarwin  true PARENT_SCOPE )

  elseif( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
    set( Linux      true PARENT_SCOPE )
    set( NotDarwin  true PARENT_SCOPE )
    set( NotWindows true PARENT_SCOPE )

  elseif( ${CMAKE_SYSTEM_NAME} MATCHES Darwin )
    set( Darwin     true PARENT_SCOPE )
    set( NotLinux   true PARENT_SCOPE )
    set( NotWindows true PARENT_SCOPE )

  endif( )
endfunction( )

# .....................................................
function( append what where )
  set(${ARGV1} "${ARGV0} ${${ARGV1}}" PARENT_SCOPE)
endfunction( )

# .....................................................
function( create_build )
  file( GLOB_RECURSE components "${CMAKE_SOURCE_DIR}/*/CMakeLists.txt" )
  foreach( component ${components} )
    get_filename_component( path ${component} PATH )
    add_subdirectory( ${path} )
  endforeach( )
endfunction( )

# .....................................................
function( get_source include sources)
  file( GLOB_RECURSE include "[^.]*.h" )
  file( GLOB_RECURSE sources "[^.]*.c" "[^.]*.cpp" )

  set( ${ARGV0} ${include} PARENT_SCOPE )
  set( ${ARGV1} ${sources} PARENT_SCOPE )
endfunction( )

#
# Usage: get_target(target name none|<variant>)
#
function( get_target target name variant)
  if( ARGV1 MATCHES shortname )
    get_filename_component( tname ${CMAKE_CURRENT_SOURCE_DIR} NAME )
  else( )
    set( tname ${ARGV1} )
  endif( )
  if( ARGV2 MATCHES none OR ARGV2 MATCHES universal OR DEFINED USE_STRICT_NAME)
    set( target ${tname} )
  else( )
   set( target ${tname}_${ARGV2} )
  endif( )

  set( ${ARGV0} ${target} PARENT_SCOPE )
endfunction( )

# .....................................................
function( get_folder folder )
  set( folder ${CMAKE_PROJECT_NAME} )
  set (${ARGV0} ${folder} PARENT_SCOPE)
endfunction( )

#
# Usage:
#  make_library(shortname|<name> none|<variant> static|shared)
#    - shortname|<name>: use folder name as library name or <name> specified by user
#    - <variant>|none: build library in specified variant (with drm or x11 or wayland support, etc),
#      universal - special variant which enables compilation flags required for all backends, but
#      moves dependency to runtime instead of linktime 
#    or without variant if none was specified
#    - static|shared: build static or shared library
#
function( make_library name variant type )
  get_target( target ${ARGV0} ${ARGV1} )
  if( ${ARGV0} MATCHES shortname )
    get_folder( folder )
  else ( )
    set( folder ${ARGV0} )
  endif( )

  configure_dependencies(${target} "${DEPENDENCIES}" ${variant})
  if(SKIPPING MATCHES ${target} OR NOT_CONFIGURED MATCHES ${target})
    return()
  else()
    report_add_target(BUILDING ${target})
  endif()

  if( NOT sources )
   get_source( include sources )
  endif( )

  if( sources.plus )
    list( APPEND sources ${sources.plus} )
  endif( )

  if( ARGV2 MATCHES static )
    add_library( ${target} STATIC ${include} ${sources} )

    append_property(${target} COMPILE_FLAGS "${SCOPE_CFLAGS}")

  elseif( ARGV2 MATCHES shared )
    add_library( ${target} SHARED ${include} ${sources} )

    if( Linux )
      target_link_libraries( ${target} "-Xlinker --start-group" )
    endif( )

    foreach( lib ${LIBS_VARIANT} )
      if(ARGV1 MATCHES none OR ARGV1 MATCHES universal)
        add_dependencies( ${target} ${lib} )
        target_link_libraries( ${target} ${lib} )
      else( )
        add_dependencies( ${target} ${lib}_${ARGV1} )
        target_link_libraries( ${target} ${lib}_${ARGV1} )
      endif( )
    endforeach( )
 
    foreach( lib ${LIBS_NOVARIANT} )
      add_dependencies( ${target} ${lib} )
      target_link_libraries( ${target} ${lib} )
    endforeach( )

    if( Linux )
      target_link_libraries( ${target} "-Xlinker --end-group" )
    endif( )

    append_property(${target} COMPILE_FLAGS "${CFLAGS} ${SCOPE_CFLAGS}")
    append_property(${target} LINK_FLAGS "${LDFLAGS} ${SCOPE_LINKFLAGS}")
    foreach(lib ${LIBS} ${SCOPE_LIBS})
      target_link_libraries( ${target} ${lib} )
    endforeach()

    set_target_properties( ${target} PROPERTIES LINK_INTERFACE_LIBRARIES "" )
  endif( )

  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} ) 
  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIB_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )

  set( target ${target} PARENT_SCOPE )
endfunction( )

# .....................................................
function( make_executable name variant )
  get_target( target ${ARGV0} ${ARGV1} )
  get_folder( folder )

  configure_dependencies(${target} "${DEPENDENCIES}" ${variant})
  if(SKIPPING MATCHES ${target} OR NOT_CONFIGURED MATCHES ${target})
    return()
  else()
    report_add_target(BUILDING ${target})
  endif()

  if( NOT sources )
    get_source( include sources )
  endif( )

  if( sources.plus )
    list( APPEND sources ${sources.plus} )
  endif( )

  project( ${target} )

  add_executable( ${target} ${include} ${sources} )

  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BIN_DIR}/${CMAKE_BUILD_TYPE} FOLDER ${folder} )

  if( Linux )
    target_link_libraries( ${target} "-Xlinker --start-group" )
  endif( )

  foreach( lib ${LIBS_VARIANT} )
    if(ARGV1 MATCHES none OR ARGV1 MATCHES universal)
      add_dependencies( ${target} ${lib} )
      target_link_libraries( ${target} ${lib} )
    else( )
      add_dependencies( ${target} ${lib}_${ARGV1} )
      target_link_libraries( ${target} ${lib}_${ARGV1} )
    endif( )
  endforeach( )

  foreach( lib ${LIBS_NOVARIANT} )
    add_dependencies( ${target} ${lib} )
    target_link_libraries( ${target} ${lib} )
  endforeach( )

  if( Linux )
    target_link_libraries( ${target} "-Xlinker --end-group" )
  endif( )

  append_property(${target} COMPILE_FLAGS "${CFLAGS} ${SCOPE_CFLAGS}")
  append_property(${target} LINK_FLAGS "${LDFLAGS} ${SCOPE_LINKFLAGS}")
  foreach(lib ${LIBS} ${SCOPE_LIBS})
    target_link_libraries( ${target} ${lib} )
  endforeach()

  set( target ${target} PARENT_SCOPE )
endfunction( )
